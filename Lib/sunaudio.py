# Module 'sunaudio' -- interpret sun audio headers

import audio

MAGIC = '.snd'

error = 'sunaudio sound header conversion error'


# convert a 4-char value to integer

def c2i(data):
	if type(data) <> type('') or len(data) <> 4:
		raise error, 'c2i: bad arg (not string[4])'
	bytes = audio.chr2num(data)
	for i in (1, 2, 3):
		if bytes[i] < 0:
			bytes[i] = bytes[i] + 256
	return ((bytes[0]*256 + bytes[1])*256 + bytes[2])*256 + bytes[3]


# read a sound header from an open file

def gethdr(fp):
	if fp.read(4) <> MAGIC:
		raise error, 'gethdr: bad magic word'
	hdr_size = c2i(fp.read(4))
	data_size = c2i(fp.read(4))
	encoding = c2i(fp.read(4))
	sample_rate = c2i(fp.read(4))
	channels = c2i(fp.read(4))
	excess = hdr_size - 24
	if excess < 0:
		raise error, 'gethdr: bad hdr_size'
	if excess > 0:
		info = fp.read(excess)
	else:
		info = ''
	return (data_size, encoding, sample_rate, channels, info)


# read and print the sound header of a named file

def printhdr(file):
	hdr = gethdr(open(file, 'r'))
	data_size, encoding, sample_rate, channels, info = hdr
	while info[-1:] = '\0':
		info = info[:-1]
	print 'File name:  ', file
	print 'Data size:  ', data_size
	print 'Encoding:   ', encoding
	print 'Sample rate:', sample_rate
	print 'Channels:   ', channels
	print 'Info:       ', `info`
