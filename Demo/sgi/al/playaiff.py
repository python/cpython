import aiff
import al
import sys
import time

def main():
	v = 1
	c = al.newconfig()
	nchannels = c.getchannels()
	nsampframes = 0 # ???
	sampwidth = c.getwidth()
	samprate = 0.0 # unknown
	filename = sys.argv[1]
	f = open(filename, 'r')
	type, totalsize = aiff.read_chunk_header(f)
	if type <> 'FORM':
		raise aiff.Error, 'FORM chunk expected at start of file'
	aiff.read_form_chunk(f)
	while 1:
		try:
			type, size = aiff.read_chunk_header(f)
		except EOFError:
			break
		if v: print 'header:', `type`, size
		if type == 'COMM':
			nchannels, nsampframes, sampwidth, samprate = \
				aiff.read_comm_chunk(f)
			if v: print nchannels, nsampframes, sampwidth, samprate
		elif type == 'SSND':
			offset, blocksize = aiff.read_ssnd_chunk(f)
			if v: print offset, blocksize
			data = f.read(size-8)
			if size%2: void = f.read(1)
			p = makeport(nchannels, sampwidth, samprate)
			play(p, data, offset, blocksize)
		elif type in aiff.skiplist:
			aiff.skip_chunk(f, size)
		else:
			raise aiff.Error, 'bad chunk type ' + type

def makeport(nchannels, sampwidth, samprate):
	c = al.newconfig()
	c.setchannels(nchannels)
	c.setwidth(sampwidth/8)
	# can't set the rate...
	p = al.openport('', 'w', c)
	return p

def play(p, data, offset, blocksize):
	data = data[offset:]
	p.writesamps(data)
	while p.getfilled() > 0: time.sleep(0.01)

main()
