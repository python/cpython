import string

def MDPrint(str):
	outstr = ''
	for i in str:
		o = ord(i)
		outstr = outstr \
			  + string.hexdigits[(o >> 4) & 0xF] \
			  + string.hexdigits[o & 0xF]
	print outstr,
	

from time import time

def makestr(start, end):
	result = ''
	for i in range(start, end + 1):
		result = result + chr(i)

	return result
	

from md5 import md5

def MDTimeTrial():
	TEST_BLOCK_SIZE = 1000
	TEST_BLOCKS = 10000

	TEST_BYTES = TEST_BLOCK_SIZE * TEST_BLOCKS

	# initialize test data, need temporary string filler
	
	filsiz = 1 << 8
	filler = makestr(0, filsiz-1)
	data = filler * (TEST_BLOCK_SIZE / filsiz);
	data = data + filler[:(TEST_BLOCK_SIZE % filsiz)]

	del filsiz, filler


	# start timer
	print 'MD5 time trial. Processing', TEST_BYTES, 'characters...'
	t1 = time()

	mdContext = md5()

	for i in range(TEST_BLOCKS):
		mdContext.update(data)

	str = mdContext.digest()
	t2 = time()

	MDPrint(str)
	print 'is digest of test input.'
	print 'Seconds to process test input:', t2 - t1
	print 'Characters processed per second:', TEST_BYTES / (t2 - t1)


def MDString(str):
	MDPrint(md5(str).digest())
	print '"' + str + '"'


def MDFile(filename):
	f = open(filename, 'rb');
	mdContext = md5()

	while 1:
		data = f.read(1024)
		if not data:
			break
		mdContext.update(data)

	MDPrint(mdContext.digest())
	print filename


import sys

def MDFilter():
	mdContext = md5()

	while 1:
		data = sys.stdin.read(16)
		if not data:
			break
		mdContext.update(data)

	MDPrint(mdContext.digest())
	print


def MDTestSuite():
	print 'MD5 test suite results:'
	MDString('')
	MDString('a')
	MDString('abc')
	MDString('message digest')
	MDString(makestr(ord('a'), ord('z')))
	MDString(makestr(ord('A'), ord('Z')) \
		  + makestr(ord('a'), ord('z')) \
		  + makestr(ord('0'), ord('9')))
	MDString((makestr(ord('1'), ord('9')) + '0') * 8)

	# Contents of file foo are "abc"
	MDFile('foo')


from sys import argv

# I don't wanna use getopt(), since I want to use the same i/f...
def main():
	if len(argv) == 1:
		MDFilter()
	for arg in argv[1:]:
		if arg[:2] == '-s':
			MDString(arg[2:])
		elif arg == '-t':
			MDTimeTrial()
		elif arg == '-x':
			MDTestSuite()
		else:
			MDFile(arg)

main()
