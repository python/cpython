#!/ufs/guido/bin/sgi/python3.3
from gl import *
from GL import *
from DEVICE import *
import time
import sys

class Struct(): pass
epoch = Struct()
EndOfFile = 'End of file'
bye = 'bye'

def openvideo(name):
    f = open(name, 'r')
    w, h = eval(f.readline()[:-1])
    return f, w, h
def loadframe(f, w, h):
    tijd = f.readline()
    if tijd = '':
	raise EndOfFile
    tijd = eval(tijd[:-1])
    f.seek(w*h*4,1)
    return tijd
def saveframe(name, w, h, tijd, data):
    f = open(name, 'w')
    f.write(`w,h` + '\n')
    f.write(`tijd` + '\n')
    f.write(data)
    f.close()
def main():
	if len(sys.argv) > 1:
		names = sys.argv[1:]
	else:
		names = ['film.video']
	for name in names:
	    f, w, h = openvideo(name)
	    print name+': '+`w`+'x'+`h`
	    num = 0
	    try:
		while 1:
		    try:
			tijd = loadframe(f, w, h)
			print '\t', tijd,
			num = num + 1
			if num % 8 = 0:
				print
		    except EndOfFile:
			raise bye
	    except bye:
		pass
	    if num % 8 <> 0:
		print
	    f.close()

main()
