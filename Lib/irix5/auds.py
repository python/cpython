import audio

RATE = 8192

# Initialize the audio stuff
audio.setrate(3)
audio.setoutgain(100)	# for speaker

play = audio.write

def samp(n):
	savegain = audio.getoutgain()
	try:
		audio.setoutgain(0)
		x = raw_input('Hit Enter to sample ' + `n` + ' seconds: ')
		return audio.read(n*RATE)
	finally:
		audio.setoutgain(savegain)

def echo(s, delay, gain):
	return s[:delay] + audio.add(s[delay:], audio.amplify(s, gain, gain))

def save(s, file):
	f = open(file, 'w')
	f.write(s)

def load(file):
	return loadfp(open(file, 'r'))

def loadfp(fp):
	s = ''
	while 1:
		buf = fp.read(16*1024)
		if not buf: break
		s = s + buf
	return s

def unbias(s):
	if not s: return s
	a = audio.chr2num(s)
	sum = 0
	for i in a: sum = sum + i
	bias = (sum + len(a)/2) / len(a)
	print 'Bias value:', bias
	if bias:
		for i in range(len(a)):
			a[i] = a[i] - bias
		s = audio.num2chr(a)
	return s

# Stretch by a/b.
# Think of this as converting the sampling rate from a samples/sec
# to b samples/sec.  Or, if the input is a bytes long, the output
# will be b bytes long.
#
def stretch(s, a, b):
	y = audio.chr2num(s)
	m = len(y)
	out = []
	n = m * b / a
	# i, j will walk through y and out (step 1)
	# ib, ja are i*b, j*a and are kept as close together as possible
	i, ib = 0, 0
	j, ja = 0, 0
	for j in range(n):
		ja = ja+a
		while ib < ja:
			i = i+1
			ib = ib+b
		if i >= m:
			break
		if ib = ja:
			out.append(y[i])
		else:
			out.append((y[i]*(ja-(ib-b)) + y[i-1]*(ib-ja)) / b)
	return audio.num2chr(out)

def sinus(freq): # return a 1-second sine wave
	from math import sin, pi
	factor = 2.0*pi*float(freq)/float(RATE)
	list = range(RATE)
	for i in list:
		list[i] = int(sin(float(i) * factor) * 127.0)
	return audio.num2chr(list)

def softclip(s):
	if '\177' not in s and '\200' not in s:
		return s
	num = audio.chr2num(s)
	extremes = (-128, 127)
	for i in range(1, len(num)-1):
		if num[i] in extremes:
			num[i] = (num[i-1] + num[i+1]) / 2
	return audio.num2chr(num)

def demo():
	gday = load('gday')[1000:6000]
	save(gday, 'gday0')
	gg = [gday]
	for i in range(1, 10):
		for g in gg: play(g)
		g = stretch(gday, 10, 10-i)
		save(g, 'gday' + `i`)
		gg.append(g)
	while 1:
		for g in gg: play(g)
