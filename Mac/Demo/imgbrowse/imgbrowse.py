"""imgbrowse - Display pictures using img"""

import FrameWork
import EasyDialogs
import Res
import Qd
import QuickDraw
import Win
#import List
import sys
import struct
import img
import imgformat
import macfs
import struct


# Where is the picture window?
LEFT=200
TOP=64
MINWIDTH=64
MINHEIGHT=64
MAXWIDTH=320
MAXHEIGHT=320

def dumppixmap(data):
	baseAddr, \
		rowBytes, \
		t, l, b, r, \
		pmVersion, \
		packType, packSize, \
		hRes, vRes, \
		pixelType, pixelSize, \
		cmpCount, cmpSize, \
		planeBytes, pmTable, pmReserved \
			= struct.unpack("lhhhhhhhlllhhhhlll", data)
	print 'Base:       0x%x'%baseAddr
	print 'rowBytes:   %d (0x%x)'%(rowBytes&0x3fff, rowBytes)
	print 'rect:       %d, %d, %d, %d'%(t, l, b, r)
	print 'pmVersion:  0x%x'%pmVersion
	print 'packing:    %d %d'%(packType, packSize)
	print 'resolution: %f x %f'%(float(hRes)/0x10000, float(vRes)/0x10000)
	print 'pixeltype:  %d, size %d'%(pixelType, pixelSize)
	print 'components: %d, size %d'%(cmpCount, cmpSize)
	print 'planeBytes: %d (0x%x)'%(planeBytes, planeBytes)
	print 'pmTable:    0x%x'%pmTable
	print 'pmReserved: 0x%x'%pmReserved

def mk16pixmap(w, h, data):
	"""kludge a pixmap together"""
	rv = struct.pack("lhhhhhhhlllhhhhlll",
		id(data)+12,
		w*2 + 0x8000,
		0, 0, h, w,
		0,
		0, 0, # XXXX?
		72<<16, 72<<16,
		16, 16, # XXXX
		3, 5,
		0, 0, 0)
	print 'Our pixmap, size %d:'%len(rv)
	dumppixmap(rv)
	return Qd.RawBitMap(rv)

def main():
	print 'hello world'
	imgbrowse()

class imgbrowse(FrameWork.Application):
	def __init__(self):
		# First init menus, etc.
		FrameWork.Application.__init__(self)
		self.lastwin = None
		# Finally, go into the event loop
		self.mainloop()
		
	def makeusermenus(self):
		self.filemenu = m = FrameWork.Menu(self.menubar, "File")
		self.openitem = FrameWork.MenuItem(m, "Open...", "O", self.opendoc)
		self.infoitem = FrameWork.MenuItem(m, "Info", "I", self.info)
		self.quititem = FrameWork.MenuItem(m, "Quit", "Q", self.quit)
	
	def quit(self, *args):
		self._quit()
		
	def opendoc(self, *args):
		fss, ok = macfs.StandardGetFile() # Any file type
		if not ok:
			return
		bar = EasyDialogs.ProgressBar('Reading and converting...')
		pathname = fss.as_pathname()
		try:
			rdr = img.reader(imgformat.macrgb16, pathname)
		except img.error, arg:
			EasyDialogs.Message(`arg`)
			return
		w, h = rdr.width, rdr.height
		bar.set(10)
		data = rdr.read()
		del bar
		pixmap = mk16pixmap(w, h, data)
		self.showimg(w, h, pixmap)
			
	def showimg(self, w, h, pixmap):
		win = imgwindow(self)
		win.open(w, h, pixmap)
		self.lastwin = win

	def info(self, *args):
		if self.lastwin:
			self.lastwin.info()		
		
class imgwindow(FrameWork.Window):
	def open(self, width, height, pixmap):
		self.pixmap = pixmap
		self.pictrect = (0, 0, width, height)
		bounds = (LEFT, TOP, LEFT+width, TOP+height)
		
		self.wid = Win.NewCWindow(bounds, "Picture", 1, 0, -1, 1, 0)
		self.do_postopen()
		
	def do_update(self, *args):
		pass
		currect = self.fitrect()
		print 'PICT:', self.pictrect
		print 'WIND:', currect
		print 'ARGS:', (self.pixmap, self.wid.GetWindowPort().portBits, self.pictrect,
				currect, QuickDraw.srcCopy, None)
		self.info()
		Qd.CopyBits(self.pixmap, self.wid.GetWindowPort().portBits, self.pictrect,
				currect, QuickDraw.srcCopy+QuickDraw.ditherCopy, None)
##		Qd.DrawPicture(self.picture, currect)
		
	def fitrect(self):
		"""Return self.pictrect scaled to fit in window"""
		graf = self.wid.GetWindowPort()
		screenrect = graf.portRect
		picwidth = self.pictrect[2] - self.pictrect[0]
		picheight = self.pictrect[3] - self.pictrect[1]
		if picwidth > screenrect[2] - screenrect[0]:
			factor = float(picwidth) / float(screenrect[2]-screenrect[0])
			picwidth = picwidth / factor
			picheight = picheight / factor
		if picheight > screenrect[3] - screenrect[1]:
			factor = float(picheight) / float(screenrect[3]-screenrect[1])
			picwidth = picwidth / factor
			picheight = picheight / factor
		return (screenrect[0], screenrect[1], screenrect[0]+int(picwidth),
				screenrect[1]+int(picheight))
				
	def info(self):
		graf = self.wid.GetWindowPort()
		bits = graf.portBits
		dumppixmap(bits.pixmap_data)

main()
