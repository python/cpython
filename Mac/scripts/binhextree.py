#
# binhextree - Recursively descend a directory and
# pack all resource files.
#
# Actually it doesn't binhex anymore, it only copies projects.
#
# Jack Jansen, CWI, August 1995.
#

import os
import binhex
import sys
import macostools
import macfs

import aetools
from Metrowerks_Shell_Suite import Metrowerks_Shell_Suite
from Required_Suite import Required_Suite 

class MwShell(aetools.TalkTo, Metrowerks_Shell_Suite, Required_Suite):
	pass

# Top-level directory
TOP=''

# Where to put CW projects, relative to TOP
CWDIR=':Mac:mwerks:projects'
# From which folders to put projects there
CWDIRDIRS=['build.mac', 'build.macstand', 'build.macfreeze', 'PlugIns']

# Helper routines
def binhexit(path, name):
	dstfile = path + '.hqx'
	if os.path.exists(dstfile):
		print 'Compare', path,'...',
		if binhexcompare(path, dstfile):
			print 'Identical, skipped.'
			return
		else:
			print 'Not up-to-date.'
	print 'Binhexing', path
	binhex.binhex(path, dstfile)
	
def binhexcompare(source, hqxfile):
	"""(source, hqxfile) - Check whether the two files match (forks only)"""
	ifp = binhex.HexBin(hqxfile)

	sfp = open(source, 'rb')
	while 1:
		d = ifp.read(128000)
		d2 = sfp.read(128000)
		if d <> d2:
			return 0
		if not d: break
	sfp.close()
	ifp.close_data()
	
	d = ifp.read_rsrc(128000)
	if d:
		sfp = binhex.openrsrc(source, 'rb')
		d2 = sfp.read(128000)
		if d <> d2:
			return 0
		while 1:
			d = ifp.read_rsrc(128000)
			d2 = sfp.read(128000)
			if d <> d2:
				return 0
			if not d: break
	return 1

# Project files to handle
project_files = {}

def hexbincwprojects(creator):
	"""Compact and hexbin all files remembered with a given creator"""
	cw_running = 0
	for fss in project_files[creator]:
		srcfile = fss.as_pathname()
		
		old_style = 0
		if srcfile[-1] == 'µ':
			dstfile = srcfile[:-1]+'mu.hqx'
			old_style = 1
		elif srcfile[-3] == '.mu':
			dstfile = srcfile + '.hqx'
		elif ord(srcfile[-1]) >= 128:
			dstfile = srcfile[:-1]+`ord(srcfile[-1])`+'.hqx'
		else:
			dstfile = srcfile + '.hqx'
			
		if os.path.exists(dstfile) and \
				os.stat(dstfile)[8] >= os.stat(srcfile)[8]:
			print 'Skip', dstfile,'- Up-to-date'
			continue
		print 'Compacting', dstfile
		if old_style:
			if not cw_running:
				try:
					mgr = MwShell(creator, start=1)
				except 'foo':
					print 'Not handled:', creator
					return
				cw_running = 1
			mgr.open(fss)
			mgr.Reset_File_Paths()
			mgr.Remove_Binaries()
			mgr.Close_Project()
		
		print 'Binhexing', dstfile
		binhex.binhex(srcfile, dstfile)
	if cw_running:
		mgr.quit()
	
def copycwproject(path, name):
	"""Copy CW project (if needed) and remember for hexbinning"""
	global project_files
	
	dstdir = os.path.join(TOP, CWDIR)
	if path[:len(dstdir)] == dstdir:
		return
	srcdir = os.path.split(path)[0]
	srcdir = os.path.split(srcdir)[1]
	if srcdir in CWDIRDIRS:
		if not os.path.exists(dstdir):
			print dstdir
			print 'No CW-project dir, skip', name
			return
		dstfile = os.path.join(dstdir, os.path.join(srcdir, name))
	else:
		if path[-2:] == '.µ':
			dstfile = path[:-2]+ '.mu'
		elif path[-4:] == '.prj':
			dstfile = None
		else:
			return

	if dstfile:
		# If the destination doesn't exists or is older that the source
		# we copy and remember it
		
		if os.path.exists(dstfile) and \
				os.stat(dstfile)[8] >= os.stat(path)[8]:
			print 'Not copying', path,'- Up-to-date'
		else:
			print 'Copy', path
			macostools.copy(path, dstfile)
	else:
		dstfile = path
	
	fss = macfs.FSSpec(dstfile)
	creator = fss.GetCreatorType()[0]
	
	if project_files.has_key(creator):
		project_files[creator].append(fss)
	else:
		project_files[creator] = [fss]	
	
def copycwexpfile(path, name):
	"""Copy CW export file"""
	global project_files
	
	dstdir = os.path.join(TOP, CWDIR)
	if path[:len(dstdir)] == dstdir:
		return
	srcdir = os.path.split(path)[0]
	srcdir = os.path.split(srcdir)[1]
	if srcdir in CWDIRDIRS:
		if not os.path.exists(dstdir):
			print dstdir
			print 'No CW-project dir, skip', name
			return
		dstfile = os.path.join(dstdir, os.path.join(srcdir, name))
	else:
		if path[-6:] != '.µ.exp':
			return
		dstfile = path[:-6] + '.mu.exp'
	if dstfile[-6:] == '.µ.exp':
		dstfile = dstfile[:-6]+'.mu.exp'

	# If the destination doesn't exists or is older that the source
	# we copy and remember it
	
	if os.path.exists(dstfile) and \
			os.stat(dstfile)[8] >= os.stat(path)[8]:
		print 'Not copying', path,'- Up-to-date'
	else:
		print 'Copy', path
		macostools.copy(path, dstfile)	

extensions = [
##	('.rsrc', binhexit),
##	('.gif', binhexit),
	('.µ', copycwproject),
	('.prj', copycwproject),
	('.prj.exp', copycwexpfile),
	('.µ.exp', copycwexpfile)
	]

def walker(arg, top, names):
	lnames = names[:]
	for n in lnames:
		if n[0] == '(' and n[-1] == ')':
			names.remove(n)
			continue
		for ext, handler in extensions:
			if n[-len(ext):] == ext:
				name = os.path.join(top, n)
				handler(name, n)
				
def dodir(name):
	global TOP, project_files
	TOP = name
	os.path.walk(name, walker, None)
	
##	for creator in project_files.keys():
##		hexbincwprojects(creator)
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
	
