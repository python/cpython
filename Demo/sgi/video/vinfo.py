from gl import *
from GL import *
from DEVICE import *
import time
import sys
import getopt

class Struct(): pass
epoch = Struct()
EndOfFile = 'End of file'
bye = 'bye'

def openvideo(filename):
	f = open(filename, 'r')
	line = f.readline()
	if not line: raise EndOfFile
	if line[:4] == 'CMIF': line = f.readline()
	x = eval(line[:-1])
	if len(x) == 3: w, h, pf = x
	else: w, h = x; pf = 2
	return f, w, h, pf

def loadframe(f, w, h, pf):
	line = f.readline()
	if line == '':
		raise EndOfFile
	x = eval(line[:-1])
	if type(x) == type(0) or type(x) == type(0.0):
		tijd = x
		if pf == 0:
			size = w*h*4
		else:
			size = (w/pf) * (h/pf)
	else:
		tijd, size = x
	f.seek(size, 1)
	return tijd

def main():
	delta = 0
	short = 0
	try:
		opts, names = getopt.getopt(sys.argv[1:], 'ds')
	except getopt.error, msg:
		sys.stderr.write(msg + '\n')
		sys.stderr.write('usage: vinfo [-d] [-s] [file] ...\n')
		sys.exit(2)
	for opt, arg in opts:
		if opt == '-d': delta = 1	# print delta between frames
		elif opt == '-s': short = 1	# short: don't print times
	if names == []:
		names = ['film.video']
	for name in names:
	    try:
		f, w, h, pf = openvideo(name)
	    except:
	    	sys.stderr.write(name + ': cannot open\n')
	    	continue
	    if pf == 0:
	        size = w*h*4
	    else:
	        size = (w/pf) * (h/pf)
	    print name, ':', w, 'x', h, '; pf =', pf, ', size =', size,
	    if pf == 0: print '(color)',
	    print
	    num = 0
	    try:
	    	otijd = 0
		while not short:
		    try:
			tijd = loadframe(f, w, h, pf)
			if delta: print '\t' + `tijd-otijd`,
			else: print '\t' + `tijd`,
			otijd = tijd
			num = num + 1
			if num % 8 == 0:
				print
		    except EndOfFile:
			raise bye
	    except bye:
		pass
	    if num % 8 <> 0:
		print
	    f.close()

main()
