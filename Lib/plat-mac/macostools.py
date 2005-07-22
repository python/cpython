"""macostools - Various utility functions for MacOS.

mkalias(src, dst) - Create a finder alias 'dst' pointing to 'src'
copy(src, dst) - Full copy of 'src' to 'dst'
"""

from Carbon import Res
from Carbon import File, Files
import os
import sys
import MacOS
import time
try:
    openrf = MacOS.openrf
except AttributeError:
    # Backward compatibility
    openrf = open

Error = 'macostools.Error'

BUFSIZ=0x80000      # Copy in 0.5Mb chunks

COPY_FLAGS = (Files.kIsStationary|Files.kNameLocked|Files.kHasBundle|
              Files.kIsInvisible|Files.kIsAlias)

#
# Not guaranteed to be correct or stay correct (Apple doesn't tell you
# how to do this), but it seems to work.
#
def mkalias(src, dst, relative=None):
    """Create a finder alias"""
    srcfsr = File.FSRef(src)
    # The next line will fail under unix-Python if the destination
    # doesn't exist yet. We should change this code to be fsref-based.
    dstdir, dstname = os.path.split(dst)
    if not dstdir: dstdir = os.curdir
    dstdirfsr = File.FSRef(dstdir)
    if relative:
        relativefsr = File.FSRef(relative)
        # ik mag er geen None in stoppen :-(
        alias = File.FSNewAlias(relativefsr, srcfsr)
    else:
        alias = srcfsr.FSNewAliasMinimal()

    dstfsr, dstfss = Res.FSCreateResourceFile(dstdirfsr, unicode(dstname),
        File.FSGetResourceForkName())
    h = Res.FSOpenResourceFile(dstfsr, File.FSGetResourceForkName(), 3)
    resource = Res.Resource(alias.data)
    resource.AddResource('alis', 0, '')
    Res.CloseResFile(h)

    dstfinfo = dstfss.FSpGetFInfo()
    dstfinfo.Flags = dstfinfo.Flags|0x8000    # Alias flag
    dstfss.FSpSetFInfo(dstfinfo)

def mkdirs(dst):
    """Make directories leading to 'dst' if they don't exist yet"""
    if dst == '' or os.path.exists(dst):
        return
    head, tail = os.path.split(dst)
    if os.sep == ':' and not ':' in head:
        head = head + ':'
    mkdirs(head)
    os.mkdir(dst, 0777)

def touched(dst):
    """Tell the finder a file has changed. No-op on MacOSX."""
    if sys.platform != 'mac': return
    import warnings
    warnings.filterwarnings("ignore", "macfs.*", DeprecationWarning, __name__)
    import macfs
    file_fss = macfs.FSSpec(dst)
    vRefNum, dirID, name = file_fss.as_tuple()
    dir_fss = macfs.FSSpec((vRefNum, dirID, ''))
    crdate, moddate, bkdate = dir_fss.GetDates()
    now = time.time()
    if now == moddate:
        now = now + 1
    try:
        dir_fss.SetDates(crdate, now, bkdate)
    except macfs.error:
        pass

def touched_ae(dst):
    """Tell the finder a file has changed"""
    pardir = os.path.split(dst)[0]
    if not pardir:
        pardir = os.curdir
    import Finder
    f = Finder.Finder()
    f.update(File.FSRef(pardir))

def copy(src, dst, createpath=0, copydates=1, forcetype=None):
    """Copy a file, including finder info, resource fork, etc"""
    src = File.pathname(src)
    dst = File.pathname(dst)
    if createpath:
        mkdirs(os.path.split(dst)[0])

    ifp = open(src, 'rb')
    ofp = open(dst, 'wb')
    d = ifp.read(BUFSIZ)
    while d:
        ofp.write(d)
        d = ifp.read(BUFSIZ)
    ifp.close()
    ofp.close()

    ifp = openrf(src, '*rb')
    ofp = openrf(dst, '*wb')
    d = ifp.read(BUFSIZ)
    while d:
        ofp.write(d)
        d = ifp.read(BUFSIZ)
    ifp.close()
    ofp.close()

    srcfss = File.FSSpec(src)
    dstfss = File.FSSpec(dst)
    sf = srcfss.FSpGetFInfo()
    df = dstfss.FSpGetFInfo()
    df.Creator, df.Type = sf.Creator, sf.Type
    if forcetype != None:
        df.Type = forcetype
    df.Flags = (sf.Flags & COPY_FLAGS)
    dstfss.FSpSetFInfo(df)
    if copydates:
        srcfsr = File.FSRef(src)
        dstfsr = File.FSRef(dst)
        catinfo, _, _, _ = srcfsr.FSGetCatalogInfo(Files.kFSCatInfoAllDates)
        dstfsr.FSSetCatalogInfo(Files.kFSCatInfoAllDates, catinfo)
    touched(dstfss)

def copytree(src, dst, copydates=1):
    """Copy a complete file tree to a new destination"""
    if os.path.isdir(src):
        mkdirs(dst)
        files = os.listdir(src)
        for f in files:
            copytree(os.path.join(src, f), os.path.join(dst, f), copydates)
    else:
        copy(src, dst, 1, copydates)
