'''
A really quick and dirty hack to extend  PixMapWrapper
They are mere copies of the toImage and fromImage methods.
Riccardo Trocca (rtrocca@libero.it)
'''
from PixMapWrapper import *
import Numeric

class ExtPixMapWrapper(PixMapWrapper):
	
	def toNumeric(self):
		
		data = self.tostring()[1:] + chr(0)
		bounds = self.bounds
		tmp=Numeric.fromstring(data,Numeric.UnsignedInt8)
		#tmp.shape=(bounds[3]-bounds[1],bounds[2]-bounds[0],4)
		tmp.shape=(bounds[2]-bounds[0],bounds[3]-bounds[1],4)
		return Numeric.transpose(tmp,(1,0,2))
		
	def fromNumeric(self,num):
		s=num.shape
		x=num.shape[1]
		y=num.shape[0]
		#bands=1 Greyscale image
		#bands=3 RGB image
		#bands=4 RGBA image
		if len(s)==2:
			bands=1
			num=Numeric.resize(num,(y,x,1))
		else:
			bands=num.shape[2]
		
		if bands==1:
			num=Numeric.concatenate((num,num,num),2)
			bands=3
		if bands==3:
			alpha=Numeric.ones((y,x))*255
			alpha.shape=(y,x,1)
			num=Numeric.concatenate((num,alpha),2)
			
		data=chr(0)+Numeric.transpose(num,(1,0,2)).astype(Numeric.UnsignedInt8).tostring()
		PixMapWrapper.fromstring(self,data,x,y)
		
			
		
			
