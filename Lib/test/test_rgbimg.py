# Testing rgbimg module

import rgbimg, os, uu

from test_support import verbose, unlink, findfile

class error(Exception):
    pass

print 'RGBimg test suite:'

def testimg(rgb_file, raw_file):
    rgb_file = findfile(rgb_file)
    raw_file = findfile(raw_file)
    width, height = rgbimg.sizeofimage(rgb_file)
    rgb = rgbimg.longimagedata(rgb_file)
    if len(rgb) != width * height * 4:
        raise error, 'bad image length'
    raw = open(raw_file, 'rb').read()
    if rgb != raw:
        raise error, \
              'images don\'t match for '+rgb_file+' and '+raw_file
    for depth in [1, 3, 4]:
        rgbimg.longstoimage(rgb, width, height, depth, '@.rgb')
    os.unlink('@.rgb')

table = [
    ('testrgb'+os.extsep+'uue', 'test'+os.extsep+'rgb'),
    ('testimg'+os.extsep+'uue', 'test'+os.extsep+'rawimg'),
    ('testimgr'+os.extsep+'uue', 'test'+os.extsep+'rawimg'+os.extsep+'rev'),
    ]
for source, target in table:
    source = findfile(source)
    target = findfile(target)
    if verbose:
        print "uudecoding", source, "->", target, "..."
    uu.decode(source, target)

if verbose:
    print "testing..."

ttob = rgbimg.ttob(0)
if ttob != 0:
    raise error, 'ttob should start out as zero'

testimg('test'+os.extsep+'rgb', 'test'+os.extsep+'rawimg')

ttob = rgbimg.ttob(1)
if ttob != 0:
    raise error, 'ttob should be zero'

testimg('test'+os.extsep+'rgb', 'test'+os.extsep+'rawimg'+os.extsep+'rev')

ttob = rgbimg.ttob(0)
if ttob != 1:
    raise error, 'ttob should be one'

ttob = rgbimg.ttob(0)
if ttob != 0:
    raise error, 'ttob should be zero'

for source, target in table:
    unlink(findfile(target))
