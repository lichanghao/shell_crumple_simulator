program dump_constitutive_oracle
  use data_mat
  use data_vector2
  use data_vector3
  implicit none

  interface
    subroutine Brenner(mat1, pe, W, dW)
      use data_mat
      type(material) :: mat1
      real(8) :: pe(6)
      real(8) :: W
      real(8) :: dW(6)
    end subroutine Brenner

    subroutine Inner_Brenner(mat1, pe, dpedeta, ddpedeta, W, dWdeta, ddWdeta, dW)
      use data_mat
      use data_vector2
      use data_vector3
      type(material) :: mat1
      real(8) :: pe(6)
      type(vector2) :: dpedeta(6)
      type(vector3) :: ddpedeta(6)
      real(8) :: W
      real(8) :: dWdeta(2)
      real(8) :: ddWdeta(3)
      real(8) :: dW(6)
    end subroutine Inner_Brenner

    subroutine Hyper_pot_inner(C_elem, curvppal, vppal, mat1, eta, W, dWdeta, ddWdeta, dW)
      use data_mat
      type(material) :: mat1
      real(8) :: C_elem(3)
      real(8) :: curvppal(2)
      real(8) :: vppal(2, 2)
      real(8) :: eta(2)
      real(8) :: W
      real(8) :: dWdeta(2)
      real(8) :: ddWdeta(3)
      real(8) :: dW(6)
    end subroutine Hyper_pot_inner

    subroutine newton_inner(C_elem, curvppal, vppal, mat1, x, W, dWdp, crit, n, maxn, fail_mode)
      use data_mat
      type(material) :: mat1
      real(8) :: C_elem(3)
      real(8) :: curvppal(2)
      real(8) :: vppal(2, 2)
      real(8) :: x(2)
      real(8) :: W
      real(8) :: dWdp(6)
      real(8) :: crit
      integer :: n
      integer :: maxn
      integer :: fail_mode
    end subroutine newton_inner
  end interface

  integer, parameter :: nbrenner = 10
  integer, parameter :: nnewton = 4
  character(len=512) :: out_dir
  character(len=512) :: brenner_dir
  character(len=512) :: newton_dir
  character(len=512) :: out_path
  integer :: i
  integer :: j
  integer :: niter
  integer :: fail_mode

  type(material) :: mat1
  type(vector2) :: dpedeta(6)
  type(vector3) :: ddpedeta(6)
  real(8) :: pe(6)
  real(8) :: W
  real(8) :: dW(6)
  real(8) :: dWdeta(2)
  real(8) :: ddWdeta(3)
  real(8) :: eta(2)
  real(8) :: crit
  integer :: maxn

  real(8), dimension(nbrenner, 6) :: brenner_cases
  real(8), dimension(nnewton, 3) :: C_cases
  real(8), dimension(nnewton, 2) :: curv_cases
  real(8), dimension(nnewton, 2) :: eta_cases
  real(8), dimension(nnewton, 2) :: crit_maxn
  real(8), dimension(nnewton, 2, 2) :: vppal_cases

  call get_command_argument(1, out_dir)
  if (len_trim(out_dir) == 0) stop "usage: dump_constitutive_oracle <out-dir>"

  brenner_dir = trim(out_dir) // "/brenner"
  newton_dir = trim(out_dir) // "/newton_inner"
  call execute_command_line("mkdir -p " // trim(brenner_dir))
  call execute_command_line("mkdir -p " // trim(newton_dir))

  call init_material(mat1)
  call init_brenner_cases(brenner_cases)
  call init_newton_cases(C_cases, curv_cases, vppal_cases, eta_cases, crit_maxn)

  do i = 1, nbrenner
    pe = brenner_cases(i, :)
    call fill_identity_eta_derivs(dpedeta, ddpedeta)
    call Inner_Brenner(mat1, pe, dpedeta, ddpedeta, W, dWdeta, ddWdeta, dW)

    write(out_path, "(A,'/case_',I2.2,'.dat')") trim(brenner_dir), i
    open(unit=10, file=out_path, status="replace", action="write")
    write(10, "(6ES30.17)") pe
    write(10, "(ES30.17)") W
    write(10, "(6ES30.17)") dW
    do j = 1, 6
      call write_inner_brenner_hessian_row(10, mat1, pe, j)
    end do
    close(10)
  end do

  do i = 1, nnewton
    eta = eta_cases(i, :)
    crit = crit_maxn(i, 1)
    maxn = nint(crit_maxn(i, 2))

    call newton_inner(C_cases(i, :), curv_cases(i, :), vppal_cases(i, :, :), &
                      mat1, eta, W, dW, crit, niter, maxn, fail_mode)
    call Hyper_pot_inner(C_cases(i, :), curv_cases(i, :), vppal_cases(i, :, :), &
                         mat1, eta, W, dWdeta, ddWdeta, dW)

    write(out_path, "(A,'/case_',I2.2,'.dat')") trim(newton_dir), i
    open(unit=11, file=out_path, status="replace", action="write")
    write(11, "(3ES30.17)") C_cases(i, :)
    write(11, "(2ES30.17)") curv_cases(i, :)
    write(11, "(2ES30.17)") vppal_cases(i, 1, :)
    write(11, "(2ES30.17)") vppal_cases(i, 2, :)
    write(11, "(2ES30.17)") eta_cases(i, :)
    write(11, "(ES30.17,1X,I0)") crit, maxn
    write(11, "(I0,1X,I0)") niter, fail_mode
    write(11, "(2ES30.17)") eta
    write(11, "(ES30.17)") W
    write(11, "(2ES30.17)") dWdeta
    write(11, "(3ES30.17)") ddWdeta
    write(11, "(6ES30.17)") dW
    close(11)
  end do

contains

  subroutine init_material(mat1)
    use data_mat
    type(material), intent(out) :: mat1
    real(8), parameter :: pi = 3.141592653589793238d0

    mat1%nCode_Pot = 2
    mat1%A0 = 0.142d0
    mat1%A1 = 0.142d0
    mat1%Vs = [0.60310500860214233d0, 26.25d0, 0.9d0]
    mat1%Va = [0.75400000810623169d0, 0.149d0, 0.25d0]
    mat1%s0 = 3.d0*sqrt(3.d0)/2.d0*mat1%A0*mat1%A0
    mat1%E(1, :) = [cos(pi/6.d0), sin(pi/6.d0)]
    mat1%E(2, :) = [cos(5.d0*pi/6.d0), sin(5.d0*pi/6.d0)]
    mat1%E(3, :) = [0.d0, -1.d0]
  end subroutine init_material

  subroutine init_brenner_cases(cases)
    real(8), intent(out) :: cases(nbrenner, 6)

    cases(1, :) = [0.142d0, 0.142d0, 0.142d0, 2.0943951023931953d0, 2.0943951023931953d0, 2.0943951023931953d0]
    cases(2, :) = [0.1415d0, 0.1425d0, 0.1435d0, 2.05d0, 2.10d0, 2.15d0]
    cases(3, :) = [0.139d0, 0.144d0, 0.147d0, 2.00d0, 2.06d0, 2.12d0]
    cases(4, :) = [0.145d0, 0.145d0, 0.141d0, 2.20d0, 2.12d0, 2.04d0]
    cases(5, :) = [0.140d0, 0.146d0, 0.143d0, 1.98d0, 2.08d0, 2.18d0]
    cases(6, :) = [0.148d0, 0.144d0, 0.142d0, 2.22d0, 2.18d0, 2.02d0]
    cases(7, :) = [0.137d0, 0.141d0, 0.145d0, 1.95d0, 2.05d0, 2.25d0]
    cases(8, :) = [0.150d0, 0.147d0, 0.143d0, 2.30d0, 2.10d0, 2.00d0]
    cases(9, :) = [0.143d0, 0.140d0, 0.138d0, 2.18d0, 2.12d0, 2.06d0]
    cases(10, :) = [0.146d0, 0.142d0, 0.139d0, 2.08d0, 1.98d0, 2.28d0]
  end subroutine init_brenner_cases

  subroutine init_newton_cases(C_cases, curv_cases, vppal_cases, eta_cases, crit_maxn)
    real(8), intent(out) :: C_cases(nnewton, 3)
    real(8), intent(out) :: curv_cases(nnewton, 2)
    real(8), intent(out) :: vppal_cases(nnewton, 2, 2)
    real(8), intent(out) :: eta_cases(nnewton, 2)
    real(8), intent(out) :: crit_maxn(nnewton, 2)

    C_cases(1, :) = [1.d0, 1.d0, 0.d0]
    curv_cases(1, :) = [0.d0, 0.d0]
    vppal_cases(1, :, :) = reshape([1.d0, 0.d0, 0.d0, 1.d0], [2, 2])
    eta_cases(1, :) = [0.d0, 0.d0]
    crit_maxn(1, :) = [1.d-8, 12.d0]

    C_cases(2, :) = [1.04d0, 0.97d0, 0.d0]
    curv_cases(2, :) = [0.04d0, -0.03d0]
    vppal_cases(2, :, :) = reshape([1.d0, 0.d0, 0.d0, 1.d0], [2, 2])
    eta_cases(2, :) = [0.003d0, -0.002d0]
    crit_maxn(2, :) = [1.d-8, 20.d0]

    C_cases(3, :) = [0.96d0, 1.03d0, 0.d0]
    curv_cases(3, :) = [0.08d0, 0.02d0]
    vppal_cases(3, :, :) = reshape([0.9659258262890683d0, -0.2588190451025207d0, &
                                    0.2588190451025207d0, 0.9659258262890683d0], [2, 2])
    eta_cases(3, :) = [-0.004d0, 0.002d0]
    crit_maxn(3, :) = [1.d-8, 25.d0]

    C_cases(4, :) = [1.10d0, 0.92d0, 0.d0]
    curv_cases(4, :) = [0.05d0, -0.01d0]
    vppal_cases(4, :, :) = reshape([1.d0, 0.d0, 0.d0, 1.d0], [2, 2])
    eta_cases(4, :) = [0.020d0, -0.015d0]
    crit_maxn(4, :) = [1.d-12, 0.d0]
  end subroutine init_newton_cases

  subroutine fill_identity_eta_derivs(dpedeta, ddpedeta)
    use data_vector2
    use data_vector3
    type(vector2), intent(out) :: dpedeta(6)
    type(vector3), intent(out) :: ddpedeta(6)
    integer :: i

    do i = 1, 6
      dpedeta(i)%val = [0.d0, 0.d0]
      ddpedeta(i)%val = [0.d0, 0.d0, 0.d0]
    end do
    dpedeta(1)%val = [1.d0, 0.d0]
    dpedeta(2)%val = [0.d0, 1.d0]
  end subroutine fill_identity_eta_derivs

  subroutine write_inner_brenner_hessian_row(unit_no, mat1, pe, row_index)
    use data_mat
    use data_vector2
    use data_vector3
    integer, intent(in) :: unit_no
    integer, intent(in) :: row_index
    type(material), intent(in) :: mat1
    real(8), intent(in) :: pe(6)
    type(vector2) :: dpedeta(6)
    type(vector3) :: ddpedeta(6)
    real(8) :: W0
    real(8) :: W1
    real(8) :: W2
    real(8) :: dWdeta(2)
    real(8) :: ddWdeta(3)
    real(8) :: dW0(6)
    real(8) :: dW1(6)
    real(8) :: dW2(6)
    real(8) :: pe_plus(6)
    real(8) :: pe_minus(6)
    real(8), parameter :: h = 1.d-7
    integer :: j

    call fill_identity_eta_derivs(dpedeta, ddpedeta)
    pe_plus = pe
    pe_minus = pe
    pe_plus(row_index) = pe_plus(row_index) + h
    pe_minus(row_index) = pe_minus(row_index) - h
    call Inner_Brenner(mat1, pe_plus, dpedeta, ddpedeta, W1, dWdeta, ddWdeta, dW1)
    call Inner_Brenner(mat1, pe_minus, dpedeta, ddpedeta, W2, dWdeta, ddWdeta, dW2)
    call Inner_Brenner(mat1, pe, dpedeta, ddpedeta, W0, dWdeta, ddWdeta, dW0)
    write(unit_no, "(6ES30.17)") ((dW1(j) - dW2(j))/(2.d0*h), j = 1, 6)
  end subroutine write_inner_brenner_hessian_row

end program dump_constitutive_oracle
