# Implement 'jpeg' interface using SGI's compression library

# XXX Options 'smooth' and 'optimize' are ignored.

# XXX It appears that compressing grayscale images doesn't work right;
# XXX the resulting file causes weirdness.

error = 'jpeg.error' # Exception

options = {'quality': 75, 'optimize': 0, 'smooth': 0, 'forcegray': 0}

comp = None
decomp = None

def compress(imgdata, width, height, bytesperpixel):
	global comp
	import cl, CL
	if comp is None: comp = cl.OpenCompressor(CL.JPEG)
	if bytesperpixel == 1:
		format = CL.GRAYSCALE
	elif bytesperpixel == 4:
		format = CL.RGBX
	if options['forcegray']:
		iformat = CL.GRAYSCALE
	else:
		iformat = CL.YUV
	# XXX How to support 'optimize'?
	params = [CL.IMAGE_WIDTH, width, CL.IMAGE_HEIGHT, height, \
		  CL.ORIGINAL_FORMAT, format, \
		  CL.ORIENTATION, CL.BOTTOM_UP, \
		  CL.QUALITY_FACTOR, options['quality'], \
		  CL.INTERNAL_FORMAT, iformat, \
		 ]
	comp.SetParams(params)
	jpegdata = comp.Compress(1, imgdata)
	return jpegdata

def decompress(jpegdata):
	global decomp
	import cl, CL
	if decomp is None: decomp = cl.OpenDecompressor(CL.JPEG)
	headersize = decomp.ReadHeader(jpegdata)
	params = [CL.IMAGE_WIDTH, 0, CL.IMAGE_HEIGHT, 0, CL.INTERNAL_FORMAT, 0]
	decomp.GetParams(params)
	width, height, format = params[1], params[3], params[5]
	if format == CL.GRAYSCALE or options['forcegray']:
		format = CL.GRAYSCALE
		bytesperpixel = 1
	else:
		format = CL.RGBX
		bytesperpixel = 4
	# XXX How to support 'smooth'?
	params = [CL.ORIGINAL_FORMAT, format, \
		  CL.ORIENTATION, CL.BOTTOM_UP, \
		  CL.FRAME_BUFFER_SIZE, width*height*bytesperpixel]
	decomp.SetParams(params)
	imgdata = decomp.Decompress(1, jpegdata)
	return imgdata, width, height, bytesperpixel

def setoption(name, value):
	if type(value) <> type(0):
		raise TypeError, 'jpeg.setoption: numeric options only'
	if name == 'forcegrey':
		name = 'forcegray'
	if not options.has_key(name):
		raise KeyError, 'jpeg.setoption: unknown option name'
	options[name] = int(value)

def test():
	import sys
	if sys.argv[1:2] == ['-g']:
		del sys.argv[1]
		setoption('forcegray', 1)
	if not sys.argv[1:]:
		sys.argv.append('/usr/local/images/data/jpg/asterix.jpg')
	for file in sys.argv[1:]:
		show(file)

def show(file):
	import gl, GL, DEVICE
	jpegdata = open(file, 'r').read()
	imgdata, width, height, bytesperpixel = decompress(jpegdata)
	gl.foreground()
	gl.prefsize(width, height)
	win = gl.winopen(file)
	if bytesperpixel == 1:
		gl.cmode()
		gl.pixmode(GL.PM_SIZE, 8)
		gl.gconfig()
		for i in range(256):
			gl.mapcolor(i, i, i, i)
	else:
		gl.RGBmode()
		gl.pixmode(GL.PM_SIZE, 32)
		gl.gconfig()
	gl.qdevice(DEVICE.REDRAW)
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.lrectwrite(0, 0, width-1, height-1, imgdata)
	while 1:
		dev, val = gl.qread()
		if dev in (DEVICE.ESCKEY, DEVICE.WINSHUT, DEVICE.WINQUIT):
			break
		if dev == DEVICE.REDRAW:
			gl.lrectwrite(0, 0, width-1, height-1, imgdata)
	gl.winclose(win)
	# Now test the compression and write the result to a fixed filename
	newjpegdata = compress(imgdata, width, height, bytesperpixel)
	open('/tmp/j.jpg', 'w').write(newjpegdata)
