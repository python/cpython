import Qt
import QuickTime
import macfs
import Qd
from QuickDraw import srcCopy
from ExtPixMapWrapper import ExtPixMapWrapper
from Qdoffs import *
import ImageMac
import W



	
def GetFrames(m):
	frameCount=0
	theTime=0
	type=QuickTime.VideoMediaType
	#type='MPEG'
	flags=QuickTime.nextTimeMediaSample
	flags=flags+QuickTime.nextTimeEdgeOK
	
	while theTime>=0:
		(theTime,duration)=m.GetMovieNextInterestingTime(flags,1,type,theTime,0)
		#print "theTime ",theTime,"   duration ",duration
		frameCount=frameCount+1
		flags = QuickTime.nextTimeMediaSample
	
	
	return frameCount-1
	
def GetMovieFromOpenFile():
	fss, ok = macfs.StandardGetFile(QuickTime.MovieFileType)
	mov = None
	if  ok:
		movieResRef = Qt.OpenMovieFile(fss, 1)
		mov, d1, d2 = Qt.NewMovieFromFile(movieResRef, 0, QuickTime.newMovieActive)
		
	return mov


class ExtMovie:
	def __init__(self,mov):
		
		self.frames=0
		self.frameArray=[]
		self.movie=mov
		self._countFrames()
		r=self.movie.GetMovieBox()
		self.myRect=(0,0,r[2]-r[0],r[3]-r[1])
		self.movie.SetMovieBox(self.myRect)
		self.pm=ExtPixMapWrapper()
		self.pm.left=0
		self.pm.top=0
		self.pm.right=r[2]-r[0]
		self.pm.bottom=r[3]-r[1]
		self.gw=NewGWorld(32,self.myRect,None,None,0)
		self.movie.SetMovieGWorld(self.gw.as_GrafPtr(), self.gw.GetGWorldDevice())
		self.GotoFrame(0)
			
	def _countFrames(self):
		#deve contare il numero di frame, creare un array con i tempi per ogni frame
		theTime=0
		#type=QuickTime.VIDEO_TYPE
		type=QuickTime.VideoMediaType
		flags=QuickTime.nextTimeMediaSample+QuickTime.nextTimeEdgeOK
		
		while theTime>=0:
			(theTime,duration)=self.movie.GetMovieNextInterestingTime(flags,1,type,theTime,0)
			self.frameArray.append((theTime,duration))
			flags = QuickTime.nextTimeMediaSample
			self.frames=self.frames+1	
		
		
	
	def GotoFrame(self,n):
		if n<=self.frames:
			self.curFrame=n
			(port,device)=GetGWorld()
			SetGWorld(self.gw.as_GrafPtr(),None)
			(self.now,self.duration)=self.frameArray[n]
			self.movie.SetMovieTimeValue(self.now)
			pixmap=self.gw.GetGWorldPixMap()
			
			if not LockPixels(pixmap):
				print "not locked"
			else:
			
				#Qd.EraseRect(self.myRect)
				#this draws the frame inside the current gworld
				self.movie.MoviesTask(0)
				#this puts it in the buffer pixmap 
				self.pm.grab(0,0,self.myRect[2],self.myRect[3])
				UnlockPixels(pixmap)
				#self.im=self.pm.toImage()
			SetGWorld(port,device)
		
	def NextFrame(self):
		self.curFrame=self.curFrame+1
		if self.curFrame>self.frames:
			self.curFrame=0
		self.GotoFrame(self.curFrame)
	
	def isLastFrame():
		return self.curFrame==self.frames
		
		
	def GetImage(self):
		return self.pm.toImage()
		
	def GetImageN(self,n):
		self.GotoFrame(n)
		return self.pm.toImage()
		
	def GetNumeric(self):
		return self.pm.toNumeric()
		
	def GetNumericN(self,n):
		self.GotoFrame(n)
		return self.pm.toNumeric()
		
	def Blit(self,destRect):
		Qd.RGBForeColor( (0,0,0) )
		Qd.RGBBackColor((65535, 65535, 65535))
		
		#Qd.MoveTo(10,10)
		#Qd.LineTo(200,150)
		Qd.CopyBits(self.gw.portBits,Qd.GetPort().portBits,self.myRect,destRect,srcCopy,None)

