#
# Turn a pyc file into a resource file containing it in 'PYC ' resource form
from Res import *
import Res
from Resources import *
import os
import macfs
import sys
import py_resource

error = 'mkpycresourcefile.error'

def mkpycresourcefile(src, dst):
	"""Copy pyc file/dir src to resource file dst."""
	
	if not os.path.isdir(src) and src[-4:] <> '.pyc':
			raise error, 'I can only handle .pyc files or directories'
	fsid = py_resource.create(dst)
	if os.path.isdir(src):
		handlesubdir(src)
	else:
		id, name = py_resource.frompycfile(src)
		print 'Wrote %d: %s %s'%(id, name, src)
	CloseResFile(fsid)
			
def handlesubdir(srcdir):
	"""Recursively scan a directory for pyc files and copy to resources"""
	src = os.listdir(srcdir)
	for file in src:
		file = os.path.join(srcdir, file)
		if os.path.isdir(file):
			handlesubdir(file)
		elif file[-4:] == '.pyc':
			id, name = py_resource.frompycfile(file)
			print 'Wrote %d: %s %s'%(id, name, file)
	
if __name__ == '__main__':
	args = sys.argv[1:]
	if not args:
		ifss, ok = macfs.GetDirectory('Select root of tree to pack:')
		if not ok:
			sys.exit(0)
		args = [ifss.as_pathname()]
	for ifn in args:
		ofss, ok = macfs.StandardPutFile('Output for '+os.path.split(ifn)[1])
		if not ok:
			sys.exit(0)
		mkpycresourcefile(ifn, ofss.as_pathname())
	sys.exit(1)			# So we can see something...
