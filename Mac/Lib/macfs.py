"""macfs - Pure Python module designed to be backward compatible with
macfs and MACFS.
"""
import sys

# First step: ensure we also emulate the MACFS module, which contained
# all the constants

sys.modules['MACFS'] = sys.modules[__name__]

# Import all those constants
from Carbon.Files import *
from Carbon.Folders import *
# Another method:
from Carbon.Folder import FindFolder

# For some obscure historical reason these are here too:
READ = 1
WRITE = 2
smAllScripts = -3


import Carbon.File
# The old name of the error object:
error = Carbon.File.Error

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
		import os
		statb = os.stat(self.as_pathname())
		return statb.st_ctime, statb.st_mtime, 0
	
	def SetDates(self, *dates):
		print "FSSpec.SetDates no longer implemented"
	
class FSRef(Carbon.File.FSRef):
	def as_fsspec(self):
		return FSSpec(self)
	
class Alias(Carbon.File.Alias):

	def GetInfo(self, index):
		return self.GetAliasInfo(index)
		
	def Update(self, *args):
		print "Alias.Update not yet implemented"
		
	def Resolve(self, src=None):
		fss, changed = self.ResolveAlias(src)
		return FSSpec(fss), changed
		
from Carbon.File import FInfo
	
FSSpecType = FSSpec
FSRefType = FSRef
AliasType = Alias
FInfoType = FInfo

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
	
# Finally, install nav services
import macfsn