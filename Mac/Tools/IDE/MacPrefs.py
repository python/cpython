import macfs
import marshal
import types

from MACFS import kOnSystemDisk

class PrefObject:
	
	def __init__(self, dict = None):
		if dict == None:
			self._prefsdict = {}
		else:
			self._prefsdict = dict
	
	def __len__(self):
		return len(self._prefsdict)
	
	def __delattr__(self, attr):
		if self._prefsdict.has_key(attr):
			del self._prefsdict[attr]
		else:
			raise AttributeError, 'delete non-existing instance attribute'
	
	def __getattr__(self, attr):
		if attr == '__members__':
			keys = self._prefsdict.keys()
			keys.sort()
			return keys
		try:
			return self._prefsdict[attr]
		except KeyError:
			raise AttributeError, attr
	
	def __setattr__(self, attr, value):
		if attr[0] <> '_':
			self._prefsdict[attr] = value
		else:
			self.__dict__[attr] = value
	
	def getprefsdict(self):
		return self._prefsdict


class PrefFile(PrefObject):
	
	def __init__(self, path, creator = 'Pyth'):
		# Find the preferences folder and our prefs file, create if needed.
		self.__path = path
		self.__creator = creator
		self._prefsdict = {}
		try:
			prefdict = marshal.load(open(self.__path, 'rb'))
		except (IOError, ValueError):
			# file not found, or currupt marshal data
			pass
		else:
			for key, value in prefdict.items():
				if type(value) == types.DictType:
					self._prefsdict[key] = PrefObject(value)
				else:
					self._prefsdict[key] = value
	
	def save(self):
		prefdict = {}
		for key, value in self._prefsdict.items():
			if type(value) == types.InstanceType:
				prefdict[key] = value.getprefsdict()
				if not prefdict[key]:
					del prefdict[key]
			else:
				prefdict[key] = value
		marshal.dump(prefdict, open(self.__path, 'wb'))
		fss = macfs.FSSpec(self.__path)
		fss.SetCreatorType(self.__creator, 'pref')
	
	def __getattr__(self, attr):
		if attr == '__members__':
			keys = self._prefsdict.keys()
			keys.sort()
			return keys
		try:
			return self._prefsdict[attr]
		except KeyError:
			if attr[0] <> '_':
				self._prefsdict[attr] = PrefObject()
				return self._prefsdict[attr]
			else:
				raise AttributeError, attr


_prefscache = {}

def GetPrefs(prefname, creator = 'Pyth'):
	import macostools, os
	if _prefscache.has_key(prefname):
		return _prefscache[prefname]
	# Find the preferences folder and our prefs file, create if needed.
	vrefnum, dirid = macfs.FindFolder(kOnSystemDisk, 'pref', 0)
	prefsfolder_fss = macfs.FSSpec((vrefnum, dirid, ''))
	prefsfolder = prefsfolder_fss.as_pathname()
	path = os.path.join(prefsfolder, prefname)
	head, tail = os.path.split(path)
	# make sure the folder(s) exist
	macostools.mkdirs(head)
	
	preffile = PrefFile(path, creator)
	_prefscache[prefname] = preffile
	return preffile
