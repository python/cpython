#
# General parser/loaders for preferences files and such
#
import Res
import macfs
import struct
import MACFS

READ=1
READWRITE=3
Error = "Preferences.Error"

debug = 0

class NullLoader:
	def __init__(self, data=None):
		self.data = data
		
	def load(self):
		if self.data is None:
			raise Error, "No default given"
		return self.data
		
	def save(self, data):
		raise Error, "Cannot save to default value"
		
	def delete(self, deep=0):
		if debug:
			print 'Attempt to delete default value'
		raise Error, "Cannot delete default value"
		
_defaultdefault = NullLoader()
		
class ResLoader:
	def __init__(self, filename, resid, resnum=None, resname=None, default=_defaultdefault):
		self.filename = filename
		self.fss = macfs.FSSpec(self.filename)
		self.resid = resid
		self.resnum = resnum
		self.resname = resname
		self.default = default
		self.data = None
		
	def load(self):
		oldrh = Res.CurResFile()
		try:
			rh = Res.FSpOpenResFile(self.fss, READ)
		except Res.Error:
			self.data = self.default.load()
			return self.data
		try:
			if self.resname:
				handle = Res.Get1NamedResource(self.resid, self.resname)
			else:
				handle = Res.Get1Resource(self.resid, self.resnum)
		except Res.Error:
			self.data = self.default.load()
		else:
			if debug:
				print 'Loaded', (self.resid, self.resnum, self.resname), 'from', self.fss.as_pathname()
			self.data = handle.data
		Res.CloseResFile(rh)
		Res.UseResFile(oldrh)
		return self.data
		
	def save(self, data):
		if self.data is None or self.data != data:
			oldrh = Res.CurResFile()
			rh = Res.FSpOpenResFile(self.fss, READWRITE)
			try:
				handle = Res.Get1Resource(self.resid, self.resnum)
			except Res.Error:
				handle = Res.Resource(data)
				handle.AddResource(self.resid, self.resnum, '')
				if debug:
					print 'Added', (self.resid, self.resnum), 'to', self.fss.as_pathname()
			else:
				handle.data = data
				handle.ChangedResource()
				if debug:
					print 'Changed', (self.resid, self.resnum), 'in', self.fss.as_pathname()
			Res.CloseResFile(rh)
			Res.UseResFile(oldrh)
			
	def delete(self, deep=0):
		if debug:
			print 'Deleting in', self.fss.as_pathname(), `self.data`, deep
		oldrh = Res.CurResFile()
		rh = Res.FSpOpenResFile(self.fss, READWRITE)
		try:
			handle = Res.Get1Resource(self.resid, self.resnum)
		except Res.Error:
			if 	deep:
				if debug: print 'deep in', self.default
				self.default.delete(1)
		else:
			handle.RemoveResource()
			if debug:
				print 'Deleted', (self.resid, self.resnum), 'from', self.fss.as_pathname()
		self.data = None
		Res.CloseResFile(rh)
		Res.UseResFile(oldrh)

class AnyResLoader:
	def __init__(self, resid, resnum=None, resname=None, default=_defaultdefault):
		self.resid = resid
		self.resnum = resnum
		self.resname = resname
		self.default = default
		self.data = None
		
	def load(self):
		try:
			if self.resname:
				handle = Res.GetNamedResource(self.resid, self.resname)
			else:
				handle = Res.GetResource(self.resid, self.resnum)
		except Res.Error:
			self.data = self.default.load()
		else:
			self.data = handle.data
		return self.data
		
	def save(self, data):
		raise Error, "Cannot save AnyResLoader preferences"
					
	def delete(self, deep=0):
		raise Error, "Cannot delete AnyResLoader preferences"
			
class StructLoader:
	def __init__(self, format, loader):
		self.format = format
		self.loader = loader
		
	def load(self):
		data = self.loader.load()
		return struct.unpack(self.format, data)
		
	def save(self, data):
		data = apply(struct.pack, (self.format,)+data)
		self.loader.save(data)
		
	def delete(self, deep=0):
		self.loader.delete(deep)
		
class PstringLoader:
	def __init__(self, loader):
		self.loader = loader
		
	def load(self):
		data = self.loader.load()
		len = ord(data[0])
		return data[1:1+len]
		
	def save(self, data):
		if len(data) > 255:
			raise Error, "String too big for pascal-style"
		self.loader.save(chr(len(data))+data)
		
	def delete(self, deep=0):
		self.loader.delete(deep)
		
class VersionLoader(StructLoader):
	def load(self):
		while 1:
			data = self.loader.load()
			if debug:
				print 'Versionloader:', `data`
			try:
				rv = struct.unpack(self.format, data)
				rv = self.versioncheck(rv)
				return rv
			except (struct.error, Error):
				self.delete(1)
				
	def versioncheck(self, data):
		return data
		
class StrListLoader:
	def __init__(self, loader):
		self.loader = loader
		
	def load(self):
		data = self.loader.load()
		num, = struct.unpack('h', data[:2])
		data = data[2:]
		rv = []
		for i in range(num):
			strlen = ord(data[0])
			if strlen < 0: strlen = strlen + 256
			str = data[1:strlen+1]
			data = data[strlen+1:]
			rv.append(str)
		return rv

	def save(self, list):
		rv = struct.pack('h', len(list))
		for str in list:
			rv = rv + chr(len(str)) + str
		self.loader.save(rv)
		
	def delete(self, deep=0):
		self.loader.delete(deep)

def preferencefile(filename, creator=None, type=None):
	create = creator != None and type != None
	vrefnum, dirid = macfs.FindFolder(MACFS.kOnSystemDisk, 'pref', create)
	fss = macfs.FSSpec((vrefnum, dirid, ":Python:" + filename))
	oldrf = Res.CurResFile()
	if create:
		try:
			rh = Res.FSpOpenResFile(fss, READ)
		except Res.Error:
			Res.FSpCreateResFile(fss, creator, type, MACFS.smAllScripts)
		else:
			Res.CloseResFile(rh)
			Res.UseResFile(oldrf)
	return fss
	
