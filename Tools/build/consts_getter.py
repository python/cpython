from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INTERNAL = ROOT / "Include" / "internal"

def get_nsmallnegints_and_nsmallposints() -> tuple[int, int]:
    nsmallposints = None
    nsmallnegints = None
    with open(INTERNAL / "pycore_runtime_structs.h") as infile:
        for line in infile:
            if line.startswith("#define _PY_NSMALLPOSINTS"):
                nsmallposints = int(line.split()[-1])
            elif line.startswith("#define _PY_NSMALLNEGINTS"):
                nsmallnegints = int(line.split()[-1])
                break
        else:
            raise NotImplementedError
    assert nsmallposints
    assert nsmallnegints
    return nsmallnegints, nsmallposints
