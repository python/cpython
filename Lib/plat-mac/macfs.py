"""macfs - Pure Python module designed to be backward compatible with
macfs and MACFS.
"""
import sys
import struct
import Carbon.Res
import Carbon.File
import Nav

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
		t = t - _EPOCHCONVERT
		if t < -0x7fffffff:
			t = t + 0x10000000L
		return (0, int(t), 0)
else:
	def _utc2time(utc): return utc[1]
	def _time2utc(t): return (0, t, 0)

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

_movablemodal = 0
_curfolder = None

def _mktypelist(typelist):
	# Workaround for OSX typeless files:
	if 'TEXT' in typelist and not '\0\0\0\0' in typelist:
		typelist = typelist + ('\0\0\0\0',)
	if not typelist:
		return None
	data = 'Pyth' + struct.pack("hh", 0, len(typelist))
	for type in typelist:
		data = data+type
	return Carbon.Res.Handle(data)
	
def StandardGetFile(*typelist):
	"""Ask for an input file, optionally specifying 4-char file types that are
	allowable"""
	return apply(PromptGetFile, (None,)+typelist)
	
def PromptGetFile(prompt, *typelist):
	"""Ask for an input file giving the user a prompt message. Optionally you can
	specifying 4-char file types that are allowable"""
	args = {}
	flags = 0x56
	typehandle = _mktypelist(typelist)
	if typehandle:
		args['typeList'] = typehandle
	else:
		flags = flags | 0x01
	if prompt:
		args['message'] = prompt
	args['preferenceKey'] = 'PyMC'
	if _movablemodal:
		args['eventProc'] = None
	args['dialogOptionFlags'] = flags
	_handleSetFolder(args)
	try:
		rr = Nav.NavChooseFile(args)
		good = 1
	except Nav.error, arg:
		if arg[0] != -128: # userCancelledErr
			raise Nav.error, arg
		good = 0
		fss = None
	else:
		if rr.selection:
			fss = FSSpec(rr.selection[0])
		else:
			fss = None
			good = 0
##	if typehandle:
##		typehandle.DisposeHandle()
	return fss, good

def StandardPutFile(prompt, default=None):
	"""Ask the user for an output file, with a prompt. Optionally you cn supply a
	default output filename"""
	args = {}
	flags = 0x07
	if prompt:
		args['message'] = prompt
	args['preferenceKey'] = 'PyMC'
	if _movablemodal:
		args['eventProc'] = None
	if default:
		args['savedFileName'] = default
	args['dialogOptionFlags'] = flags
	_handleSetFolder(args)
	try:
		rr = Nav.NavPutFile(args)
		good = 1
	except Nav.error, arg:
		if arg[0] != -128: # userCancelledErr
			raise Nav.error, arg
		good = 0
		fss = None
	else:
		fss = FSSpec(rr.selection[0])
	return fss, good
	
def SetFolder(folder):
	global _curfolder
	if _curfolder:
		rv = _curfolder
	else:
		rv = None
	_curfolder = FSSpec(folder)
	return rv
	
def _handleSetFolder(args):
	global _curfolder
	if not _curfolder:
		return
	import aepack
	fss = _curfolder
	aedesc = aepack.pack(fss)
	args['defaultLocation'] = aedesc
	_curfolder = None
	
def GetDirectory(prompt=None):
	"""Ask the user to select a folder. Optionally you can give a prompt."""
	args = {}
	flags = 0x17
	if prompt:
		args['message'] = prompt
	args['preferenceKey'] = 'PyMC'
	if _movablemodal:
		args['eventProc'] = None
	args['dialogOptionFlags'] = flags
	_handleSetFolder(args)
	try:
		rr = Nav.NavChooseFolder(args)
		good = 1
	except Nav.error, arg:
		if arg[0] != -128: # userCancelledErr
			raise Nav.error, arg
		good = 0
		fss = None
	else:
		fss = FSSpec(rr.selection[0])
	return fss, good
	
