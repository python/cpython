"""macostools - Various utility functions for MacOS.

mkalias(src, dst) - Create a finder alias 'dst' pointing to 'src'
copy(src, dst) - Full copy of 'src' to 'dst'
"""

import macfs
from Carbon import Res
import os
from MACFS import *
import MacOS
import time
try:
	openrf = MacOS.openrf
except AttributeError:
	# Backward compatability
	openrf = open

Error = 'macostools.Error'

BUFSIZ=0x80000		# Copy in 0.5Mb chunks

#
# Not guaranteed to be correct or stay correct (Apple doesn't tell you
# how to do this), but it seems to work.
#
def mkalias(src, dst, relative=None):
	"""Create a finder alias"""
	srcfss = macfs.FSSpec(src)
	# The next line will fail under unix-Python if the destination
	# doesn't exist yet. We should change this code to be fsref-based.
	dstfss = macfs.FSSpec(dst)
	if relative:
		relativefss = macfs.FSSpec(relative)
		# ik mag er geen None in stoppen :-(
		alias = srcfss.NewAlias(relativefss)
	else:
		alias = srcfss.NewAlias()
	
	if os.path.isdir(src):
		cr, tp = 'MACS', 'fdrp'
	else:
		cr, tp = srcfss.GetCreatorType()
	
	Res.FSpCreateResFile(dstfss, cr, tp, -1)
	h = Res.FSpOpenResFile(dstfss, 3)
	resource = Res.Resource(alias.data)
	resource.AddResource('alis', 0, '')
	Res.CloseResFile(h)
	
	dstfinfo = dstfss.GetFInfo()
	dstfinfo.Flags = dstfinfo.Flags|0x8000    # Alias flag
	dstfss.SetFInfo(dstfinfo)
	
def mkdirs(dst):
	"""Make directories leading to 'dst' if they don't exist yet"""
	if dst == '' or os.path.exists(dst):
		return
	head, tail = os.path.split(dst)
	if os.sep == ':' and not ':' in head:
		head = head + ':'
	mkdirs(head)
	os.mkdir(dst, 0777)
	
def touched(dst):
	"""Tell the finder a file has changed"""
	file_fss = macfs.FSSpec(dst)
	vRefNum, dirID, name = file_fss.as_tuple()
	dir_fss = macfs.FSSpec((vRefNum, dirID, ''))
	crdate, moddate, bkdate = dir_fss.GetDates()
	now = time.time()
	if now == moddate:
		now = now + 1
	dir_fss.SetDates(crdate, now, bkdate)
	
def touched_ae(dst):
	"""Tell the finder a file has changed"""
	import Finder
	f = Finder.Finder()
	file_fss = macfs.FSSpec(dst)
	vRefNum, dirID, name = file_fss.as_tuple()
	dir_fss = macfs.FSSpec((vRefNum, dirID, ''))
	f.update(dir_fss)
	
def copy(src, dst, createpath=0, copydates=1, forcetype=None):
	"""Copy a file, including finder info, resource fork, etc"""
	if hasattr(src, 'as_pathname'):
		src = src.as_pathname()
	if hasattr(dst, 'as_pathname'):
		dst = dst.as_pathname()
	if createpath:
		mkdirs(os.path.split(dst)[0])
	
	ifp = open(src, 'rb')
	ofp = open(dst, 'wb')
	d = ifp.read(BUFSIZ)
	while d:
		ofp.write(d)
		d = ifp.read(BUFSIZ)
	ifp.close()
	ofp.close()
	
	ifp = openrf(src, '*rb')
	ofp = openrf(dst, '*wb')
	d = ifp.read(BUFSIZ)
	while d:
		ofp.write(d)
		d = ifp.read(BUFSIZ)
	ifp.close()
	ofp.close()
	
	srcfss = macfs.FSSpec(src)
	dstfss = macfs.FSSpec(dst)
	sf = srcfss.GetFInfo()
	df = dstfss.GetFInfo()
	df.Creator, df.Type = sf.Creator, sf.Type
	if forcetype != None:
		df.Type = forcetype
	df.Flags = (sf.Flags & (kIsStationary|kNameLocked|kHasBundle|kIsInvisible|kIsAlias))
	dstfss.SetFInfo(df)
	if copydates:
		crdate, mddate, bkdate = srcfss.GetDates()
		dstfss.SetDates(crdate, mddate, bkdate)
	touched(dstfss)
	
def copytree(src, dst, copydates=1):
	"""Copy a complete file tree to a new destination"""
	if os.path.isdir(src):
		mkdirs(dst)
		files = os.listdir(src)
		for f in files:
			copytree(os.path.join(src, f), os.path.join(dst, f), copydates)
	else:
		copy(src, dst, 1, copydates)
