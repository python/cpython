import imageop

error = 'imgconv.error'

LOSSY = 1
NOT_LOSSY = 0

def null(img, x, y):
	return img
	
def mono2grey(img, x, y):
	return imageop.mono2grey(img, x, y, 0, 255)

converters = [ \
	  ('grey',  'grey4', imageop.grey2grey4,   LOSSY), \
	  ('grey',  'grey2', imageop.dither2grey2, LOSSY), \
	  ('grey',  'mono',  imageop.dither2mono,  LOSSY), \
	  ('mono',  'grey',  mono2grey,            NOT_LOSSY), \
	  ('grey2', 'grey',  imageop.grey22grey,   NOT_LOSSY), \
	  ('grey4', 'grey',  imageop.grey42grey,   NOT_LOSSY), \
]

def addconverter(fcs, tcs, lossy, func):
	for i in range(len(converters)):
		ifcs, itcs, irtn, ilossy = converters[i]
		if (fcs, tcs) == (ifcs, itcs):
			converters[i] = (fcs, tcs, func, lossy)
			return
	converters.append((fcs,tcs,lossy,func))

def getconverter(fcs, tcs):
	if fcs == tcs: return null
	
	for ifcs, itcs, irtn, ilossy in converters:
		if (fcs, tcs) == (ifcs, itcs):
			return irtn
	raise error, 'Sorry, that conversion is not implemented'
	  
