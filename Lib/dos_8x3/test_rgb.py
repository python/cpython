# Testing rgbimg module

import rgbimg, os

error = 'test_rgbimg.error'

print 'RGBimg test suite:'

def findfile(file):
	if os.path.isabs(file): return file
	import sys
	for dn in sys.path:
		fn = os.path.join(dn, file)
		if os.path.exists(fn): return fn
	return file

def testimg(rgb_file, raw_file):
	rgb_file = findfile(rgb_file)
	raw_file = findfile(raw_file)
	width, height = rgbimg.sizeofimage(rgb_file)
	rgb = rgbimg.longimagedata(rgb_file)
	if len(rgb) != width * height * 4:
		raise error, 'bad image length'
	raw = open(raw_file, 'r').read()
	if rgb != raw:
		raise error, \
		      'images don\'t match for '+rgb_file+' and '+raw_file
	for depth in [1, 3, 4]:
		rgbimg.longstoimage(rgb, width, height, depth, '@.rgb')
	os.unlink('@.rgb')

ttob = rgbimg.ttob(0)
if ttob != 0:
	raise error, 'ttob should start out as zero'

testimg('test.rgb', 'test.rawimg')

ttob = rgbimg.ttob(1)
if ttob != 0:
	raise error, 'ttob should be zero'

testimg('test.rgb', 'test.rawimg.rev')

ttob = rgbimg.ttob(0)
if ttob != 1:
	raise error, 'ttob should be one'

ttob = rgbimg.ttob(0)
if ttob != 0:
	raise error, 'ttob should be zero'
