import sys
from time import millitimer

def main():
	filename = 'film2.video'
	if sys.argv[1:]: filename = sys.argv[1]
	f = open(filename, 'r')

	line = f.readline()
	w, h = eval(line[:-1])
	w2, h2 = w/2, h/2
	size = w2 * h2

	data = data2 = t = t0 = t1 = None
	nframes = 0
	t0 = millitimer()
	while 1:
		line = f.readline()
		if not line: break
		t = eval(line[:-1])
		data = None
		data = f.read(size)
		if len(data) <> size:
			raise EOFError
		dostat(w2, h2, data)
		nframes = nframes+1
	t1 = millitimer()
	
	t = 0.001 * (t1-t0)
	fps = 0.1 * int(10*nframes/t)
	print nframes, 'frames in', t, 'sec. =', fps, 'frames/sec.'

def dostat(w, h, data):
	print
	stat3(w, h, data)

# Statistic op 1: frequencies of byte values
def stat1(w, h, data):
	bins = [0]*256
	for c in data:
		i = ord(c)
		bins[i] = bins[i]+1
	prbins(bins)

def prbins(bins):
	import string
	s = ''
	tot = 0
	for i in range(256):
		tot = tot + bins[i]
		s = s + string.rjust(`bins[i]`, 4)
		if len(s) >= 4*16:
			print s, string.rjust(`tot`, 7)
			s = ''
			tot = 0

# Statistic op 2: run lengths
def stat2(w, h, data):
	runs = []
	for y in range(h):
		count, value = 0, ord(data[y*w])
		for c in data[y*w : y*w+w]:
			i = ord(c)
			if i <> value:
				runs.append(count, value)
				count, value = 0, i
			count = count+1
		runs.append(count, value)
	print len(runs), 'runs =', 0.1 * (10*w*h/len(runs)), 'bytes/run'

# Statistic op 3: frequencies of byte differences
def stat3(w, h, data):
	bins = [0]*256
	prev = 0
	for c in data:
		i = ord(c)
		delta = divmod(i-prev, 256)[1]
		prev = i
		bins[delta] = bins[delta]+1
	prbins(bins)

# Try packing
def packblock(w, h, data):
	res = ''
	for y in range(h):
		res = res + packline(data[y*w : y*w+w])
	return res

def packline(line):
	bytes = []
	for c in line:
		bytes.append(ord(c))
	prev = bytes[0]
	i, n = 1, len(bytes)
	while i < n:
		for pack in (0, 2, 4, 8):
			if pack = 0:
				lo, hi = 0, 0
			else:
				hi = pow(2, pack-1)-1
				lo = -hi-1
			p = prev
			j = i
			count = 0
			while j < n:
				x = bytes[j]
				delta = byte(x-p)
				if not lo <= delta <= hi:
					break
				p = x
				j = j+1

def byte(x): return divmod(x, 256)[1]

main()