class MovieWin(W.Window):
	
	def __init__(self,eMovie,title="MovieWin"):
		self.ExtMovie=eMovie		
		
def test():
	import ImageFilter
	from MLab import max
	from MLab import min
	from Numeric import *
	Qt.EnterMovies()
	m=GetMovieFromOpenFile()
	em=ExtMovie(m)
	print "Total frames:",em.frames,"  Current frame:",em.curFrame	
	#ImageMac.showImage(em.GetImage(),"frame 0",1)
	#em.GotoFrame(500)
	#ImageMac.showImage(em.GetImage().filter(ImageFilter.SMOOTH),"frame 500",2)
	#ImageMac.showImage(em.GetImageN(1000),"frame 1000",2)
	#r=array(((1,0,0,0),(0,0,0,0),(0,0,0,0),(0,0,0,0)))
	#g=array(((0,0,0,0),(0,1,0,0),(0,0,0,0),(0,0,0,0)))
	#b=array(((0,0,0,0),(0,0,0,0),(0,0,1,0),(0,0,0,0)))
	#bw=array(((0.3086,0.6094,0.0820,0)))
	#r2=array(((1,0,0,0)))
	#ImageMac.showNumeric(em.GetNumericN(0),"frame 0",1)
	#print em.GetNumericN(500).shape
	#print "original (1,1)",em.GetNumericN(0)[100,100]
	#print "product shape ",innerproduct(em.GetNumericN(0),r).shape
	#print "product (1,1) ",innerproduct(em.GetNumericN(0),r)[100,100]
	
	#ImageMac.showNumeric(ImageMac.BWImage(em.GetNumericN(50)))
	#ImageMac.showNumeric(innerproduct(em.GetNumericN(500),r),"frame 500r",2)
	#ImageMac.showNumeric(innerproduct(em.GetNumericN(500),g),"frame 500g",2)
	#ImageMac.showNumeric(innerproduct(em.GetNumericN(500),b),"frame 500b",2)
	
	#ImageMac.showNumeric(innerproduct(em.GetNumericN(500),r2),"frame 500r2",2)
	#ImageMac.showNumeric(innerproduct(em.GetNumericN(10),bw),"frame 0bw",1)
	#ImageMac.showNumeric(innerproduct(em.GetNumericN(400),bw),"frame 10bw",1)
	#colordif=(em.GetNumericN(100)-em.GetNumericN(10))+(255,255,255,255)
	#colordif=colordif/2
	#ImageMac.showNumeric(colordif,"colordif",1)
	#ImageMac.showNumeric(ImageMac.BWImage(colordif),"bwcolordif",1)
	ilut=arange(0,256)
	#ilut[118]=255
	#ilut[119]=255
	#ilut[120]=255
	ilut[121]=255
	ilut[122]=255
	ilut[123]=255
	ilut[124]=255
	ilut[125]=255
	ilut[126]=255
	ilut[127]=255
	ilut[128]=255
	ilut[129]=255
	#ilut[130]=255
	#ilut[131]=255
	#ilut[132]=255
	mlut=ones(256)
	mlut[118]=0
	mlut[119]=0
	mlut[120]=0
	mlut[121]=0
	mlut[122]=0
	mlut[123]=0
	mlut[124]=0
	mlut[125]=0
	mlut[126]=0
	mlut[127]=0
	mlut[128]=0
	mlut[129]=0
	mlut[130]=0
	mlut[131]=0
	mlut[132]=0
	
	ImageMac.showImage(em.GetImageN(100),"provaImg",2)
	ImageMac.showNumeric(em.GetNumericN(100),"provaNum",2)
	ImageMac.showImage(em.GetImageN(100).filter(ImageFilter.SMOOTH),"frame 500",2)
	#image=ImageMac.BWImage(em.GetNumericN(100))
	#ImageMac.showNumeric(image)
	
	
	
	
	
	
	
	
	#difimage=abs(image-ImageMac.BWImage(em.GetNumericN(10)))
	#ImageMac.PlotHisto(difimage,32)
	#ImageMac.showNumeric(difimage)
	#difimage=127+(image-ImageMac.BWImage(em.GetNumericN(10)))/2
	#ImageMac.PlotHisto(difimage,32)
	#ImageMac.showNumeric(difimage)
	#fimage=ImageMac.Filter3x3(16.0,(1,1,1,1,8,1,1,1,1),difimage)
	#ImageMac.showNumeric(fimage)
	#difimage2=choose(fimage.astype(UnsignedInt8),ilut)
	#ImageMac.showNumeric(difimage2)
	
	#(r,g,b,a)=ImageMac.SplitBands(em.GetNumericN(10))
	#ImageMac.showNumeric(r,"r")
	#ImageMac.showNumeric(g,"g")
	#ImageMac.showNumeric(b,"b")
	#ImageMac.showNumeric(a,"a")
	#bwdif=abs(((innerproduct(em.GetNumericN(400),bw)-innerproduct(em.GetNumericN(10),bw))+255)/2)
	#ImageMac.showNumeric(bwdif,"frame diff/bw",1)
	#ImageMac.PlotHisto(bwdif)
	#ImageMac.showNumeric(choose(bwdif.astype(UnsignedInt8),ilut),"frame diff/bw",1)
	#ImageMac.PlotHisto(choose(bwdif.astype(UnsignedInt8),ilut))
	#bwimage=ImageMac.BWImage(em.GetNumericN(100))
	#ImageMac.showNumeric((ImageMac.BWImage(em.GetNumericN(90))+ImageMac.BWImage(em.GetNumericN(110))+ImageMac.BWImage(em.GetNumericN(130))+ImageMac.BWImage(em.GetNumericN(150))+ImageMac.BWImage(em.GetNumericN(170)))/5)
	#bwdif=abs(((bwimage-ImageMac.BWImage(em.GetNumericN(10)))+255)/2)
	#ImageMac.showNumeric(bwimage,"original frame",1)
	#ImageMac.showNumeric(bwdif,"frame diff/bw",1)
	#ImageMac.PlotHisto(bwdif)
	#ImageMac.showNumeric(choose(bwdif.astype(UnsignedInt8),ilut),"frame diff/bw",1)
	#mmask=choose(bwdif.astype(UnsignedInt8),mlut)
	#ImageMac.showNumeric(255-255*mmask,"frame diff/bw",1)
	#mmask.shape=bwimage.shape
	#ImageMac.showNumeric(mmask*bwimage,"frame diff/bw",1)
	
	#ImageMac.showNumeric((innerproduct(em.GetNumericN(300),bw)-innerproduct(em.GetNumericN(0),bw)),"frame diff/bw",1)
	#ImageMac.showNumeric((innerproduct(em.GetNumericN(400)-em.GetNumericN(10),bw)),"frame diff2/bw",1)
	#cdif=em.GetNumericN(400)-em.GetNumericN(10)
	#ImageMac.showNumeric(,"frame diff2/bw",1)
	
	#ImageMac.showNumeric(innerproduct(cdif,r),"frame 500r",1)
	#ImageMac.showNumeric(innerproduct(cdif,g),"frame 500g",1)
	#ImageMac.showNumeric(innerproduct(cdif,b),"frame 500b",1)
def test2():
	Qt.EnterMovies()
	m=GetMovieFromOpenFile()
	if m==None:
		print "no movie opened"
	else:
		em=ExtMovie(m)
		print "Total frames: ",em.frames,"  Current frame:",em.curFrame
		ImageMac.showImage(em.GetImage(),"frame 0",1)

if __name__ == '__main__':
	test2()
		