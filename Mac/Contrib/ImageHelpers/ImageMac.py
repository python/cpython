'''
ImageMac.py by Trocca Riccardo (rtrocca@libero.it)
This module provides functions to display images and Numeric arrays
It provides two classes ImageMacWin e NumericMacWin and two simple methods showImage and
showNumeric.

They work like this:
showImage(Image,"optional window title",zoomFactor)
the same for showNumeric
zoomfactor (defaults to 1) allows to zoom in the image by a factor of 1x 2x 3x and so on
I did't try with a 0.5x or similar.
The windows don't provide a scrollbar or a resize box.
Probably a better solution (and more similar to the original implementation in PIL and NumPy)
would be to save a temp file is some suitable format and then make an application (through appleevents) to open it.
Good guesses should be GraphicConverter or PictureViewer.

However the classes ImageMacWin e NumericMacWin use an extended version of PixMapWrapper in order to 
provide an image buffer and then blit it in the window.

Being one of my first experiences with Python I didn't use Exceptions to signal error conditions, sorry.

'''
import W
import Qd
from ExtPixMapWrapper import *
from Numeric import *
import Image
import macfs

class ImageMacWin(W.Window):
	
	def __init__(self,size=(300,300),title="ImageMacWin"):
		self.pm=ExtPixMapWrapper()
		self.empty=1
		self.size=size
		W.Window.__init__(self,size,title)
	
	def Show(self,image,resize=0):
		#print "format: ", image.format," size: ",image.size," mode: ",image.mode
		#print "string len :",len(image.tostring())
		self.pm.fromImage(image)
		self.empty=0
		if resize:
			self.size=(image.size[0]*resize,image.size[1]*resize)
			W.Window.do_resize(self,self.size[0],self.size[1],self.wid)
		self.do_drawing()
	
	def do_drawing(self):
		#print "do_drawing"
		self.SetPort()
		Qd.RGBForeColor( (0,0,0) )
		Qd.RGBBackColor((65535, 65535, 65535))
		Qd.EraseRect((0,0,self.size[0],self.size[1]))
		if not self.empty:
			#print "should blit"
			self.pm.blit(0,0,self.size[0],self.size[1])
		
	def do_update(self,macoswindowid,event):
		#print "update"
		self.do_drawing()
		
class NumericMacWin(W.Window):
	
	def __init__(self,size=(300,300),title="ImageMacWin"):
		self.pm=ExtPixMapWrapper()
		self.empty=1
		self.size=size
		W.Window.__init__(self,size,title)
	
	def Show(self,num,resize=0):
		#print "shape: ", num.shape
		#print "string len :",len(num.tostring())
		self.pm.fromNumeric(num)
		self.empty=0
		if resize:
			self.size=(num.shape[1]*resize,num.shape[0]*resize)
			W.Window.do_resize(self,self.size[0],self.size[1],self.wid)
		self.do_drawing()
	
	def do_drawing(self):
		#print "do_drawing"
		self.SetPort()
		Qd.RGBForeColor( (0,0,0) )
		Qd.RGBBackColor((65535, 65535, 65535))
		Qd.EraseRect((0,0,self.size[0],self.size[1]))
		if not self.empty:
			#print "should blit"
			self.pm.blit(0,0,self.size[0],self.size[1])
		
	def do_update(self,macoswindowid,event):
		#print "update"
		self.do_drawing()

'''
Some utilities: convert an Image to a NumPy array and viceversa.
The Image2Numeric function doesn't make any color space conversion.
The Numeric2Image function returns an L or RGB or RGBA images depending on the shape of
the array:
	(x,y)	-> 	'L'
	(x,y,1)	-> 	'L'
	(x,y,3)	->	'RGB'
	(x,y,4)	->	'RGBA'
'''
def Image2Numeric(im):
	tmp=fromstring(im.tostring(),UnsignedInt8)
	
	if (im.mode=='RGB')|(im.mode=='YCbCr'):
		bands=3
	
	if (im.mode=='RGBA')|(im.mode=='CMYK'):
		bands=4
	
	if (im.mode=='L'):
		bands=1

	tmp.shape=(im.size[0],im.size[1],bands)
	return transpose(tmp,(1,0,2))

def Numeric2Image(num):
	#sometimes a monoband image's shape can be (x,y,1), other times just (x,y). Here w deal with both
	if len(num.shape)==3: 
		bands=num.shape[2]
		if bands==1:
			mode='L'
		elif bands==3:
			mode='RGB'
		else:
			mode='RGBA'
		return Image.fromstring(mode,(num.shape[1],num.shape[0]),transpose(num,(1,0,2)).astype(UnsignedInt8).tostring())
	else:
		return Image.fromstring('L',(num.shape[1],num.shape[0]),transpose(num).astype(UnsignedInt8).tostring())	

