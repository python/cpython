"""Shared AIX support functions."""

import sys
import subprocess
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
"AIX.{}.{:04d}.{}".format(vmtl, builddate, bitsize)
e.g., "AIX.6107.1415.32" for AIX 6.1 TL7 bd 1415, 32-bit
and, "AIX.6107.1415.64" for AIX 6.1 TL7 bd 1415, 64-bit
"""

# _bd - set to impossible setting year98-week98 if None
_tmp = str(get_config_var("AIX_BUILDDATE"))
_bd = 9898 if (_tmp == "None") else int(_tmp)
_bgt = get_config_var("BUILD_GNU_TYPE")
_is_32bit = sys.maxsize == 2147483647


def _aix_tag(vrtl, bd):
    # type: (int, int) -> str
    sz = 32 if _is_32bit else 64
    return "AIX.{}.{:04d}.{}".format(vrtl, bd, sz)


# compute vrtl from the VRMF string
def _aix_vrtl(vrmf):
    # type: (str) -> int
    v, r, tl = vrmf.split(".")[:3]
    return int("{}{}{:02d}".format(v[-1], r, int(tl)))


def _aix_bosmp64():
    # type: () -> List[str]
    """
    The fileset bos.mp64 is the AIX kernel. It's VRMF and builddate
    reflect the current levels of the runtime environment.
    """
    tmp = subprocess.check_output(["/usr/bin/lslpp", "-Lqc", "bos.mp64"])
    tmp = tmp.decode("utf-8").strip().split(":")  # type: ignore
    # lpp, vrmf, bd = list(tmp[index] for index in [0, 2, -1])  # type: ignore
    # e.g., ['bos.mp64', '7.1.4.34', '1806']
    return list(tmp[index] for index in [0, 2, -1])


def aix_platform():
    # type: () -> str
    lpp, vrmf, bd = _aix_bosmp64()
    assert lpp == "bos.mp64", "%s != %s" % (lpp, "bos.mp64")
    return _aix_tag(_aix_vrtl(vrmf), int(bd))


# extract vrtl from the BUILD_GNU_TYPE as an int
def _aix_bgt():
    # type: () -> int
    assert _bgt
    return _aix_vrtl(_bgt)


def aix_buildtag():
    # type: () -> str
    return _aix_tag(_aix_bgt(), _bd)
