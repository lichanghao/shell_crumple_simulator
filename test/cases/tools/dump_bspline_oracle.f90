program dump_bspline_oracle
  implicit none

  interface
    subroutine BSpline(N, v, w)
      real(8) :: N(12)
      real(8) :: v
      real(8) :: w
    end subroutine BSpline

    subroutine DBSpline(DN, v, w)
      real(8) :: DN(12, 2)
      real(8) :: v
      real(8) :: w
    end subroutine DBSpline

    subroutine DDBSpline(DDN, v, w)
      real(8) :: DDN(12, 3)
      real(8) :: v
      real(8) :: w
    end subroutine DDBSpline
  end interface

  integer, parameter :: nfixtures = 10
  character(len=512) :: out_dir
  character(len=512) :: out_path
  character(len=32), dimension(nfixtures) :: names = [ &
      "interior_01.dat                ", &
      "interior_02.dat                ", &
      "interior_03.dat                ", &
      "interior_04.dat                ", &
      "interior_05.dat                ", &
      "boundary_01.dat                ", &
      "boundary_02.dat                ", &
      "boundary_03.dat                ", &
      "boundary_04.dat                ", &
      "boundary_05.dat                " ]
  real(8), dimension(nfixtures) :: vv = [ &
      1.d0 / 6.d0, &
      1.d0 / 6.d0, &
      0.2d0, &
      0.31d0, &
      0.45d0, &
      0.5d0, &
      0.d0, &
      0.5d0, &
      0.2d0, &
      0.2d0 ]
  real(8), dimension(nfixtures) :: ww = [ &
      1.d0 / 6.d0, &
      2.d0 / 3.d0, &
      0.3d0, &
      0.27d0, &
      0.1d0, &
      0.d0, &
      0.5d0, &
      0.5d0, &
      0.d0, &
      0.8d0 ]
  real(8) :: n(12)
  real(8) :: dn(12, 2)
  real(8) :: ddn(12, 3)
  integer :: i
  integer :: j

  call get_command_argument(1, out_dir)
  if (len_trim(out_dir) == 0) stop "usage: dump_bspline_oracle <out-dir>"

  do i = 1, nfixtures
    call BSpline(n, vv(i), ww(i))
    call DBSpline(dn, vv(i), ww(i))
    call DDBSpline(ddn, vv(i), ww(i))

    out_path = trim(out_dir) // "/" // trim(names(i))
    open(unit=10, file=out_path, status="replace", action="write")
    write(10, "(2d30.17)") vv(i), ww(i)
    do j = 1, 12
      write(10, "(d30.17)") n(j)
    end do
    do j = 1, 12
      write(10, "(2d30.17)") dn(j, 1), dn(j, 2)
    end do
    do j = 1, 12
      write(10, "(3d30.17)") ddn(j, 1), ddn(j, 2), ddn(j, 3)
    end do
    close(10)
  end do

end program dump_bspline_oracle