def showImage(im,title="ImageWin",zoomFactor=1):
	imw=ImageMacWin((300,200),title)
	imw.open()
	try:
		imw.Show(im,zoomFactor )
	except MemoryError,e:
		imw.close()
		print "ImageMac.showImage: Insufficient Memory"
		

def showNumeric(num,title="NumericWin",zoomFactor=1):
	#im=Numeric2Image(num)
	numw=NumericMacWin((300,200),title)
	numw.open()
	try:
		numw.Show(num,zoomFactor )
	except MemoryError:
		numw.close()
		print "ImageMac.showNumeric Insufficient Memory"
	
'''
GimmeImage pops up a file dialog and asks for an image file.
it returns a PIL image.
Optional argument: a string to be displayed by the dialog. 
'''
	
def GimmeImage(prompt="Image File:"):
	import macfs
	fsspec, ok = macfs.PromptGetFile(prompt)
	if ok:
		path = fsspec.as_pathname()
		return Image.open(path)
	return None
	
'''
This is just some experimental stuff:
	Filter3x3 a convolution filter (too slow use signal tools instead)
	diffBWImage subtracts 2 images contained in NumPy arrays
	averageN it computes the average of a list incrementally
	BWImage converts an RGB or RGBA image (in a NumPy array) to BW
	SplitBands splits the bands of an Image (inside a NumPy)
	NumHisto and PlotHisto are some experiments to plot an intesity histogram
'''

def Filter3x3(mul,fi,num):
	(a,b,c,d,e,f,g,h,i)=fi
	print fi
	num.shape=(num.shape[0],num.shape[1])
	res=zeros(num.shape)
	for x in range(1,num.shape[0]-1):
		for y in range(1,num.shape[1]-1):
			xb=x-1
			xa=x+1
			yb=y-1
			ya=y+1
			res[x,y]=int((a*num[xb,yb]+b*num[x,yb]+c*num[xa,yb]+d*num[xb,y]+e*num[x,y]+f*num[xa,y]+g*num[xb,ya]+h*num[x,ya]+i*num[xa,ya])/mul)
	return res
		
def diffBWImage(num1,num2):
	return 127+(num1-num2)/2

def averageN(N,avrg,new):
	return ((N-1)*avrg+new)/N
		
def BWImage(num):
	if num.shape[2]==3:
		bw=array(((0.3086,0.6094,0.0820)))
	else:
		bw=array(((0.3086,0.6094,0.0820,0)))
	res=innerproduct(num,bw)
	res.shape=(res.shape[0],res.shape[1])
	return res

def SplitBands(num):
	x=num.shape[0]
	y=num.shape[1]
	if num.shape[2]==3:
		return (reshape(num[:,:,0],(x,y)),reshape(num[:,:,1],(x,y)),reshape(num[:,:,2],(x,y)))
	else:
		return (reshape(num[:,:,0],(x,y)),reshape(num[:,:,1],(x,y)),reshape(num[:,:,2],(x,y)),reshape(num[:,:,3],(x,y)))

def NumHisto(datas):
	#print "type(datas) ",type(datas)
	a=ravel(datas)
	n=searchsorted(sort(a),arange(0,256))
	n=concatenate([n,[len(a)]])
	return n[1:]-n[:-1]
	
def PlotHisto(datas,ratio=1):
	from graphite import *
	from MLab import max
	h=NumHisto(datas)
	#print "histo: ",h
	#print "histo.shape: ",h.shape
	maxval=max(h)
	#print "maxval ",maxval
	h.shape=(256,1)
	x=arange(0,256)
	x.shape=(256,1)
	datah=concatenate([x,h],1)
	print "data: "
	print datah
	g=Graph()
	g.datasets.append(Dataset(datah))
	f0=PointPlot()
	f0.lineStyle = LineStyle(width=2, color=red, kind=SOLID)
	g.formats = [f0]
	g.axes[X].range = [0,255]
	g.axes[X].tickMarks[0].spacing = 10
	#g.axes[X].tickMarks[0].labels = "%d"
	g.axes[Y].range = [0,maxval/ratio]
	g.bottom = 370
	g.top =10
	g.left=10
	g.right=590
	
	genOutput(g,'QD',size=(600,400))
	
def test():
	import MacOS
	import Image
	import ImageFilter
	import Numeric
	fsspec, ok = macfs.PromptGetFile("Image File:")
	if ok: 
		path = fsspec.as_pathname()
		im=Image.open(path)
		#im2=im.filter(ImageFilter.SMOOTH)
		showImage(im,"normal")
		num=Image2Numeric(im)
		#num=Numeric.transpose(num,(1,0,2))
		
		showNumeric(num,"Numeric")
		
		print "num.shape ",num.shape
		showImage(Numeric2Image(num),"difficile")
		#showImage(im.filter(ImageFilter.SMOOTH),"smooth")
		#showImage(im.filter(ImageFilter.FIND_EDGES).filter(ImageFilter.SHARPEN),"detail")
	
		print "here"
	else:
		print "did not open file"

if __name__ == '__main__':
	test()