"""macfs - Pure Python module designed to be backward compatible with
macfs and MACFS.
"""
import sys
import struct
import Carbon.Res
import Carbon.File
import warnings

warnings.warn("macfs is deprecated, use Carbon.File, Carbon.Folder or EasyDialogs",
              DeprecationWarning, stacklevel=2)

# First step: ensure we also emulate the MACFS module, which contained
# all the constants

sys.modules['MACFS'] = sys.modules[__name__]

# Import all those constants
from Carbon.Files import *
from Carbon.Folders import *

# For some obscure historical reason these are here too:
READ = 1
WRITE = 2
smAllScripts = -3

#
# Find the epoch conversion for file dates in a way that works on OS9 and OSX
import time
if time.gmtime(0)[0] == 1970:
    _EPOCHCONVERT = -((1970-1904)*365 + 17) * (24*60*60) + 0x100000000L
    def _utc2time(utc):
        t = utc[1] + _EPOCHCONVERT
        return int(t)
    def _time2utc(t):
        t = int(t) - _EPOCHCONVERT
        if t < -0x7fffffff:
            t = t + 0x10000000L
        return (0, int(t), 0)
else:
    def _utc2time(utc):
        t = utc[1]
        if t < 0:
            t = t + 0x100000000L
        return t
    def _time2utc(t):
        if t > 0x7fffffff:
            t = t - 0x100000000L
        return (0, int(t), 0)

# The old name of the error object:
error = Carbon.File.Error

#
# The various objects macfs used to export. We override them here, because some
# of the method names are subtly different.
#
class FSSpec(Carbon.File.FSSpec):
    def as_fsref(self):
        return FSRef(self)

    def NewAlias(self, src=None):
        return Alias(Carbon.File.NewAlias(src, self))

    def GetCreatorType(self):
        finfo = self.FSpGetFInfo()
        return finfo.Creator, finfo.Type

    def SetCreatorType(self, ctor, tp):
        finfo = self.FSpGetFInfo()
        finfo.Creator = ctor
        finfo.Type = tp
        self.FSpSetFInfo(finfo)

    def GetFInfo(self):
        return self.FSpGetFInfo()

    def SetFInfo(self, info):
        return self.FSpSetFInfo(info)

    def GetDates(self):
        catInfoFlags = kFSCatInfoCreateDate|kFSCatInfoContentMod|kFSCatInfoBackupDate
        catinfo, d1, d2, d3 = FSRef(self).FSGetCatalogInfo(catInfoFlags)
        cdate = catinfo.createDate
        mdate = catinfo.contentModDate
        bdate = catinfo.backupDate
        return _utc2time(cdate), _utc2time(mdate), _utc2time(bdate)

    def SetDates(self, cdate, mdate, bdate):
        catInfoFlags = kFSCatInfoCreateDate|kFSCatInfoContentMod|kFSCatInfoBackupDate
        catinfo = Carbon.File.FSCatalogInfo(
            createDate = _time2utc(cdate),
            contentModDate = _time2utc(mdate),
            backupDate = _time2utc(bdate))
        FSRef(self).FSSetCatalogInfo(catInfoFlags, catinfo)

class FSRef(Carbon.File.FSRef):
    def as_fsspec(self):
        return FSSpec(self)

class Alias(Carbon.File.Alias):

    def GetInfo(self, index):
        return self.GetAliasInfo(index)

    def Update(self, *args):
        pass # print "Alias.Update not yet implemented"

    def Resolve(self, src=None):
        fss, changed = self.ResolveAlias(src)
        return FSSpec(fss), changed

from Carbon.File import FInfo

# Backward-compatible type names:
FSSpecType = FSSpec
FSRefType = FSRef
AliasType = Alias
FInfoType = FInfo

# Global functions:
def ResolveAliasFile(fss, chain=1):
    fss, isdir, isalias = Carbon.File.ResolveAliasFile(fss, chain)
    return FSSpec(fss), isdir, isalias

def RawFSSpec(data):
    return FSSpec(rawdata=data)

def RawAlias(data):
    return Alias(rawdata=data)

def FindApplication(*args):
    raise NotImplementedError, "FindApplication no longer implemented"

def NewAliasMinimalFromFullPath(path):
    return Alias(Carbon.File.NewAliasMinimalFromFullPath(path, '', ''))

# Another global function:
from Carbon.Folder import FindFolder

#
# Finally the old Standard File routine emulators.
#

_curfolder = None

def StandardGetFile(*typelist):
    """Ask for an input file, optionally specifying 4-char file types that are
    allowable"""
    return PromptGetFile('', *typelist)

def PromptGetFile(prompt, *typelist):
    """Ask for an input file giving the user a prompt message. Optionally you can
    specifying 4-char file types that are allowable"""
    import EasyDialogs
    warnings.warn("macfs.StandardGetFile and friends are deprecated, use EasyDialogs.AskFileForOpen",
              DeprecationWarning, stacklevel=2)
    if not typelist:
        typelist = None
    fss = EasyDialogs.AskFileForOpen(message=prompt, wanted=FSSpec,
        typeList=typelist, defaultLocation=_handleSetFolder())
    return fss, not fss is None

def StandardPutFile(prompt, default=None):
    """Ask the user for an output file, with a prompt. Optionally you cn supply a
    default output filename"""
    import EasyDialogs
    warnings.warn("macfs.StandardGetFile and friends are deprecated, use EasyDialogs.AskFileForOpen",
              DeprecationWarning, stacklevel=2)
    fss = EasyDialogs.AskFileForSave(wanted=FSSpec, message=prompt,
    savedFileName=default, defaultLocation=_handleSetFolder())
    return fss, not fss is None

def SetFolder(folder):
    global _curfolder
    warnings.warn("macfs.StandardGetFile and friends are deprecated, use EasyDialogs.AskFileForOpen",
              DeprecationWarning, stacklevel=2)
    if _curfolder:
        rv = FSSpec(_curfolder)
    else:
        rv = None
    _curfolder = folder
    return rv

def _handleSetFolder():
    global _curfolder
    rv = _curfolder
    _curfolder = None
    return rv

def GetDirectory(prompt=None):
    """Ask the user to select a folder. Optionally you can give a prompt."""
    import EasyDialogs
    warnings.warn("macfs.StandardGetFile and friends are deprecated, use EasyDialogs.AskFileForOpen",
              DeprecationWarning, stacklevel=2)
    fss = EasyDialogs.AskFolder(message=prompt, wanted=FSSpec,
        defaultLocation=_handleSetFolder())
    return fss, not fss is None
