"""macfs - Pure Python module designed to be backward compatible with
macfs and MACFS.
"""
import sys

# First step: ensure we also emulate the MACFS module, which contained
# all the constants

sys.modules['MACFS'] = sys.modules[__name__]

# Import all those constants
import Carbon.Files
from Carbon.Folder import FindFolder

# For some obscure historical reason these are here too:
READ = 1
WRITE = 2
smAllScripts = -3

import Carbon.File

class FSSpec(Carbon.File.FSSpec):
	def as_FSRef(self):
		return FSRef(self)
		
	def NewAlias(self, src=None):
		if src is None:
			src = FSSpec((0,0,''))
		return self.NewAlias(src)
		
	def GetCreatorType(self):
		finfo = self.FSpGetFInfo()
		return finfo[1], finfo[0]
		
	def SetCreatorType(self, ctor, tp):
		finfo = self.FSpGetFInfo()
		finfo = (tp, ctor) + finfo[2:]
		self.FSpSetFInfo(finfo)
		
	def GetFInfo(self):
		return self.FSpGetFInfo()
		
	def SetFInfo(self, info):
		return self.FSpSetFInfo(info)
		
	def GetDates(self):
		raise NotImplementedError, "FSSpec.GetDates no longer implemented"
	
	def SetDates(self, *dates):
		raise NotImplementedError, "FSSpec.SetDates no longer implemented"
	
class FSRef(Carbon.File.FSRef):
	def as_FSSpec(self):
		return FSSpec(self)
	
class Alias(Carbon.File.Alias):

	def Resolve(self, src=None):
		if src is None:
			src = FSSpec((0, 0, ''))
		return self.ResolveAlias(src)
		
	def GetInfo(self, index):
		return self.GetAliasInfo(index)
		
	def Update(self, *args):
		raise NotImplementedError, "Alias.Update not yet implemented"
	
class FInfo:
	pass
	
FSSpecType = FSSpec
FSRefType = FSRef
AliasType = Alias
FInfoType = FInfo

def ResolveAliasFile(fss, chain=1):
	return Carbon.Files.ResolveAliasFile(fss, chain)
	
def RawFSSpec(data):
	return FSSpec(rawdata=data)
	
def RawAlias(data):
	return Alias(rawdata=data)
	
def FindApplication(*args):
	raise NotImplementedError, "FindApplication no longer implemented"
	
def NewAliasMinimalFromFullPath(path):
	return Carbon.Files.NewAliasMinimalFromFullPath(path, '', '')