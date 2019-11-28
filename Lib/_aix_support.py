"""Shared AIX support functions."""

import subprocess
import sys
from sysconfig import get_config_var

"""
AIX filesets are identified by four decimal values: V.R.M.F.
V (version) and R (release) can be retreived using ``uname``
M(aintenance) and F(ix) values are not available via uname
For determining ABI compatibility the values V.R.M
are essential - suplemented by a final value known as
`builddate` that is expressed as YYWW (Year WeekofYear)

The values for V.R.M of the system Python was built on are available in
the variable sysconfig.get_config_var("BUILD_GNU_TYPE"). The system builddate
value may be available in sys.config.get_config_var("AIX_BUILDDATE")
When all of these values on a running system are greater or equal to the
values of the build system then the bdist modules are binary compatible
with the AIX kernel.

For pep425 purposes the AIX platform tag becomes:
"AIX-{:1x}{:1d}{:02d}-{:04d}-{}".format(v, r, tl, builddate, bitsize)
e.g., "AIX-6107-1415-32" for AIX 6.1 TL7 bd 1415, 32-bit
and, "AIX-6107-1415-64" for AIX 6.1 TL7 bd 1415, 64-bit
"""

# _bd - set to impossible setting year98-week98 if None
_tmp = str(get_config_var("AIX_BUILDDATE"))
_bd = 9898 if (_tmp == "None") else int(_tmp)
_bgt = get_config_var("BUILD_GNU_TYPE")
_is_32bit = sys.maxsize == 2147483647


def _aix_tag(v, bd):
    # type: (List[int], int) -> str
    # v is used as variable name so line below passes pep8 length test
    # v[version, release, technology_level]
    sz = 32 if _is_32bit else 64
    return "AIX-{:1x}{:1d}{:02d}-{:04d}-{}".format(v[0], v[1], v[2], bd, sz)


# compute vrtl from the VRMF string
def _aix_vrtl(vrmf):
    # type: (str) -> List[int]
    v, r, tl = vrmf.split(".")[:3]
    return [int(v[-1]), int(r), int(tl)]


def _aix_bosmp64():
    # type: () -> Tuple[str, int]
    """
    The fileset bos.mp64 is the AIX kernel. It's VRMF and builddate
    reflect the current levels of the runtime environment.
    """
    out = subprocess.check_output(["/usr/bin/lslpp", "-Lqc", "bos.mp64"])
    out = out.decode("utf-8").strip().split(":")  # type: ignore
    # vrmf, bd = list(out[index] for index in [2, -1])  # type: ignore
    # e.g., ['7.1.4.34', '1806']
    # Use str() and int() to help mypy see types
    return str(out[2]), int(out[-1])


def aix_platform():
    # type: () -> str
    vrmf, bd = _aix_bosmp64()
    return _aix_tag(_aix_vrtl(vrmf), bd)


# extract vrtl from the BUILD_GNU_TYPE as an int
def _aix_bgt():
    # type: () -> List[int]
    assert _bgt
    return _aix_vrtl(_bgt)


def aix_buildtag():
    # type: () -> str
    return _aix_tag(_aix_bgt(), _bd)
