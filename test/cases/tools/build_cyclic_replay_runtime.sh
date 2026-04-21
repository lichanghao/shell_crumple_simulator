#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <fortran-source-dir> <build-dir>" >&2
  exit 2
fi

src_dir=$1
build_dir=$2

rm -rf "$build_dir"
mkdir -p "$build_dir"
cp -R "$src_dir"/. "$build_dir"/

pasapas="$build_dir/pasapas.f90"

python3 - "$pasapas" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

if "replay_trace_loaded" in text:
    path.write_text(text)
    raise SystemExit(0)

needle = """  REAL(8) :: W(NWORK), Prec(BCs%ndofOP)\n"""
replacement = needle + """  LOGICAL :: replay_trace_loaded\n  REAL(8), ALLOCATABLE :: replay_trace(:)\n"""
if needle not in text:
    raise SystemExit("failed to inject replay trace declarations")
text = text.replace(needle, replacement, 1)

needle = """  PARAMETER(pi0=3.14159265358d0)\n"""
replacement = needle + """\n  replay_trace_loaded = .FALSE.\n  call load_replay_trace('imperfection_trace.dat', BCs%nloadstep, replay_trace, replay_trace_loaded)\n"""
if needle not in text:
    raise SystemExit("failed to inject replay trace initialization")
text = text.replace(needle, replacement, 1)

needle = """    ! Introduce random imperfections\n    call random_seed()\n    if (imperfect .eq. 1) then\n      call random_number(a)\n      do ii = 1, mesh0%numnods\n        x0(3*ii - 2) = x0(3*ii - 2) + mat1%A0*2.*(a - 0.5)*fact_imp\n        x0(3*ii - 1) = x0(3*ii - 1) + mat1%A0*2.*(a - 0.5)*fact_imp\n        x0(3*ii) = x0(3*ii) + mat1%A0*2.*(a - 0.5)*fact_imp\n      end do\n    end if\n"""
replacement = """    ! Introduce imperfections. In replay mode, read the committed scalar for this step;\n    ! otherwise fall back to the canonical random-number path.\n    call random_seed()\n    if (imperfect .eq. 1) then\n      if (replay_trace_loaded) then\n        a = replay_trace(iload)\n      else\n        call random_number(a)\n      end if\n      do ii = 1, mesh0%numnods\n        x0(3*ii - 2) = x0(3*ii - 2) + mat1%A0*2.*(a - 0.5)*fact_imp\n        x0(3*ii - 1) = x0(3*ii - 1) + mat1%A0*2.*(a - 0.5)*fact_imp\n        x0(3*ii) = x0(3*ii) + mat1%A0*2.*(a - 0.5)*fact_imp\n      end do\n    end if\n"""
if needle not in text:
    raise SystemExit("failed to replace imperfection block")
text = text.replace(needle, replacement, 1)

append = """

CONTAINS

  SUBROUTINE load_replay_trace(path, nloadstep, replay_trace, loaded)
    implicit REAL(8) (a - h, o - z)
    implicit INTEGER*4(i - n)
    CHARACTER(len=*), intent(in) :: path
    INTEGER, intent(in) :: nloadstep
    REAL(8), ALLOCATABLE, intent(out) :: replay_trace(:)
    LOGICAL, intent(out) :: loaded
    INTEGER :: istat, i

    loaded = .FALSE.
    ALLOCATE(replay_trace(max(1, nloadstep)))
    replay_trace = 0.d0

    open(unit=201, file=path, status='old', action='read', iostat=istat)
    if (istat .ne. 0) return

    do i = 1, nloadstep
      read(201, *, iostat=istat) replay_trace(i)
      if (istat .ne. 0) then
        close(201)
        STOP 'replay trace shorter than nloadstep'
      end if
    end do
    close(201)
    loaded = .TRUE.
  END SUBROUTINE load_replay_trace
"""

if "END SUBROUTINE pasapas" not in text:
    raise SystemExit("failed to find end of pasapas")
text = text.replace("END SUBROUTINE pasapas", append + "\nEND SUBROUTINE pasapas", 1)
path.write_text(text)
PY

cd "$build_dir"
rm -f *.o *.mod crunch_it_replay
mpif77 -w -O3 -fallow-argument-mismatch -c headers.f90
mpif77 -w -O3 -fallow-argument-mismatch -c *.f90
mpif77 -w -O3 -fallow-argument-mismatch -c *.f
mpif77 -O3 -fallow-argument-mismatch -o crunch_it_replay *.o
echo "$build_dir/crunch_it_replay"
