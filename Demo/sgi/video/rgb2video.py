import sys
import VFile
import getopt
import imgfile
import string
import imgconv

def main():
	format = None
	interval = 40
	outfile = 'film.video'

	try:
		opts, args = getopt.getopt(sys.argv[1:], 'f:i:o:')
	except getopt.error:
		usage()
		sys.exit(1)
	for opt, arg in opts:
		if opt == '-f':
			format = arg
		elif opt == '-i':
			interval = string.atoi(arg)
		elif opt == '-o':
			outfile = arg
		else:
			usage()
			sys.exit(1)
	if not args:
		usage()
		sys.exit(1)
	
	xsize, ysize, zsize = imgfile.getsizes(args[0])
	nxsize = xsize
	
	if zsize == 3:
		oformat = 'rgb'
	elif zsize == 1:
		oformat = 'grey'
		if xsize % 4:
			addbytes = 4-(xsize%4)
			nxsize = xsize + addbytes
			print 'rgb2video: add',addbytes,'pixels per line'
	else:
		print 'rgb2video: incorrect number of planes:',zsize
		sys.exit(1)

	if format == None:
		format = oformat
	cfunc = imgconv.getconverter(oformat, format)

	vout = VFile.VoutFile(outfile)
	vout.format = format
	vout.width = nxsize
	vout.height = ysize
	vout.writeheader()
	t = 0
	sys.stderr.write('Processing ')
	for img in args:
		sys.stderr.write(img + ' ')
		if imgfile.getsizes(img) <> (xsize, ysize, zsize):
			print 'rgb2video: Image is different size:', img
			sys.exit(1)
		data = imgfile.read(img)
		if xsize <> nxsize:
			ndata = ''
			for i in range(0,len(data), xsize):
				curline = data[i:i+xsize]
				ndata = ndata + curline + ('\0'*(nxsize-xsize))
			data = ndata
		vout.writeframe(t, cfunc(data, nxsize, ysize), None)
		t = t + interval
	sys.stderr.write('\n')
	vout.close()

def usage():
	print 'Usage: rgb2video [-o output] [-i frameinterval] [-f format] rgbfile ...'
	
main()
		
