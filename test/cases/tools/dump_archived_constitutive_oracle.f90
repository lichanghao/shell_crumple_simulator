program dump_archived_constitutive_oracle
  use data_mat
  use data_vector2
  use data_vector3
  implicit none

  interface
    subroutine gauss(ngauss, shapef, weight)
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      integer, intent(in) :: ngauss
      real(8), intent(out) :: shapef(ngauss, 12, 6)
      real(8), intent(out) :: weight(ngauss)
    end subroutine gauss

    subroutine metric(xneigh, DN, F0, C_elem, dC, xnor_elem, dnorm)
      use data_vector3
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: xneigh(12, 3), DN(12, 2), F0(2, 2)
      real(8), intent(out) :: C_elem(3), xnor_elem(3)
      type(vector3), intent(out) :: dC(12, 3), dnorm(12, 3)
    end subroutine metric

    subroutine curv(xneigh, DDN, F0, xnor_elem, dnorm, curv0_elem, dcurv)
      use data_vector3
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: xneigh(12, 3), DDN(12, 3), F0(2, 2), xnor_elem(3)
      real(8), intent(out) :: curv0_elem(3)
      type(vector3), intent(in) :: dnorm(12, 3)
      type(vector3), intent(out) :: dcurv(12, 3)
    end subroutine curv

    subroutine principal(C_elem, curv0_elem, curvppal, vppal, &
                         dcurvppaldC, dcurvppaldk, dvppaldC, dvppaldk, flag_num_diff)
      use data_vector3
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: C_elem(3), curv0_elem(3)
      real(8), intent(out) :: curvppal(2), vppal(2, 2)
      type(vector3), intent(out) :: dcurvppaldC(2), dcurvppaldk(2)
      type(vector3), intent(out) :: dvppaldC(2, 2), dvppaldk(2, 2)
      logical, intent(out) :: flag_num_diff
    end subroutine principal

    subroutine def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), A_norm(3), Ei(3, 2)
      real(8), intent(out) :: pe(6)
    end subroutine def_bonds_

    subroutine Hyper_pot_inner(C_elem, curvppal, vppal, mat1, eta, W, dWdeta, ddWdeta, dW)
      use data_mat
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      type(material), intent(in) :: mat1
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), eta(2)
      real(8), intent(out) :: W, dWdeta(2), ddWdeta(3), dW(6)
    end subroutine Hyper_pot_inner

    subroutine newton_inner(C_elem, curvppal, vppal, mat1, x, W, dWdp, crit, n, maxn, fail_mode)
      use data_mat
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      type(material), intent(in) :: mat1
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), crit
      real(8), intent(inout) :: x(2)
      real(8), intent(out) :: W, dWdp(6)
      integer, intent(out) :: n, fail_mode
      integer, intent(inout) :: maxn
    end subroutine newton_inner
  end interface

  integer, parameter :: ncases = 10
  character(len=512) :: case_dir
  character(len=512) :: out_dir
  character(len=512) :: dims_path
  character(len=512) :: general_path
  character(len=512) :: zero_path
  character(len=512) :: config_path
  character(len=512) :: mesh_path
  character(len=512) :: out_path
  integer :: numele
  integer :: numnods
  integer :: ngauss
  integer :: ielem
  integer :: igauss
  integer :: icase
  integer :: maxn
  integer :: niter
  integer :: fail_mode
  integer :: ibond
  integer :: node
  logical :: flag_num_diff

  type(material) :: mat1
  type(vector3) :: dC(12, 3)
  type(vector3) :: dcurv(12, 3)
  type(vector3) :: dnorm(12, 3)
  type(vector3) :: dcurvppaldC(2)
  type(vector3) :: dcurvppaldk(2)
  type(vector3) :: dvppaldC(2, 2)
  type(vector3) :: dvppaldk(2, 2)

  integer, allocatable :: neigh_vert(:, :)
  real(8), allocatable :: x0(:, :)
  real(8), allocatable :: eta(:, :, :)
  real(8), allocatable :: F0(:, :, :)
  real(8), allocatable :: shapef(:, :, :)
  real(8), allocatable :: weight(:)

  real(8) :: xneigh(12, 3)
  real(8) :: DN(12, 2)
  real(8) :: DDN(12, 3)
  real(8) :: C_elem(3)
  real(8) :: curv0_elem(3)
  real(8) :: curvppal(2)
  real(8) :: vppal(2, 2)
  real(8) :: xnor_elem(3)
  real(8) :: A_norm(3)
  real(8) :: Ei(3, 2)
  real(8) :: pe(6)
  real(8) :: eta_archived(2)
  real(8) :: eta0(2)
  real(8) :: W
  real(8) :: dWdeta(2)
  real(8) :: ddWdeta(3)
  real(8) :: dW(6)
  real(8), parameter :: crit = 1.0d-8

  call get_command_argument(1, case_dir)
  call get_command_argument(2, out_dir)
  if ((len_trim(case_dir) == 0) .or. (len_trim(out_dir) == 0)) then
    stop "usage: dump_archived_constitutive_oracle <case-dir> <out-dir>"
  end if

  dims_path = trim(case_dir) // "/nano_dims.dat"
  general_path = trim(case_dir) // "/nano_general.dat"
  zero_path = trim(case_dir) // "/nano_zero.dat"
  config_path = trim(case_dir) // "/nano_final_config.dat"
  mesh_path = trim(case_dir) // "/nano_Mesh.dat"

  call execute_command_line("mkdir -p " // trim(out_dir))

  call read_dims(dims_path, numele, numnods, ngauss)
  allocate(neigh_vert(12, numele))
  allocate(x0(numnods, 3))
  allocate(eta(ngauss, 2, numele))
  allocate(F0(2, 2, numele))
  allocate(shapef(ngauss, 12, 6))
  allocate(weight(ngauss))

  call read_general_material(general_path, mat1)
  call read_zero(zero_path, numele, F0)
  call read_config(config_path, numnods, numele, ngauss, x0, eta)
  call read_mesh_patches(mesh_path, numele, neigh_vert)
  call gauss(ngauss, shapef, weight)

  icase = 0
  do ielem = 1, numele
    if (any(neigh_vert(:, ielem) .gt. numnods)) cycle

    do node = 1, 12
      xneigh(node, :) = x0(neigh_vert(node, ielem), :)
    end do

    do igauss = 1, ngauss
      DN(:, :) = shapef(igauss, :, 2:3)
      DDN(:, :) = shapef(igauss, :, 4:6)

      call metric(xneigh, DN, F0(:, :, ielem), C_elem, dC, xnor_elem, dnorm)
      call curv(xneigh, DDN, F0(:, :, ielem), xnor_elem, dnorm, curv0_elem, dcurv)
      call principal(C_elem, curv0_elem, curvppal, vppal, &
                     dcurvppaldC, dcurvppaldk, dvppaldC, dvppaldk, flag_num_diff)

      eta_archived(:) = eta(igauss, :, ielem)
      do ibond = 1, 3
        Ei(ibond, :) = mat1%A0 * mat1%E(ibond, :) + eta_archived(:)
        A_norm(ibond) = sqrt(Ei(ibond, 1) * Ei(ibond, 1) + Ei(ibond, 2) * Ei(ibond, 2))
        Ei(ibond, :) = Ei(ibond, :) / A_norm(ibond)
      end do
      call def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)

      eta0(:) = eta_archived(:)
      maxn = 100
      call newton_inner(C_elem, curvppal, vppal, mat1, eta0, W, dW, crit, niter, maxn, fail_mode)
      call Hyper_pot_inner(C_elem, curvppal, vppal, mat1, eta0, W, dWdeta, ddWdeta, dW)

      icase = icase + 1
      write(out_path, "(A,'/case_',I2.2,'.dat')") trim(out_dir), icase
      call write_case(out_path, ielem, igauss, C_elem, curv0_elem, curvppal, vppal, &
                      eta_archived, A_norm, Ei, pe, crit, 100, niter, fail_mode, &
                      eta0, W, dWdeta, ddWdeta, dW)
      if (icase == ncases) exit
    end do
    if (icase == ncases) exit
  end do

  if (icase /= ncases) then
    stop "did not find enough archived interior constitutive cases"
  end if

