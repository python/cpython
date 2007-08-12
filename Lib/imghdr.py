"""Recognize image file formats based on their first few bytes."""

__all__ = ["what"]

#-------------------------#
# Recognize image headers #
#-------------------------#

def what(file, h=None):
    if h is None:
        if type(file) == type(''):
            f = open(file, 'rb')
            h = f.read(32)
        else:
            location = file.tell()
            h = file.read(32)
            file.seek(location)
            f = None
    else:
        f = None
    try:
        for tf in tests:
            res = tf(h, f)
            if res:
                return res
    finally:
        if f: f.close()
    return None


#---------------------------------#
# Subroutines per image file type #
#---------------------------------#

tests = []

def test_rgb(h, f):
    """SGI image library"""
    if h[:2] == b'\001\332':
        return 'rgb'

tests.append(test_rgb)

def test_gif(h, f):
    """GIF ('87 and '89 variants)"""
    if h[:6] in (b'GIF87a', b'GIF89a'):
        return 'gif'

tests.append(test_gif)

def test_pbm(h, f):
    """PBM (portable bitmap)"""
    if len(h) >= 3 and \
        h[0] == ord('P') and h[1] in b'14' and h[2] in b' \t\n\r':
        return 'pbm'

tests.append(test_pbm)

def test_pgm(h, f):
    """PGM (portable graymap)"""
    if len(h) >= 3 and \
        h[0] == ord('P') and h[1] in b'25' and h[2] in b' \t\n\r':
        return 'pgm'

tests.append(test_pgm)

def test_ppm(h, f):
    """PPM (portable pixmap)"""
    if len(h) >= 3 and \
        h[0] == ord('P') and h[1] in b'36' and h[2] in b' \t\n\r':
        return 'ppm'

tests.append(test_ppm)

def test_tiff(h, f):
    """TIFF (can be in Motorola or Intel byte order)"""
    if h[:2] in (b'MM', b'II'):
        return 'tiff'

tests.append(test_tiff)

def test_rast(h, f):
    """Sun raster file"""
    if h[:4] == b'\x59\xA6\x6A\x95':
        return 'rast'

tests.append(test_rast)

def test_xbm(h, f):
    """X bitmap (X10 or X11)"""
    s = b'#define '
    if h[:len(s)] == s:
        return 'xbm'

tests.append(test_xbm)

def test_jpeg(h, f):
    """JPEG data in JFIF format"""
    if h[6:10] == b'JFIF':
        return 'jpeg'

tests.append(test_jpeg)

def test_exif(h, f):
    """JPEG data in Exif format"""
    if h[6:10] == b'Exif':
        return 'jpeg'

tests.append(test_exif)

def test_bmp(h, f):
    if h[:2] == b'BM':
        return 'bmp'

tests.append(test_bmp)

def test_png(h, f):
    if h[:8] == b'\211PNG\r\n\032\n':
        return 'png'

tests.append(test_png)

#--------------------#
# Small test program #
#--------------------#

def test():
    import sys
    recursive = 0
    if sys.argv[1:] and sys.argv[1] == '-r':
        del sys.argv[1:2]
        recursive = 1
    try:
        if sys.argv[1:]:
            testall(sys.argv[1:], recursive, 1)
        else:
            testall(['.'], recursive, 1)
    except KeyboardInterrupt:
        sys.stderr.write('\n[Interrupted]\n')
        sys.exit(1)

def testall(list, recursive, toplevel):
    import sys
    import os
    for filename in list:
        if os.path.isdir(filename):
            print(filename + '/:', end=' ')
            if recursive or toplevel:
                print('recursing down:')
                import glob
                names = glob.glob(os.path.join(filename, '*'))
                testall(names, recursive, 0)
            else:
                print('*** directory (use -r) ***')
        else:
            print(filename + ':', end=' ')
            sys.stdout.flush()
            try:
                print(what(filename))
            except IOError:
                print('*** not found ***')

if __name__ == '__main__':
    test()
