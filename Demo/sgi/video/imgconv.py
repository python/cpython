import imageop

error = 'imgconv.error'

LOSSY = 1
NOT_LOSSY = 0

def null(img, x, y):
	return img
	
def mono2grey(img, x, y):
	return imageop.mono2grey(img, x, y, 0, 255)

def grey2jpeggrey(img, x, y):
	import jpeg
	return jpeg.compress(img, x, y, 1)

def rgb2jpeg(img, x, y):
	import jpeg
	return jpeg.compress(img, x, y, 4)

def jpeggrey2grey(img, width, height):
	import jpeg
	data, width, height, bytesperpixel = jpeg.decompress(img)
	if bytesperpixel <> 1: raise RuntimeError, 'not greyscale jpeg'
	return data

def jpeg2rgb(img, width, height):
	import jpeg
	data, width, height, bytesperpixel = jpeg.decompress(img)
	if bytesperpixel <> 4: raise RuntimeError, 'not rgb jpeg'
	return data

converters = [ \
	  ('grey',  'grey4', imageop.grey2grey4,   LOSSY), \
	  ('grey',  'grey2', imageop.dither2grey2, LOSSY), \
	  ('grey',  'mono',  imageop.dither2mono,  LOSSY), \
	  ('mono',  'grey',  mono2grey,            NOT_LOSSY), \
	  ('grey2', 'grey',  imageop.grey22grey,   NOT_LOSSY), \
	  ('grey4', 'grey',  imageop.grey42grey,   NOT_LOSSY), \
	  ('rgb',   'rgb8',  imageop.rgb2rgb8,     LOSSY), \
	  ('rgb8',  'rgb',   imageop.rgb82rgb,     NOT_LOSSY), \
	  ('rgb',   'grey',  imageop.rgb2grey,     LOSSY), \
	  ('grey',  'rgb',   imageop.grey2rgb,     NOT_LOSSY), \
	  ('jpeggrey','grey',jpeggrey2grey,        NOT_LOSSY), \
	  ('grey',  'jpeggrey',grey2jpeggrey,      LOSSY), \
	  ('jpeg',  'rgb',   jpeg2rgb,             NOT_LOSSY), \
	  ('rgb',   'jpeg',  rgb2jpeg,             LOSSY), \
]

built = {}

def addconverter(fcs, tcs, lossy, func):
	for i in range(len(converters)):
		ifcs, itcs, irtn, ilossy = converters[i]
		if (fcs, tcs) == (ifcs, itcs):
			converters[i] = (fcs, tcs, func, lossy)
			return
	converters.append((fcs,tcs,lossy,func))

def getconverter(fcs, tcs):
	#
	# If formats are the same return the dummy converter
	#
	if fcs == tcs: return null
	#
	# Otherwise, if we have a converter return that one
	#
	for ifcs, itcs, irtn, ilossy in converters:
		if (fcs, tcs) == (ifcs, itcs):
			return irtn
	#
	# Finally, we try to create a converter
	#
	if not built.has_key(fcs):
		built[fcs] = enumerate_converters(fcs)
	if not built[fcs].has_key(tcs):
		raise error, 'Sorry, conversion from '+fcs+' to '+tcs+ \
			  ' is not implemented'
	if len(built[fcs][tcs]) == 3:
		#
		# Converter not instantiated yet
		#
		built[fcs][tcs].append(instantiate_converter(built[fcs][tcs]))
	cf = built[fcs][tcs][3]
	return cf

def enumerate_converters(fcs):
	cvs = {}
	formats = [fcs]
	steps = 0
	while 1:
		workdone = 0
		for ifcs, itcs, irtn, ilossy in converters:
			if ifcs == fcs:
				template = [ilossy, 1, [irtn]]
			elif cvs.has_key(ifcs):
				template = cvs[ifcs][:]
				template[2] = template[2][:]
				if ilossy:
					template[0] = ilossy
				template[1] = template[1] + 1
				template[2].append(irtn)
			else:
				continue
			if not cvs.has_key(itcs):
				cvs[itcs] = template
				workdone = 1
			else:
				previous = cvs[itcs]
				if template < previous:
					cvs[itcs] = template
					workdone = 1
		if not workdone:
			break
		steps = steps + 1
		if steps > len(converters):
			print '------------------loop in emunerate_converters--------'
			print 'CONVERTERS:'
			print converters
			print 'RESULTS:'
			print cvs
			raise error, 'Internal error - loop'
	return cvs

def instantiate_converter(args):
	list = args[2]
	cl = RtConverters(list)
	args.append(cl.convert)
	return args

class RtConverters:
	def __init__(self, list):
		self.list = list

	def convert(self, img, w, h):
		for cv in self.list:
			img = cv(img, w, h)
		return img
