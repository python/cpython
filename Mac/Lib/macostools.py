"""macostools - Various utility functions for MacOS.

mkalias(src, dst) - Create a finder alias 'dst' pointing to 'src'
copy(src, dst) - Full copy of 'src' to 'dst'
"""

import macfs
import Res
import os

Error = 'macostools.Error'

FSSpecType = type(macfs.FSSpec(':'))

BUFSIZ=0x100000		# Copy in 1Mb chunks

#
# Not guaranteed to be correct or stay correct (Apple doesn't tell you
# how to do this), but it seems to work.
#
def mkalias(src, dst):
	"""Create a finder alias"""
	srcfss = macfs.FSSpec(src)
	dstfss = macfs.FSSpec(dst)
	alias = srcfss.NewAlias()
	srcfinfo = srcfss.GetFInfo()

	Res.FSpCreateResFile(dstfss, srcfinfo.Creator, srcfinfo.Type, -1)
	h = Res.FSpOpenResFile(dstfss, 3)
	resource = Res.Resource(alias.data)
	resource.AddResource('alis', 0, '')
	Res.CloseResFile(h)
	
	dstfinfo = dstfss.GetFInfo()
	dstfinfo.Flags = dstfinfo.Flags|0x8000    # Alias flag
	dstfss.SetFInfo(dstfinfo)
	
def copy(src, dst):
	"""Copy a file, including finder info, resource fork, etc"""
	srcfss = macfs.FSSpec(src)
	dstfss = macfs.FSSpec(dst)
	
	ifp = fopen(srcfss.as_pathname(), 'rb')
	ofp = fopen(dstfss.as_pathname(), 'wb')
	d = ifp.read(BUFSIZ)
	while d:
		ofp.write(d)
		d = ifp.read(BUFSIZ)
	ifp.close()
	ofp.close()
	
	ifp = fopen(srcfss.as_pathname(), '*rb')
	ofp = fopen(dstfss.as_pathname(), '*wb')
	d = ifp.read(BUFSIZ)
	while d:
		ofp.write(d)
		d = ifp.read(BUFSIZ)
	ifp.close()
	ofp.close()
	
	sf = srcfss.GetFInfo()
	df = dstfss.GetFInfo()
	df.Creator, df.Type, df.Flags = sf.Creator, sf.Type, sf.Flags
	dstfss.SetFInfo(df)
	
def copytree(src, dst):
	"""Copy a complete file tree to a new destination"""
	if os.path.isdir(src):
		if not os.path.exists(dst):
			os.mkdir(dst)
		elif not os.path.isdir(dst):
			raise Error, 'Not a directory: '+dst
		files = os.listdir(src)
		for f in files:
			copytree(os.path.join(src, f), os.path.join(dst, f))
	else:
		copy(src, dst)
