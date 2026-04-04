program dump_element_energy_oracle
  ! Computes ener_elem for element 83 (1-based) from the archived compression
  ! simulator state and dumps W_elem and f_elem to a fixture file.
  !
  ! This is the canonical Fortran reference for C++ test
  ! ElementEnergy.FElemMatchesFortranOracle.
  !
  ! The computation mirrors ener_elem.f90 without the data_crease module
  ! (ncrease=0 is assumed for the compression test) and without energy.f90
  ! (which contains MPI). Hyper_Pot and Stresses are reproduced inline.
  !
  ! Usage:
  !   dump_element_energy_oracle <case-dir> <out-dir>
  !
  ! Fixture format (case_01.dat):
  !   Line 1:  ielem  ngauss         (integers, 1-based element index)
  !   Line 2:  W_elem                (one double)
  !   Lines 3-14: f_elem(inode,1:3)  (one row per node, 3 doubles each)

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

    subroutine def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      real(8), intent(in) :: C_elem(3), curvppal(2), vppal(2, 2), A_norm(3), Ei(3, 2)
      real(8), intent(out) :: pe(6)
    end subroutine def_bonds_

    subroutine Morse(mat1, pe, W, dW)
      use data_mat
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      type(material), intent(in) :: mat1
      real(8), intent(in) :: pe(6)
      real(8), intent(out) :: W, dW(6)
    end subroutine Morse

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

  ! Target element (1-based)
  integer, parameter :: target_ielem = 83

  character(len=512) :: case_dir, out_dir, out_path

  integer :: numele, numnods, ngauss
  integer :: ielem, igauss, ibond, ij, inode
  integer :: maxn, niter, fail_mode, inner_fail

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

  real(8) :: xneigh(12, 3)
  real(8) :: DN(12, 2), DDN(12, 3)
  real(8) :: C_elem(3), curv0_elem(3), xnor_elem(3)
  real(8) :: curvppal(2), vppal(2, 2)
  real(8) :: C_elem_(3), curv0_elem_(3), curvppal_(2), vppal_(2, 2)
  real(8) :: A_norm(3), Ei(3, 2)
  real(8) :: pe(6), pe_(6), dW(6), dW_(6)
  real(8) :: S_n(3), S_m(3)
  real(8) :: eta_gauss(2)
  real(8) :: W, W_
  real(8) :: f_elem(12, 3), W_elem
  logical :: flag_num_diff, flag_dummy
  real(8), parameter :: crit = 1.0d-8
  real(8), parameter :: h = 1.0d-8

  call get_command_argument(1, case_dir)
  call get_command_argument(2, out_dir)
  if ((len_trim(case_dir) == 0) .or. (len_trim(out_dir) == 0)) then
    stop "usage: dump_element_energy_oracle <case-dir> <out-dir>"
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

  ! Build xneigh for element 83 (1-based)
  ielem = target_ielem
  do inode = 1, 12
    xneigh(inode, :) = x0(neigh_vert(inode, ielem), :)
  end do

  ! Compute ener_elem inline (ncrease=0, no crease subtraction needed)
  f_elem = 0.0d0
  W_elem = 0.0d0
  inner_fail = 0

  do igauss = 1, ngauss
    DN(:, :)  = shapef(igauss, :, 2:3)
    DDN(:, :) = shapef(igauss, :, 4:6)

    call metric(xneigh, DN, F0(:, :, ielem), C_elem, dC, xnor_elem, dnorm)
    call curv(xneigh, DDN, F0(:, :, ielem), xnor_elem, dnorm, curv0_elem, dcurv)
    call principal(C_elem, curv0_elem, curvppal, vppal, &
                   dcurvppaldC, dcurvppaldk, dvppaldC, dvppaldk, flag_num_diff)

    ! Inner Newton from eta=0 (matches C++ test initial condition)
    eta_gauss = 0.0d0
    maxn = 100
    call newton_inner(C_elem, curvppal, vppal, mat1, eta_gauss, W, dW, crit, niter, maxn, fail_mode)
    if (fail_mode /= 0) then
      write(*, *) "WARNING: newton_inner fail for igauss=", igauss, " fail_mode=", fail_mode
      inner_fail = inner_fail + 1
    end if

    ! Bond vectors at converged eta
    do ibond = 1, 3
      Ei(ibond, :) = mat1%A0 * mat1%E(ibond, :) + eta_gauss(:)
      A_norm(ibond) = sqrt(Ei(ibond, 1)**2 + Ei(ibond, 2)**2)
      Ei(ibond, :) = Ei(ibond, :) / A_norm(ibond)
    end do

    if (flag_num_diff) then
      ! Numerical differentiation path (canonical ener_elem.f90 lines 60-84)
      ! Both S_n and S_m perturb C_elem (Fortran S_m loop is identical to S_n)
      call def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)
      call My_Hyper_Pot(mat1, pe, W, dW)
      do ij = 1, 3
        C_elem_ = C_elem
        curv0_elem_ = curv0_elem
        C_elem_(ij) = C_elem_(ij) + h
        call principal_(C_elem_, curv0_elem_, curvppal_, vppal_, flag_dummy)
        call def_bonds_(C_elem_, curvppal_, vppal_, A_norm, Ei, pe_)
        call My_Hyper_Pot(mat1, pe_, W_, dW_)
        S_n(ij) = (W_ - W) / h
      end do
      do ij = 1, 3
        C_elem_ = C_elem
        curv0_elem_ = curv0_elem
        C_elem_(ij) = C_elem_(ij) + h
        call principal_(C_elem_, curv0_elem_, curvppal_, vppal_, flag_dummy)
        call def_bonds_(C_elem_, curvppal_, vppal_, A_norm, Ei, pe_)
        call My_Hyper_Pot(mat1, pe_, W_, dW_)
        S_m(ij) = (W_ - W) / h
      end do
    else
      ! Analytical path (canonical ener_elem.f90 lines 85-95)
      call def_bonds(C_elem, curvppal, vppal, dcurvppaldC, dcurvppaldk, &
                     dvppaldC, dvppaldk, A_norm, Ei, pe, dpedC, dpedk)
      call My_Hyper_Pot(mat1, pe, W, dW)
      call My_Stresses(dW, dpedC, dpedk, S_n, S_m)
    end if

    ! Force and energy accumulation (canonical ener_elem.f90 lines 97-101)
    do ij = 1, 3
      f_elem = f_elem + (S_n(ij)*dC(:, :)%val(ij) + S_m(ij)*dcurv(:, :)%val(ij)) * weight(igauss)
    end do
    W_elem = W_elem + W * weight(igauss)
  end do

  if (inner_fail /= 0) then
    write(*, *) "ERROR: inner Newton failed for", inner_fail, "Gauss point(s)"
    stop
  end if

  call execute_command_line("mkdir -p " // trim(out_dir))
  write(out_path, "(A,'/case_01.dat')") trim(out_dir)
  call write_fixture(out_path, ielem, ngauss, W_elem, f_elem)
  write(*, "(A,ES14.6)") "W_elem = ", W_elem

contains

  ! Inline Hyper_Pot wrapper: avoids energy.f90 MPI dependency.
  ! For the archived compression case, nCode_Pot=1 (Morse).
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
      stop "My_Hyper_Pot: only Morse (nCode_Pot=1) supported in element-energy oracle"
    end if
  end subroutine My_Hyper_Pot

  ! Inline Stresses: matches Stresses subroutine in energy.f90 lines 196-210.
  subroutine My_Stresses(dW, dpedC, dpedk, S_n, S_m)
    use data_vector3
    implicit real(8) (a - h, o - z)
    implicit integer*4(i - n)
    real(8), intent(in) :: dW(6)
    type(vector3), intent(in) :: dpedC(6), dpedk(6)
    real(8), intent(out) :: S_n(3), S_m(3)
    integer :: i

    S_n = 0.0d0
    S_m = 0.0d0
    do i = 1, 6
      S_n = S_n + dW(i) * dpedC(i)%val
      S_m = S_m + dW(i) * dpedk(i)%val
    end do
  end subroutine My_Stresses

  subroutine write_fixture(path, ielem_out, ngauss_out, W_elem_out, f_elem_out)
    character(len=*), intent(in) :: path
    integer, intent(in) :: ielem_out, ngauss_out
    real(8), intent(in) :: W_elem_out
    real(8), intent(in) :: f_elem_out(12, 3)
    integer :: node

    open(unit=20, file=path, status="replace", action="write")
    write(20, "(2I8)") ielem_out, ngauss_out
    write(20, "(ES32.17E3)") W_elem_out
    do node = 1, 12
      write(20, "(3ES32.17E3)") f_elem_out(node, 1), f_elem_out(node, 2), f_elem_out(node, 3)
    end do
    close(20)
  end subroutine write_fixture

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
    else if (mat1%nCode_Pot == 2) then
      read(11, *) mat1%A1
      read(11, *) mat1%Vs(1)
      read(11, *) mat1%Vs(2)
      read(11, *) mat1%Vs(3)
      read(11, *) mat1%Va(1)
      read(11, *) mat1%Va(2)
      read(11, *) mat1%Va(3)
    else
      stop "unsupported material potential code"
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
    integer :: ie
    real(8) :: J0_dummy

    open(unit=12, file=path, status="old", action="read")
    read(12, "(A)")
    read(12, "(A)")
    do ie = 1, numele
      read(12, *) J0_dummy
      read(12, *) F0(1, 1, ie), F0(1, 2, ie)
      read(12, *) F0(2, 1, ie), F0(2, 2, ie)
    end do
    close(12)
  end subroutine read_zero

  subroutine read_config(path, numnods, x0)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numnods
    real(8), intent(out) :: x0(numnods, 3)
    integer :: in

    open(unit=13, file=path, status="old", action="read")
    read(13, "(A)")
    read(13, "(A)")
    read(13, "(A)")
    do in = 1, numnods
      read(13, *) x0(in, 1), x0(in, 2), x0(in, 3)
    end do
    close(13)
  end subroutine read_config

  subroutine read_mesh_patches(path, numele, neigh_vert)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numele
    integer, intent(out) :: neigh_vert(12, numele)
    integer :: ie, jj, ne_dummy, nv_dummy, ie_dummy, v1, v2, v3
    integer :: code_bc_dummy(3), neigh_elem_dummy

    open(unit=14, file=path, status="old", action="read")
    read(14, "(A)")
    read(14, "(A)")
    read(14, "(A)")
    do ie = 1, numele
      read(14, "(A)")
      read(14, *) ie_dummy, v1, v2, v3
      read(14, *) ne_dummy
      read(14, *) nv_dummy
      do jj = 1, 12
        read(14, *) neigh_elem_dummy, neigh_vert(jj, ie)
      end do
      read(14, *) code_bc_dummy(1), code_bc_dummy(2), code_bc_dummy(3)
    end do
    close(14)
  end subroutine read_mesh_patches

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

end program dump_element_energy_oracle
