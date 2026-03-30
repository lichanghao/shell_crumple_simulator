program dump_ghost_coords
  use data_mesh
  implicit none

  interface
    subroutine ghost_nodes(meshh, xx)
      use data_mesh
      implicit real(8) (a - h, o - z)
      implicit integer*4(i - n)
      type(mesh) :: meshh
      real(8) :: xx(3 * (meshh%numnods + meshh%nedge))
    end subroutine ghost_nodes
  end interface

  type(mesh) :: meshh
  character(len=512) :: case_dir
  character(len=512) :: dims_path
  character(len=512) :: mesh_path
  character(len=512) :: config_path
  character(len=512) :: out_path
  real(8), allocatable :: xx(:)
  integer :: i

  call get_command_argument(1, case_dir)
  if (len_trim(case_dir) == 0) stop "usage: dump_ghost_coords <case-dir>"

  dims_path = trim(case_dir) // "/nano_dims.dat"
  mesh_path = trim(case_dir) // "/nano_Mesh.dat"
  config_path = trim(case_dir) // "/nano_config.dat"
  out_path = trim(case_dir) // "/ghost_coords.dat"

  call read_dims(dims_path, meshh%numele, meshh%numnods, meshh%nedge)
  allocate(meshh%nghost_tab(meshh%nedge, 3))
  call read_nghost_tab(mesh_path, meshh%nedge, meshh%nghost_tab)

  allocate(xx(3 * (meshh%numnods + meshh%nedge)))
  xx = 0.0d0
  call read_config_coords(config_path, meshh%numnods, xx)
  call ghost_nodes(meshh, xx)

  open(unit=13, file=out_path, status="replace", action="write")
  do i = 1, meshh%nedge
    write(13, "(3d30.17)") xx(3 * (meshh%numnods + i) - 2:3 * (meshh%numnods + i))
  end do
  close(13)

contains

  subroutine read_dims(path, numele, numnods, nedge)
    character(len=*), intent(in) :: path
    integer, intent(out) :: numele
    integer, intent(out) :: numnods
    integer, intent(out) :: nedge

    open(unit=10, file=path, status="old", action="read")
    call skip_to_label(10, "mesh0%numele")
    read(10, *) numele
    call skip_to_label(10, "mesh0%numnods")
    read(10, *) numnods
    call skip_to_label(10, "mesh0%nedge")
    read(10, *) nedge
    close(10)
  end subroutine read_dims

  subroutine read_nghost_tab(path, nedge, nghost_tab)
    character(len=*), intent(in) :: path
    integer, intent(in) :: nedge
    integer, intent(out) :: nghost_tab(nedge, 3)
    integer :: i

    open(unit=11, file=path, status="old", action="read")
    call skip_to_label(11, "nghost_tab")
    do i = 1, nedge
      read(11, *) nghost_tab(i, 1), nghost_tab(i, 2), nghost_tab(i, 3)
    end do
    close(11)
  end subroutine read_nghost_tab

  subroutine read_config_coords(path, numnods, xx)
    character(len=*), intent(in) :: path
    integer, intent(in) :: numnods
    real(8), intent(inout) :: xx(*)
    integer :: i

    open(unit=12, file=path, status="old", action="read")
    call skip_to_label(12, "Nodal positions")
    do i = 1, numnods
      read(12, *) xx(3 * i - 2), xx(3 * i - 1), xx(3 * i)
    end do
    close(12)
  end subroutine read_config_coords

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

end program dump_ghost_coords
