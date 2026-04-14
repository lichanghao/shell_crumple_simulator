program dump_first_step_full_oracle
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

    subroutine principal_(C_elem, curv0_elem, curvppal, vppal, flag_num_diff)
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: C_elem(3), curv0_elem(3)
      real(8), intent(out) :: curvppal(2), vppal(2, 2)
      logical, intent(out) :: flag_num_diff
    end subroutine principal_

    subroutine def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), A_norm(3), Ei(3, 2)
      real(8), intent(out) :: pe(6)
    end subroutine def_bonds_

    subroutine def_bonds(C_elem, curvppal, vppal, dcurvppaldC, dcurvppaldk, &
                         dvppaldC, dvppaldk, A_norm, Ei, pe, dpedC, dpedk)
      use data_vector3
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), A_norm(3), Ei(3, 2)
      type(vector3), intent(in) :: dcurvppaldC(2), dcurvppaldk(2)
      type(vector3), intent(in) :: dvppaldC(2, 2), dvppaldk(2, 2)
      real(8), intent(out) :: pe(6)
      type(vector3), intent(out) :: dpedC(6), dpedk(6)
    end subroutine def_bonds

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

    subroutine Hyper_pot_inner(C_elem, curvppal, vppal, mat1, eta, W, dWdeta, ddWdeta, dW)
      use data_mat
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      type(material), intent(in) :: mat1
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), eta(2)
      real(8), intent(out) :: W, dWdeta(2), ddWdeta(3), dW(6)
    end subroutine Hyper_pot_inner
  end interface

  integer, parameter :: target_ielem = 83

  character(len=512) :: case_dir, out_path
  integer :: numele, numnods, ngauss
  integer :: ielem, igauss, ibond, inode, maxn, niter, fail_mode

  type(material) :: mat1
  type(vector3) :: dC(12, 3), dcurv(12, 3), dnorm(12, 3)
  type(vector3) :: dcurvppaldC(2), dcurvppaldk(2)
  type(vector3) :: dvppaldC(2, 2), dvppaldk(2, 2)
  type(vector3) :: dpedC(6), dpedk(6)

  integer, allocatable :: neigh_vert(:, :)
  real(8), allocatable :: x0(:, :)
  real(8), allocatable :: F0(:, :, :)
  real(8), allocatable :: shapef(:, :, :)
  real(8), allocatable :: weight(:)

  real(8) :: xneigh(12, 3), DN(12, 2), DDN(12, 3)
  real(8) :: C_elem(3), curv0_elem(3), xnor_elem(3)
  real(8) :: curvppal(2), vppal(2, 2), C_elem_(3), curv0_elem_(3), curvppal_(2), vppal_(2, 2)
  real(8) :: A_norm(3), Ei(3, 2), pe(6), pe_(6)
  real(8) :: eta_final(2), W, dWdeta(2), ddWdeta(3), dW(6)
  real(8) :: f_elem(12, 3), W_elem, W_, dW_(6), S_n(3), S_m(3)
  logical :: flag_num_diff
  real(8), parameter :: crit = 1.0d-8, h = 1.0d-8
  integer :: ij

  call get_command_argument(1, case_dir)
  call get_command_argument(2, out_path)
  if ((len_trim(case_dir) == 0) .or. (len_trim(out_path) == 0)) then
    stop "usage: dump_first_step_full_oracle <case-dir> <out-path>"
  end if

  call read_dims(trim(case_dir) // "/nano_dims.dat", numele, numnods, ngauss)
  allocate(neigh_vert(12, numele))
  allocate(x0(numnods, 3))
  allocate(F0(2, 2, numele))
  allocate(shapef(ngauss, 12, 6))
  allocate(weight(ngauss))

  call read_general_material(trim(case_dir) // "/nano_general.dat", mat1)
  call read_zero(trim(case_dir) // "/nano_zero.dat", numele, F0)
  call read_config(trim(case_dir) // "/nano_final_config.dat", numnods, x0)
  call read_mesh_patches(trim(case_dir) // "/nano_Mesh.dat", numele, neigh_vert)
  call gauss(ngauss, shapef, weight)

  ielem = target_ielem
  do inode = 1, 12
    xneigh(inode, :) = x0(neigh_vert(inode, ielem), :)
  end do

  open(unit=15, file=trim(out_path), status="replace", action="write")
  write(15, "(2I8)") ielem, ngauss

  f_elem = 0.d0
  W_elem = 0.d0

  do igauss = 1, ngauss
    DN(:, :) = shapef(igauss, :, 2:3)
    DDN(:, :) = shapef(igauss, :, 4:6)

    call metric(xneigh, DN, F0(:, :, ielem), C_elem, dC, xnor_elem, dnorm)
    call curv(xneigh, DDN, F0(:, :, ielem), xnor_elem, dnorm, curv0_elem, dcurv)
    call principal(C_elem, curv0_elem, curvppal, vppal, &
                   dcurvppaldC, dcurvppaldk, dvppaldC, dvppaldk, flag_num_diff)

    eta_final = 0.d0
    maxn = 100
    call newton_inner(C_elem, curvppal, vppal, mat1, eta_final, W, dW, crit, niter, maxn, fail_mode)
    call Hyper_pot_inner(C_elem, curvppal, vppal, mat1, eta_final, W, dWdeta, ddWdeta, dW)

    do ibond = 1, 3
      Ei(ibond, :) = mat1%A0 * mat1%E(ibond, :) + eta_final(:)
      A_norm(ibond) = sqrt(Ei(ibond, 1) * Ei(ibond, 1) + Ei(ibond, 2) * Ei(ibond, 2))
      Ei(ibond, :) = Ei(ibond, :) / A_norm(ibond)
    end do
    call def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)

    if (flag_num_diff) then
      call My_Hyper_Pot(mat1, pe, W, dW)
      do ij = 1, 3
        C_elem_ = C_elem
        curv0_elem_ = curv0_elem
        C_elem_(ij) = C_elem_(ij) + h
        call principal_(C_elem_, curv0_elem_, curvppal_, vppal_, flag_num_diff)
        call def_bonds_(C_elem_, curvppal_, vppal_, A_norm, Ei, pe_)
        call My_Hyper_Pot(mat1, pe_, W_, dW_)
        S_n(ij) = (W_ - W) / h
      end do
      do ij = 1, 3
        C_elem_ = C_elem
        curv0_elem_ = curv0_elem
        C_elem_(ij) = C_elem_(ij) + h
        call principal_(C_elem_, curv0_elem_, curvppal_, vppal_, flag_num_diff)
        call def_bonds_(C_elem_, curvppal_, vppal_, A_norm, Ei, pe_)
        call My_Hyper_Pot(mat1, pe_, W_, dW_)
        S_m(ij) = (W_ - W) / h
      end do
    else
      call def_bonds(C_elem, curvppal, vppal, dcurvppaldC, dcurvppaldk, &
                     dvppaldC, dvppaldk, A_norm, Ei, pe, dpedC, dpedk)
      call My_Hyper_Pot(mat1, pe, W, dW)
      call My_Stresses(dW, dpedC, dpedk, S_n, S_m)
    end if

    write(15, "(3ES32.17E3)") C_elem
    write(15, "(3ES32.17E3)") curv0_elem
    write(15, "(2ES32.17E3)") curvppal
    write(15, "(2ES32.17E3)") vppal(1, :)
    write(15, "(2ES32.17E3)") vppal(2, :)
    write(15, "(I8)") merge(1, 0, flag_num_diff)
    write(15, "(6ES32.17E3)") pe
    write(15, "(2I8)") niter, fail_mode
    write(15, "(2ES32.17E3)") eta_final
    write(15, "(ES32.17E3)") W
    write(15, "(3ES32.17E3)") ddWdeta

    do ij = 1, 3
      f_elem = f_elem + (S_n(ij)*dC(:, :)%val(ij) + S_m(ij)*dcurv(:, :)%val(ij)) * weight(igauss)
    end do
    W_elem = W_elem + W * weight(igauss)
  end do

  write(15, "(ES32.17E3)") W_elem
  do inode = 1, 12
    write(15, "(3ES32.17E3)") f_elem(inode, :)
  end do
  close(15)

contains

  subroutine read_dims(path, numele, numnods, ngauss)
    character(len=*), intent(in) :: path
    integer, intent(out) :: numele, numnods, ngauss
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
    else
      stop "expected Morse potential code 1"
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
    real(8) :: dummy
    open(unit=12, file=path, status="old", action="read")
    read(12, "(A)")
    read(12, "(A)")
    do ielem = 1, numele
      read(12, *) dummy
      read(12, *) F0(1, 1, ielem), F0(1, 2, ielem)
      read(12, *) F0(2, 1, ielem), F0(2, 2, ielem)
    end do
    close(12)
  end subroutine read_zero

  subroutine read_config(path, numnods, x0)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numnods
    real(8), intent(out) :: x0(numnods, 3)
    integer :: inode
    open(unit=13, file=path, status="old", action="read")
    read(13, "(A)")
    read(13, "(A)")
    read(13, "(A)")
    do inode = 1, numnods
      read(13, *) x0(inode, 1), x0(inode, 2), x0(inode, 3)
    end do
    close(13)
  end subroutine read_config

  subroutine read_mesh_patches(path, numele, neigh_vert)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numele
    integer, intent(out) :: neigh_vert(12, numele)
    character(len=512) :: line
    integer :: ielem, jj, a, b, c, d, code_bc_dummy(3), neigh_elem_dummy
    open(unit=14, file=path, status="old", action="read")
    read(14, "(A)")
    read(14, "(A)")
    read(14, "(A)")
    do ielem = 1, numele
      read(14, "(A)") line
      read(14, *) a, b, c, d
      read(14, *) a
      read(14, *) a
      do jj = 1, 12
        read(14, *) neigh_elem_dummy, neigh_vert(jj, ielem)
      end do
      read(14, *) code_bc_dummy(1), code_bc_dummy(2), code_bc_dummy(3)
    end do
    close(14)
  end subroutine read_mesh_patches

  subroutine skip_to_label(unit_no, label)
    integer, intent(in) :: unit_no
    character(len=*), intent(in) :: label
    character(len=512) :: line
    do
      read(unit_no, "(A)", end=900) line
      if (trim(adjustl(line)) == trim(label)) return
    end do
900 stop "label not found"
  end subroutine skip_to_label

  subroutine My_Hyper_Pot(mat1, pe, W, dW)
    use data_mat
    implicit real(8) (a - h, o - z)
    implicit integer*4(i - n)
    type(material), intent(in) :: mat1
    real(8), intent(in) :: pe(6)
    real(8), intent(out) :: W, dW(6)
    interface
      subroutine Morse(mat1, pe, W, dW)
        use data_mat
        implicit real(8) (a - h, o - z)
        implicit integer*4(i - n)
        type(material), intent(in) :: mat1
        real(8), intent(in) :: pe(6)
        real(8), intent(out) :: W, dW(6)
      end subroutine Morse
    end interface
    if (mat1%nCode_Pot == 1) then
      call Morse(mat1, pe, W, dW)
    else
      stop "My_Hyper_Pot: only Morse supported"
    end if
  end subroutine My_Hyper_Pot

  subroutine My_Stresses(dW, dpedC, dpedk, S_n, S_m)
    use data_vector3
    implicit real(8) (a - h, o - z)
    implicit integer*4(i - n)
    real(8), intent(in) :: dW(6)
    type(vector3), intent(in) :: dpedC(6), dpedk(6)
    real(8), intent(out) :: S_n(3), S_m(3)
    integer :: i
    S_n = 0.d0
    S_m = 0.d0
    do i = 1, 6
      S_n = S_n + dW(i) * dpedC(i)%val
      S_m = S_m + dW(i) * dpedk(i)%val
    end do
  end subroutine My_Stresses

end program dump_first_step_full_oracle
