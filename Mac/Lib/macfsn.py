"""StandardFile compatability module: implement macfs StandardFile
API calls with Navigation Services"""
import macfs
import struct
import Res
try:
	import Nav
except ImportError:
	Nav = None

_curfolder = None
_movablemodal = 1

def _mktypelist(typelist):
	if not typelist:
		return None
	data = 'Pyth' + struct.pack("hh", 0, len(typelist))
	for type in typelist:
		data = data+type
	return Res.Handle(data)
	
def _StandardGetFile(*typelist):
	return apply(_PromptGetFile, (None,)+typelist)
	
def _PromptGetFile(prompt, *typelist):
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
		fss = macfs.FSSpec(':cancelled')
	else:
		fss = rr.selection[0]
##	if typehandle:
##		typehandle.DisposeHandle()
	return fss, good

def _StandardPutFile(prompt, default=None):
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
		fss = macfs.FSSpec(':cancelled')
	else:
		fss = rr.selection[0]
	return fss, good
	
def _SetFolder(folder):
	global _curfolder
	if _curfolder:
		rv = _curfolder
	else:
		rv = None
	_curfolder = macfs.FSSpec(folder)
	return rv
	
def _handleSetFolder(args):
	global _curfolder
	if not _curfolder:
		return
	import aepack
	fss = macfs.FSSpec(_curfolder)
	aedesc = aepack.pack(fss)
	args['defaultLocation'] = aedesc
	_curfolder = None
	
def _GetDirectory(prompt=None):
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
		fss = macfs.FSSpec(':cancelled')
	else:
		fss = rr.selection[0]
	return fss, good
	
def _install():
	macfs.StandardGetFile = StandardGetFile
	macfs.PromptGetFile = PromptGetFile
	macfs.StandardPutFile = StandardPutFile
	macfs.SetFolder = SetFolder
	macfs.GetDirectory = GetDirectory
	
if Nav and Nav.NavServicesAvailable():
	StandardGetFile = _StandardGetFile
	PromptGetFile = _PromptGetFile
	StandardPutFile = _StandardPutFile
	SetFolder = _SetFolder
	GetDirectory = _GetDirectory
	_install()
else:
	from macfs import StandardGetFile, PromptGetFile, StandardPutFile, SetFolder, GetDirectory
	

if __name__ == '__main__':
	print 'Testing StandardGetFile'
	fss, ok = StandardGetFile()
	print '->', fss, ok
	print 'Testing StandardGetFile("TEXT")'
	fss, ok = StandardGetFile("TEXT")
	print '->', fss, ok
	print 'Testing PromptGetFile'
	fss, ok = PromptGetFile("prompt")
	print '->', fss, ok
	print 'Testing StandardPutFile("the prompt", "default")'
	fss, ok = StandardPutFile("the prompt", "default")
	print '->', fss, ok
	print 'Testing GetDirectory("another prompt")'
	fss, ok = GetDirectory("Another prompt")
	print '->', fss, ok
	import sys
	sys.exit(1)
	
