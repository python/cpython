# Implement 'jpeg' interface using SGI's compression library

# XXX Options 'smooth' and 'optimize' are ignored.

# XXX It appears that compressing grayscale images doesn't work right;
# XXX the resulting file causes weirdness.

class error(Exception):
    pass

options = {'quality': 75, 'optimize': 0, 'smooth': 0, 'forcegray': 0}

comp = None
decomp = None

def compress(imgdata, width, height, bytesperpixel):
    global comp
    import cl
    if comp is None: comp = cl.OpenCompressor(cl.JPEG)
    if bytesperpixel == 1:
        format = cl.GRAYSCALE
    elif bytesperpixel == 4:
        format = cl.RGBX
    if options['forcegray']:
        iformat = cl.GRAYSCALE
    else:
        iformat = cl.YUV
    # XXX How to support 'optimize'?
    params = [cl.IMAGE_WIDTH, width, cl.IMAGE_HEIGHT, height,
              cl.ORIGINAL_FORMAT, format,
              cl.ORIENTATION, cl.BOTTOM_UP,
              cl.QUALITY_FACTOR, options['quality'],
              cl.INTERNAL_FORMAT, iformat,
             ]
    comp.SetParams(params)
    jpegdata = comp.Compress(1, imgdata)
    return jpegdata

def decompress(jpegdata):
    global decomp
    import cl
    if decomp is None: decomp = cl.OpenDecompressor(cl.JPEG)
    headersize = decomp.ReadHeader(jpegdata)
    params = [cl.IMAGE_WIDTH, 0, cl.IMAGE_HEIGHT, 0, cl.INTERNAL_FORMAT, 0]
    decomp.GetParams(params)
    width, height, format = params[1], params[3], params[5]
    if format == cl.GRAYSCALE or options['forcegray']:
        format = cl.GRAYSCALE
        bytesperpixel = 1
    else:
        format = cl.RGBX
        bytesperpixel = 4
    # XXX How to support 'smooth'?
    params = [cl.ORIGINAL_FORMAT, format,
              cl.ORIENTATION, cl.BOTTOM_UP,
              cl.FRAME_BUFFER_SIZE, width*height*bytesperpixel]
    decomp.SetParams(params)
    imgdata = decomp.Decompress(1, jpegdata)
    return imgdata, width, height, bytesperpixel

def setoption(name, value):
    if type(value) is not type(0):
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
