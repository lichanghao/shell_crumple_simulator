program dump_principal_exponential_oracle
  ! Computes principal curvature outputs (task3f) and deformed-bond outputs (task3a)
  ! for the 10 archived constitutive cases (elements 83-87, both Gauss points) from the
  ! archived compression simulator state.
  !
  ! This provides Fortran oracle fixtures for:
  !   Principal.MatchesArchivedCompressionFortranOracle
  !   Exponential.MatchesArchivedCompressionFortranOracle
  !
  ! Usage:
  !   dump_principal_exponential_oracle <case-dir> <out-dir>
  !
  ! Where:
  !   <case-dir>  path to test/cases/graphene_compression_simulator/np1
  !   <out-dir>   path to test/cases/principal_exponential_oracle
  !
  ! Output: <out-dir>/case_01.dat ... case_10.dat
  !
  ! Fixture format (36 rows per file):
  !   Row  1: ielem igauss   (header)
  !   Row  2: C_elem(3)
  !   Row  3: curv0_elem(3)
  !   Row  4: flag_num_diff  (0=false, 1=true)
  !   Row  5: curvppal(2)
  !   Row  6: vppal(1,1:2)
  !   Row  7: vppal(2,1:2)
  !   Row  8: dcurvppaldC(1)%val(3)
  !   Row  9: dcurvppaldC(2)%val(3)
  !   Row 10: dcurvppaldk(1)%val(3)
  !   Row 11: dcurvppaldk(2)%val(3)
  !   Row 12: dvppaldC(1,1)%val(3)
  !   Row 13: dvppaldC(1,2)%val(3)
  !   Row 14: dvppaldC(2,1)%val(3)
  !   Row 15: dvppaldC(2,2)%val(3)
  !   Row 16: dvppaldk(1,1)%val(3)
  !   Row 17: dvppaldk(1,2)%val(3)
  !   Row 18: dvppaldk(2,1)%val(3)
  !   Row 19: dvppaldk(2,2)%val(3)
  !   Row 20: A_norm(3)   [eta=0 bond norms]
  !   Row 21: Ei(1,1:2)   [eta=0 unit bond vectors]
  !   Row 22: Ei(2,1:2)
  !   Row 23: Ei(3,1:2)
  !   Row 24: pe(6)
  !   Row 25: dpedC(1)%val(3)
  !   Row 26: dpedC(2)%val(3)
  !   Row 27: dpedC(3)%val(3)
  !   Row 28: dpedC(4)%val(3)
  !   Row 29: dpedC(5)%val(3)
  !   Row 30: dpedC(6)%val(3)
  !   Row 31: dpedk(1)%val(3)
  !   Row 32: dpedk(2)%val(3)
  !   Row 33: dpedk(3)%val(3)
  !   Row 34: dpedk(4)%val(3)
  !   Row 35: dpedk(5)%val(3)
  !   Row 36: dpedk(6)%val(3)
  !
  ! Bond geometry note: A_norm and Ei are computed with eta=0 (inner displacement zero).
  ! This makes the bond-vector oracle independent of Newton convergence.
  ! The Fortran expression is:
  !   Ei(ibond,:) = mat1%A0 * mat1%E(ibond,:)
  !   A_norm(ibond) = ||Ei(ibond,:)||
  !   Ei(ibond,:) = Ei(ibond,:) / A_norm(ibond)

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
  end interface

  integer, parameter :: ncases = 10
  character(len=512) :: case_dir, out_dir, out_path

  integer :: numele, numnods, ngauss
  integer :: ielem, igauss, icase, ibond, node
  logical :: flag_num_diff
  integer :: flag_nd_int

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
  real(8) :: A_norm(3), Ei(3, 2)
  real(8) :: pe(6)

  call get_command_argument(1, case_dir)
  call get_command_argument(2, out_dir)
  if ((len_trim(case_dir) == 0) .or. (len_trim(out_dir) == 0)) then
    stop "usage: dump_principal_exponential_oracle <case-dir> <out-dir>"
  end if

  call execute_command_line("mkdir -p " // trim(out_dir))

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

  icase = 0
  do ielem = 1, numele
    if (any(neigh_vert(:, ielem) > numnods)) cycle

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

      ! Bond geometry with eta=0 (inner displacement zero)
      do ibond = 1, 3
        Ei(ibond, :) = mat1%A0 * mat1%E(ibond, :)
        A_norm(ibond) = sqrt(Ei(ibond, 1)**2 + Ei(ibond, 2)**2)
        Ei(ibond, :) = Ei(ibond, :) / A_norm(ibond)
      end do

      call def_bonds(C_elem, curvppal, vppal, dcurvppaldC, dcurvppaldk, &
                     dvppaldC, dvppaldk, A_norm, Ei, pe, dpedC, dpedk)

      icase = icase + 1
      write(out_path, "(A,'/case_',I2.2,'.dat')") trim(out_dir), icase

      if (flag_num_diff) then
        flag_nd_int = 1
      else
        flag_nd_int = 0
      end if

      open(unit=20, file=trim(out_path), status="replace", action="write")
      write(20, "(2I8)") ielem, igauss
      write(20, "(3ES32.17E3)") C_elem
      write(20, "(3ES32.17E3)") curv0_elem
      write(20, "(I8)") flag_nd_int
      write(20, "(2ES32.17E3)") curvppal
      write(20, "(2ES32.17E3)") vppal(1, :)
      write(20, "(2ES32.17E3)") vppal(2, :)
      write(20, "(3ES32.17E3)") dcurvppaldC(1)%val
      write(20, "(3ES32.17E3)") dcurvppaldC(2)%val
      write(20, "(3ES32.17E3)") dcurvppaldk(1)%val
      write(20, "(3ES32.17E3)") dcurvppaldk(2)%val
      write(20, "(3ES32.17E3)") dvppaldC(1, 1)%val
      write(20, "(3ES32.17E3)") dvppaldC(1, 2)%val
      write(20, "(3ES32.17E3)") dvppaldC(2, 1)%val
      write(20, "(3ES32.17E3)") dvppaldC(2, 2)%val
      write(20, "(3ES32.17E3)") dvppaldk(1, 1)%val
      write(20, "(3ES32.17E3)") dvppaldk(1, 2)%val
      write(20, "(3ES32.17E3)") dvppaldk(2, 1)%val
      write(20, "(3ES32.17E3)") dvppaldk(2, 2)%val
      write(20, "(3ES32.17E3)") A_norm
      write(20, "(2ES32.17E3)") Ei(1, :)
      write(20, "(2ES32.17E3)") Ei(2, :)
      write(20, "(2ES32.17E3)") Ei(3, :)
      write(20, "(6ES32.17E3)") pe
      write(20, "(3ES32.17E3)") dpedC(1)%val
      write(20, "(3ES32.17E3)") dpedC(2)%val
      write(20, "(3ES32.17E3)") dpedC(3)%val
      write(20, "(3ES32.17E3)") dpedC(4)%val
      write(20, "(3ES32.17E3)") dpedC(5)%val
      write(20, "(3ES32.17E3)") dpedC(6)%val
      write(20, "(3ES32.17E3)") dpedk(1)%val
      write(20, "(3ES32.17E3)") dpedk(2)%val
      write(20, "(3ES32.17E3)") dpedk(3)%val
      write(20, "(3ES32.17E3)") dpedk(4)%val
      write(20, "(3ES32.17E3)") dpedk(5)%val
      write(20, "(3ES32.17E3)") dpedk(6)%val
      close(20)

      write(*, "(A,I3,A,I1)") "  wrote case ", icase, " (ielem=", ielem
      if (icase == ncases) exit
    end do
    if (icase == ncases) exit
  end do

  if (icase /= ncases) then
    stop "did not find enough archived interior cases"
  end if
  write(*, *) "Done. Wrote", ncases, "cases to", trim(out_dir)

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

end program dump_principal_exponential_oracle
