#
# Turn a pyc file into a resource file containing it in 'PYC ' resource form
import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('res')
from Res import *
import Res
from Resources import *
import os
import macfs
import sys

READ = 1
WRITE = 2
smAllScripts = -3

error = 'mkpycresourcefile.error'

def Pstring(str):
	if len(str) > 255:
		raise ValueError, 'String too large'
	return chr(len(str))+str
	
def createoutput(dst):
	"""Create output file. Return handle and first id to use."""
	

	FSpCreateResFile(dst, 'PYTH', 'rsrc', smAllScripts)
	output = FSpOpenResFile(dst, WRITE)
	UseResFile(output)
	num = 128
	return output, num
	
def writemodule(name, id, data):
	"""Write pyc code to a PYC resource with given name and id."""
	# XXXX Check that it doesn't exist
	res = Resource(data)
	res.AddResource('PYC ', id, name)
	res.WriteResource()
	res.ReleaseResource()
		
def mkpycresourcefile(src, dst):
	"""Copy pyc file/dir src to resource file dst."""
	
	if not os.path.isdir(src) and src[-4:] <> '.pyc':
			raise error, 'I can only handle .pyc files or directories'
	handle, oid = createoutput(dst)
	if os.path.isdir(src):
		id = handlesubdir(handle, oid, src)
	else:
		id = handleonepycfile(handle, oid, src)
	print 'Wrote',id-oid,'PYC resources to', dst
	CloseResFile(handle)
			
def handleonepycfile(handle, id, file):
	"""Copy one pyc file to the open resource file"""
	d, name = os.path.split(file)
	name = name[:-4]
	print '  module', name
	writemodule(name, id, open(file, 'rb').read())
	return id+1
	
def handlesubdir(handle, id, srcdir):
	"""Recursively scan a directory for pyc files and copy to resources"""
	print 'Directory', srcdir
	src = os.listdir(srcdir)
	for file in src:
		file = os.path.join(srcdir, file)
		if os.path.isdir(file):
			id = handlesubdir(handle, id, file)
		elif file[-4:] == '.pyc':
			id = handleonepycfile(handle, id, file)
	return id
				
	
if __name__ == '__main__':
	args = sys.argv[1:]
	if not args:
		ifss, ok = macfs.StandardGetFile('PYC ')
		if ok:
			args = [ifss.as_pathname()]
		else:
			ifss, ok = macfs.GetDirectory()
			if not ok:
				sys.exit(0)
			args = [ifss.as_pathname()]
	for ifn in args:
		ofss, ok = macfs.StandardPutFile('Output for '+os.path.split(ifn)[1])
		if not ok:
			sys.exit(0)
		mkpycresourcefile(ifn, ofss.as_pathname())
	sys.exit(1)			# So we can see something...
