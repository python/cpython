"""Creation of PYC resources"""
import os
import Res
import __builtin__

READ = 1
WRITE = 2
smAllScripts = -3

def Pstring(str):
	"""Return a pascal-style string from a python-style string"""
	if len(str) > 255:
		raise ValueError, 'String too large'
	return chr(len(str))+str
	
def create(dst, creator='Pyth'):
	"""Create output file. Return handle and first id to use."""
	
	try:
		os.unlink(dst)
	except os.error:
		pass
	Res.FSpCreateResFile(dst, creator, 'rsrc', smAllScripts)
	return open(dst)
	
def open(dst):
	output = Res.FSpOpenResFile(dst, WRITE)
	Res.UseResFile(output)
	return output

def writemodule(name, id, data, type='PYC ', preload=0, ispackage=0):
	"""Write pyc code to a PYC resource with given name and id."""
	# XXXX Check that it doesn't exist
	
	# Normally, byte 4-7 are the time stamp, but that is not used
	# for 'PYC ' resources. We abuse byte 4 as a flag to indicate
	# that it is a package rather than an ordinary module. 
	# See also macimport.c. (jvr)
	if ispackage:
		data = data[:4] + '\377\0\0\0' + data[8:] # flag resource as package
	else:
		data = data[:4] + '\0\0\0\0' + data[8:] # clear mod date field, used as package flag
	res = Res.Resource(data)
	res.AddResource(type, id, name)
	if preload:
		attrs = res.GetResAttrs()
		attrs = attrs | 0x04
		res.SetResAttrs(attrs)
	res.WriteResource()
	res.ReleaseResource()
		
def frompycfile(file, name=None, preload=0, ispackage=0):
	"""Copy one pyc file to the open resource file"""
	if name == None:
		d, name = os.path.split(file)
		name = name[:-4]
	id = findfreeid()
	data = __builtin__.open(file, 'rb').read()
	writemodule(name, id, data, preload=preload, ispackage=ispackage)
	return id, name

def frompyfile(file, name=None, preload=0, ispackage=0):
	"""Compile python source file to pyc file and add to resource file"""
	import py_compile
	
	py_compile.compile(file)
	file = file +'c'
	return frompycfile(file, name, preload=preload, ispackage=ispackage)

# XXXX Note this is incorrect, it only handles one type and one file....

_firstfreeid = None

def findfreeid(type='PYC '):
	"""Find next free id-number for given resource type"""
	global _firstfreeid
	
	if _firstfreeid == None:
		Res.SetResLoad(0)
		highest = 511
		num = Res.Count1Resources(type)
		for i in range(1, num+1):
			r = Res.Get1IndResource(type, i)
			id, d1, d2 = r.GetResInfo()
			highest = max(highest, id)
		Res.SetResLoad(1)
		_firstfreeid = highest+1
	id = _firstfreeid
	_firstfreeid = _firstfreeid+1
	return id