contains

  subroutine read_dims(path, numele, numnods, ngauss)
    character(len=*), intent(in) :: path
    integer, intent(out) :: numele
    integer, intent(out) :: numnods
    integer, intent(out) :: ngauss

    open(unit=10, file=path, status="old", action="read")
    call skip_to_label(10, "mesh0%numele")
    read(10, *) numele
    call skip_to_label(10, "mesh0%numnods")
    read(10, *) numnods
    call skip_to_label(10, "ngauss")
    read(10, *) ngauss
    close(10)
  end subroutine read_dims

  subroutine read_general_material(path, mat1)
    character(len=*), intent(in) :: path
    type(material), intent(out) :: mat1
    integer :: i

    open(unit=11, file=path, status="old", action="read")
    call skip_to_label(11, "mat1%A0")
    read(11, *) mat1%A0
    call skip_to_label(11, "mat1%nCode_Pot")
    read(11, *) mat1%nCode_Pot
    if (mat1%nCode_Pot == 1) then
      read(11, *) mat1%Vs(1)
      read(11, *) mat1%Vs(2)
      read(11, *) mat1%Va(1)
      read(11, *) mat1%Va(2)
    else if (mat1%nCode_Pot == 2) then
      read(11, *) mat1%A1
      read(11, *) mat1%Vs(1)
      read(11, *) mat1%Vs(2)
      read(11, *) mat1%Vs(3)
      read(11, *) mat1%Va(1)
      read(11, *) mat1%Va(2)
      read(11, *) mat1%Va(3)
    else if (mat1%nCode_Pot == 3) then
      read(11, *) mat1%Vs(1)
      read(11, *) mat1%Va(1)
    else if (mat1%nCode_Pot == 22) then
      read(11, *) mat1%A1
      read(11, *) mat1%Vs(1)
      read(11, *) mat1%Vs(2)
      read(11, *) mat1%Vs(3)
      read(11, *) mat1%Va(1)
      read(11, *) mat1%Va(2)
      read(11, *) mat1%Va(3)
    else
      stop "unsupported material potential code in archived general file"
    end if
    call skip_to_label(11, "mat1%E")
    do i = 1, 3
      read(11, *) mat1%E(i, 1), mat1%E(i, 2)
    end do
    call skip_to_label(11, "mat1%s0")
    read(11, *) mat1%s0
    close(11)
  end subroutine read_general_material

  subroutine read_zero(path, numele, F0)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numele
    real(8), intent(out) :: F0(2, 2, numele)
    integer :: ielem
    real(8) :: J0_dummy

    open(unit=12, file=path, status="old", action="read")
    read(12, "(A)")
    read(12, "(A)")
    do ielem = 1, numele
      read(12, *) J0_dummy
      read(12, *) F0(1, 1, ielem), F0(1, 2, ielem)
      read(12, *) F0(2, 1, ielem), F0(2, 2, ielem)
    end do
    close(12)
  end subroutine read_zero

  subroutine read_config(path, numnods, numele, ngauss, x0, eta)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numnods
    integer, intent(in) :: numele
    integer, intent(in) :: ngauss
    real(8), intent(out) :: x0(numnods, 3)
    real(8), intent(out) :: eta(ngauss, 2, numele)
    integer :: inode
    integer :: ielem
    integer :: igauss

    open(unit=13, file=path, status="old", action="read")
    read(13, "(A)")
    read(13, "(A)")
    read(13, "(A)")
    do inode = 1, numnods
      read(13, *) x0(inode, 1), x0(inode, 2), x0(inode, 3)
    end do
    call skip_to_label(13, "Inner displacements")
    do ielem = 1, numele
      do igauss = 1, ngauss
        read(13, *) eta(igauss, 1, ielem), eta(igauss, 2, ielem)
      end do
    end do
    close(13)
  end subroutine read_config

  subroutine read_mesh_patches(path, numele, neigh_vert)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numele
    integer, intent(out) :: neigh_vert(12, numele)
    character(len=512) :: line
    integer :: ielem
    integer :: jj
    integer :: num_neigh_elem
    integer :: num_neigh_vert_dummy
    integer :: ielem_dummy
    integer :: v1_dummy
    integer :: v2_dummy
    integer :: v3_dummy
    integer :: code_bc_dummy(3)
    integer :: neigh_elem_dummy

    open(unit=14, file=path, status="old", action="read")
    read(14, "(A)")
    read(14, "(A)")
    read(14, "(A)")
    do ielem = 1, numele
      read(14, "(A)") line
      read(14, *) ielem_dummy, v1_dummy, v2_dummy, v3_dummy
      read(14, *) num_neigh_elem
      read(14, *) num_neigh_vert_dummy
      do jj = 1, 12
        read(14, *) neigh_elem_dummy, neigh_vert(jj, ielem)
      end do
      read(14, *) code_bc_dummy(1), code_bc_dummy(2), code_bc_dummy(3)
    end do
    close(14)
  end subroutine read_mesh_patches

  subroutine write_case(path, ielem, igauss, C_elem, curv0_elem, curvppal, vppal, &
                        eta_archived, A_norm, Ei, pe, crit, max_iter, niter, fail_mode, &
                        eta_final, W, dWdeta, ddWdeta, dW)
    character(len=*), intent(in) :: path
    integer, intent(in) :: ielem
    integer, intent(in) :: igauss
    real(8), intent(in) :: C_elem(3), curv0_elem(3), curvppal(2), vppal(2, 2)
    real(8), intent(in) :: eta_archived(2), A_norm(3), Ei(3, 2), pe(6)
    real(8), intent(in) :: crit, eta_final(2), W, dWdeta(2), ddWdeta(3), dW(6)
    integer, intent(in) :: max_iter, niter, fail_mode
    real(8) :: W_out
    real(8) :: dWdeta_out(2)
    real(8) :: ddWdeta_out(3)
    real(8) :: dW_out(6)

    W_out = W
    dWdeta_out = dWdeta
    ddWdeta_out = ddWdeta
    dW_out = dW
    call zero_tiny_scalar(W_out)
    call zero_tiny_vector(dWdeta_out)
    call zero_tiny_vector(ddWdeta_out)
    call zero_tiny_vector(dW_out)

    open(unit=15, file=path, status="replace", action="write")
    write(15, "(2I8)") ielem, igauss
    write(15, "(3ES32.17E3)") C_elem
    write(15, "(3ES32.17E3)") curv0_elem
    write(15, "(2ES32.17E3)") curvppal
    write(15, "(2ES32.17E3)") vppal(1, :)
    write(15, "(2ES32.17E3)") vppal(2, :)
    write(15, "(2ES32.17E3)") eta_archived
    write(15, "(3ES32.17E3)") A_norm
    write(15, "(2ES32.17E3)") Ei(1, :)
    write(15, "(2ES32.17E3)") Ei(2, :)
    write(15, "(2ES32.17E3)") Ei(3, :)
    write(15, "(6ES32.17E3)") pe
    write(15, "(2ES32.17E3)") eta_archived
    write(15, "(ES32.17E3,1X,I0)") crit, max_iter
    write(15, "(I0,1X,I0)") niter, fail_mode
    write(15, "(2ES32.17E3)") eta_final
    write(15, "(ES32.17E3)") W_out
    write(15, "(2ES32.17E3)") dWdeta_out
    write(15, "(3ES32.17E3)") ddWdeta_out
    write(15, "(6ES32.17E3)") dW_out
    close(15)
  end subroutine write_case

  subroutine zero_tiny_scalar(value)
    real(8), intent(inout) :: value
    if (abs(value) < 1.0d-300) value = 0.0d0
  end subroutine zero_tiny_scalar

  subroutine zero_tiny_vector(values)
    real(8), intent(inout) :: values(:)
    integer :: i
    do i = 1, size(values)
      call zero_tiny_scalar(values(i))
    end do
  end subroutine zero_tiny_vector

  subroutine skip_to_label(unit_no, label)
    integer, intent(in) :: unit_no
    character(len=*), intent(in) :: label
    character(len=512) :: line
    integer :: ios

    do
      read(unit_no, "(A)", iostat=ios) line
      if (ios /= 0) stop "label not found"
      if (trim(adjustl(line)) == label) return
    end do
  end subroutine skip_to_label

end program dump_archived_constitutive_oracle
