import sys
import VFile

def main():
	if sys.argv[1:]:
		for filename in sys.argv[1:]:
			process(filename)
	else:
		process('film.video')

def process(filename):
	vin = VFile.VinFile().init(filename)
	print 'File:    ', filename
	print 'Version: ', vin.version
	print 'Size:    ', vin.width, 'x', vin.height
	print 'Pack:    ', vin.packfactor, '; chrom:', vin.chrompack
	print 'Bits:    ', vin.c0bits, vin.c1bits, vin.c2bits
	print 'Format:  ', vin.format
	print 'Offset:  ', vin.offset
	print 'Frame times:',
	n = 0
	t = 0
	while 1:
		try:
			t, data, cdata = vin.getnextframe()
		except EOFError:
			print
			break
		if n%8 == 0:
			sys.stdout.write('\n')
		sys.stdout.write('\t' + `t`)
		n = n+1
	print 'Total', n, 'frames in', t*0.001, 'sec.',
	if t:
		print '-- average', int(n*10000.0/t)*0.1, 'frames/sec',
	print

main()
