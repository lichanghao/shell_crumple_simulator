program dump_element_energy_brenner_oracle
  ! Computes ener_elem for element 83 (1-based) from the archived compression
  ! simulator geometry but with the Brenner REBO material (nCode_Pot=2).
  !
  ! This provides the Fortran oracle for task3c:
  !   ElementEnergy.BrennerMaterialMatchesFortranOracle
  !
  ! The computation mirrors ener_elem.f90: metric → curv → principal → newton_inner
  ! → def_bonds → Brenner (outer potential) → Stresses → f_elem assembly.
  !
  ! Usage:
  !   dump_element_energy_brenner_oracle <case-dir> <oracle-dir>
  !
  ! Where:
  !   <case-dir>   path to test/cases/graphene_compression_simulator/np1
  !   <oracle-dir> path to test/cases/element_energy_oracle  (parent dir)
  !
  ! Output: <oracle-dir>/brenner_geom_np1/case_01.dat
  !
  ! Format (14 rows, same as archived_compression_np1/case_01.dat):
  !   Row  1: ielem ngauss
  !   Row  2: W_elem
  !   Rows 3-14: f_elem(inode, 1:3)  for inode=1..12
  !
  ! Material: Brenner REBO parameters from dump_constitutive_oracle.f90.
  !   nCode_Pot = 2
  !   A0 = 0.142 Angstrom
  !   A1 = 0.142 Angstrom
  !   Vs = [0.60310500860214233, 26.25, 0.9]
  !   Va = [0.75400000810623169, 0.149, 0.25]
  !   E(1,:) = [sqrt(3)/2, 0.5], E(2,:) = [-sqrt(3)/2, 0.5], E(3,:) = [0, -1]

  use data_mat
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
      logical :: flag_num_diff
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

    subroutine Brenner(mat1, pe, W, dW)
      use data_mat
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      type(material), intent(in) :: mat1
      real(8), intent(in) :: pe(6)
      real(8), intent(out) :: W, dW(6)
    end subroutine Brenner

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

  character(len=512) :: case_dir, oracle_dir, out_path

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
  logical :: flag_num_diff
  real(8), parameter :: crit = 1.0d-8
  real(8), parameter :: h = 1.0d-8
  real(8), parameter :: pi = 3.141592653589793238d0

  call get_command_argument(1, case_dir)
  call get_command_argument(2, oracle_dir)
  if ((len_trim(case_dir) == 0) .or. (len_trim(oracle_dir) == 0)) then
    stop "usage: dump_element_energy_brenner_oracle <case-dir> <oracle-dir>"
  end if

  ! Initialize Brenner material (same parameters as dump_constitutive_oracle.f90)
  mat1%nCode_Pot = 2
  mat1%A0 = 0.142d0
  mat1%A1 = 0.142d0
  mat1%Vs(1) = 0.60310500860214233d0
  mat1%Vs(2) = 26.25d0
  mat1%Vs(3) = 0.9d0
  mat1%Va(1) = 0.75400000810623169d0
  mat1%Va(2) = 0.149d0
  mat1%Va(3) = 0.25d0
  mat1%E(1, 1) = sqrt(3.0d0) / 2.0d0
  mat1%E(1, 2) = 0.5d0
  mat1%E(2, 1) = -sqrt(3.0d0) / 2.0d0
  mat1%E(2, 2) = 0.5d0
  mat1%E(3, 1) = 0.0d0
  mat1%E(3, 2) = -1.0d0
  mat1%s0 = 3.0d0 * sqrt(3.0d0) / 2.0d0 * mat1%A0 * mat1%A0

  call read_dims(trim(case_dir) // "/nano_dims.dat", numele, numnods, ngauss)
  allocate(neigh_vert(12, numele))
  allocate(x0(numnods, 3))
  allocate(F0(2, 2, numele))
  allocate(shapef(ngauss, 12, 6))
  allocate(weight(ngauss))

  ! Only need mesh geometry, not archived material (we use hardcoded Brenner)
  call read_zero(trim(case_dir) // "/nano_zero.dat", numele, F0)
  call read_config(trim(case_dir) // "/nano_final_config.dat", numnods, x0)
  call read_mesh_patches(trim(case_dir) // "/nano_Mesh.dat", numele, neigh_vert)
  call gauss(ngauss, shapef, weight)

  ! Build xneigh for element 83 (1-based)
  ielem = target_ielem
  do inode = 1, 12
    xneigh(inode, :) = x0(neigh_vert(inode, ielem), :)
  end do

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

    ! Inner Newton from eta=0
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
      ! Both S_n and S_m perturb C_elem (canonical Fortran formula)
      call def_bonds_(C_elem, curvppal, vppal, A_norm, Ei, pe)
      call Brenner(mat1, pe, W, dW)
      do ij = 1, 3
        C_elem_ = C_elem
        curv0_elem_ = curv0_elem
        C_elem_(ij) = C_elem_(ij) + h
        call principal_(C_elem_, curv0_elem_, curvppal_, vppal_, flag_num_diff)
        call def_bonds_(C_elem_, curvppal_, vppal_, A_norm, Ei, pe_)
        call Brenner(mat1, pe_, W_, dW_)
        S_n(ij) = (W_ - W) / h
      end do
      do ij = 1, 3
        C_elem_ = C_elem
        curv0_elem_ = curv0_elem
        C_elem_(ij) = C_elem_(ij) + h
        call principal_(C_elem_, curv0_elem_, curvppal_, vppal_, flag_num_diff)
        call def_bonds_(C_elem_, curvppal_, vppal_, A_norm, Ei, pe_)
        call Brenner(mat1, pe_, W_, dW_)
        S_m(ij) = (W_ - W) / h
      end do
    else
      ! Analytical path (canonical ener_elem.f90 lines 85-95)
      call def_bonds(C_elem, curvppal, vppal, dcurvppaldC, dcurvppaldk, &
                     dvppaldC, dvppaldk, A_norm, Ei, pe, dpedC, dpedk)
      call Brenner(mat1, pe, W, dW)
      call My_Stresses(dW, dpedC, dpedk, S_n, S_m)
    end if

    ! Force and energy accumulation
    do ij = 1, 3
      f_elem = f_elem + (S_n(ij)*dC(:, :)%val(ij) + S_m(ij)*dcurv(:, :)%val(ij)) * weight(igauss)
    end do
    W_elem = W_elem + W * weight(igauss)
  end do

  if (inner_fail /= 0) then
    write(*, *) "ERROR: Brenner inner Newton failed for", inner_fail, "Gauss point(s)"
    stop
  end if

  call execute_command_line("mkdir -p " // trim(oracle_dir) // "/brenner_geom_np1")
  write(out_path, "(A,A)") trim(oracle_dir), "/brenner_geom_np1/case_01.dat"

  open(unit=20, file=trim(out_path), status="replace", action="write")
  write(20, "(2I8)") ielem, ngauss
  write(20, "(ES32.17E3)") W_elem
  do inode = 1, 12
    write(20, "(3ES32.17E3)") f_elem(inode, 1), f_elem(inode, 2), f_elem(inode, 3)
  end do
  close(20)

  write(*, "(A,ES14.6)") "Brenner W_elem = ", W_elem
  write(*, "(A,A)") "Wrote fixture to ", trim(out_path)

contains

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
    integer :: in_

    open(unit=13, file=path, status="old", action="read")
    read(13, "(A)")
    read(13, "(A)")
    read(13, "(A)")
    do in_ = 1, numnods
      read(13, *) x0(in_, 1), x0(in_, 2), x0(in_, 3)
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

end program dump_element_energy_brenner_oracle
