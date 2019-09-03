"""Shared AIX support functions."""

import sys
from subprocess import Popen, PIPE, DEVNULL


_is_32bit = sys.maxsize == 2147483647

def aix_pep425():
    """
    AIX filesets are identified by four decimal values aka VRMF.
    V (version) is the value returned by "uname -v"
    R (release) is the value returned by "uname -r"
    M and F values are not available via uname
    There is a fifth, lessor known value: builddate that
    is expressed as YYWW (Year WeekofYear)

    The fileset bos.mp64 contains the AIX kernel and it's
    VRMF and builddate are equivalent to installed
    levels of the runtime environment.
    The program lslpp is used to get these values.
    The pep425 platform tag for AIX becomes:
    AIX.VRTL.YYWW.SZ, e.g., AIX.6107.1415.32
    """

    p = Popen(["/usr/bin/lslpp", "-Lqc", "bos.mp64"],
              universal_newlines=True, stdout=PIPE, stderr=DEVNULL)
    _lslppLqc = p.stdout.read().strip().split(":")
    p.wait()

    (lpp, vrmf, bd) = list(_lslppLqc[index] for index in [0, 2, -1])
    assert lpp == "bos.mp64", "%s != %s" % (lpp, "bos.mp64")
    v, r, tl = vrmf.split(".")[:3]
    sz = 32 if _is_32bit else 64
    return "AIX.{}{}{:02d}.{}.{}".format(v,r,int(tl),bd,sz)


def get_platform():
    """Return AIX platform tag"""
    return aix_pep425()
