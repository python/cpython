#
# hexbintree - Recursively descend a directory and
# pack all resource files.
#
# Jack Jansen, CWI, August 1995.
#
# To do:
# - Also do project files (.µ and .¹), after using AppleEvents to the
#   various builders to clean the projects
# - Don't hexbin (and clean) if there exists a .hqx file that is newer.
#

import os
import binhex
import sys
import macostools
import macfs

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('AE')
import aetools
from Metrowerks_Shell_Suite import Metrowerks_Shell_Suite
from Required_Suite import Required_Suite 

class MwShell(aetools.TalkTo, Metrowerks_Shell_Suite, Required_Suite):
	pass

# Top-level directory
TOP=''

# Where to put CW projects, relative to TOP
CWDIR=':Mac:mwerks:projects'

# Helper routines
def binhexit(path, name):
	dstfile = path + '.hqx'
	if os.path.exists(dstfile) and \
			os.stat(dstfile)[8] > os.stat(path)[8]:
		print 'Skip', path,'- Up-to-date'
		return
	print 'Binhexing', path
	binhex.binhex(path, dstfile)
	
# Project files to handle
project_files = {}

def hexbincwprojects(creator):
	"""Compact and hexbin all files remembered with a given creator"""
	print 'Please start project mgr with signature', creator,'-'
	sys.stdin.readline()
	try:
		mgr = MwShell(creator)
	except 'foo':
		print 'Not handled:', creator
		return
	for fss in project_files[creator]:
		srcfile = fss.as_pathname()
		dstfile = srcfile + '.hqx'
		if os.path.exists(dstfile) and \
				os.stat(dstfile)[8] > os.stat(srcfile)[8]:
			print 'Skip', path,'- Up-to-date'
			continue
		print 'Compacting', dstfile
		mgr.open(fss)
		mgr.Reset_File_Paths()
		mgr.Remove_Binaries()
		mgr.Close_Project()
		
		print 'Binhexing', dstfile
		binhex.binhex(srcfile, dstfile)
	mgr.quit()
	
def copycwproject(path, name):
	"""Copy CW project (if needed) and remember for hexbinning"""
	global project_files
	
	dstdir = os.path.join(TOP, CWDIR)
	if not os.path.exists(dstdir):
		print dstdir
		print 'No CW-project dir, skip', name
		return
	dstfile = os.path.join(dstdir, name)
	# Check that we're not in the dest directory
	if dstfile == path:
		return

	# If the destination doesn't exists or is older that the source
	# we copy and remember it
	
	if os.path.exists(dstfile) and \
			os.stat(dstfile)[8] > os.stat(path)[8]:
		print 'Not copying', path,'- Up-to-date'
	else:
		print 'Copy', path
		macostools.copy(path, dstfile)
	
	fss = macfs.FSSpec(dstfile)
	creator = fss.GetCreatorType()[0]
	
	if project_files.has_key(creator):
		project_files[creator].append(fss)
	else:
		project_files[creator] = [fss]	
	

extensions = [
	('.rsrc', binhexit),
	('.µ', copycwproject)
	]

def walker(arg, top, names):
	for n in names:
		for ext, handler in extensions:
			if n[-len(ext):] == ext:
				name = os.path.join(top, n)
				handler(name, n)
				
def dodir(name):
	global TOP, project_files
	TOP = name
	os.path.walk(name, walker, None)
	
	for creator in project_files.keys():
		hexbincwprojects(creator)
	project_files = {}
				
def main():
	if len(sys.argv) > 1:
		for dir in sys.argv[1:]:
			dodir(dir)
	elif os.name == 'mac':
		import macfs
		dir, ok = macfs.GetDirectory('Folder to search:')
		if not ok:
			sys.exit(0)
		dodir(dir.as_pathname())
	else:
		print 'Usage: hexbintree dir ...'
		sys.exit(1)
	if os.name == 'mac':
		sys.exit(1)   # Keep window
	else:
		sys.exit(0)
		
if __name__ == '__main__':
	main()
	
