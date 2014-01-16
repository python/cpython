"""Simple test script for imgfile.c
   Roger E. Masse
"""

from test.test_support import verbose, unlink, findfile, import_module

imgfile = import_module('imgfile', deprecated=True)
import uu


def testimage(name):
    """Run through the imgfile's battery of possible methods
       on the image passed in name.
    """

    import sys
    import os

    outputfile = '/tmp/deleteme'

    # try opening the name directly
    try:
        # This function returns a tuple (x, y, z) where x and y are the size
        # of the image in pixels and z is the number of bytes per pixel. Only
        # 3 byte RGB pixels and 1 byte greyscale pixels are supported.
        sizes = imgfile.getsizes(name)
    except imgfile.error:
        # get a more qualified path component of the script...
        if __name__ == '__main__':
            ourname = sys.argv[0]
        else: # ...or the full path of the module
            ourname = sys.modules[__name__].__file__

        parts = ourname.split(os.sep)
        parts[-1] = name
        name = os.sep.join(parts)
        sizes = imgfile.getsizes(name)
    if verbose:
        print 'Opening test image: %s, sizes: %s' % (name, str(sizes))
    # This function reads and decodes the image on the specified file,
    # and returns it as a python string. The string has either 1 byte
    # greyscale pixels or 4 byte RGBA pixels. The bottom left pixel
    # is the first in the string. This format is suitable to pass
    # to gl.lrectwrite, for instance.
    image = imgfile.read(name)

    # This function writes the RGB or greyscale data in data to
    # image file file. x and y give the size of the image, z is
    # 1 for 1 byte greyscale images or 3 for RGB images (which
    # are stored as 4 byte values of which only the lower three
    # bytes are used). These are the formats returned by gl.lrectread.
    if verbose:
        print 'Writing output file'
    imgfile.write (outputfile, image, sizes[0], sizes[1], sizes[2])


    if verbose:
        print 'Opening scaled test image: %s, sizes: %s' % (name, str(sizes))
    # This function is identical to read but it returns an image that
    # is scaled to the given x and y sizes. If the filter and blur
    # parameters are omitted scaling is done by simply dropping
    # or duplicating pixels, so the result will be less than perfect,
    # especially for computer-generated images.  Alternatively,
    # you can specify a filter to use to smoothen the image after
    # scaling. The filter forms supported are 'impulse', 'box',
    # 'triangle', 'quadratic' and 'gaussian'. If a filter is
    # specified blur is an optional parameter specifying the
    # blurriness of the filter. It defaults to 1.0.  readscaled
    # makes no attempt to keep the aspect ratio correct, so that
    # is the users' responsibility.
    if verbose:
        print 'Filtering with "impulse"'
    simage = imgfile.readscaled (name, sizes[0]/2, sizes[1]/2, 'impulse', 2.0)

    # This function sets a global flag which defines whether the
    # scan lines of the image are read or written from bottom to
    # top (flag is zero, compatible with SGI GL) or from top to
    # bottom(flag is one, compatible with X). The default is zero.
    if verbose:
        print 'Switching to X compatibility'
    imgfile.ttob (1)

    if verbose:
        print 'Filtering with "triangle"'
    simage = imgfile.readscaled (name, sizes[0]/2, sizes[1]/2, 'triangle', 3.0)
    if verbose:
        print 'Switching back to SGI compatibility'
    imgfile.ttob (0)

    if verbose: print 'Filtering with "quadratic"'
    simage = imgfile.readscaled (name, sizes[0]/2, sizes[1]/2, 'quadratic')
    if verbose: print 'Filtering with "gaussian"'
    simage = imgfile.readscaled (name, sizes[0]/2, sizes[1]/2, 'gaussian', 1.0)

    if verbose:
        print 'Writing output file'
    imgfile.write (outputfile, simage, sizes[0]/2, sizes[1]/2, sizes[2])

    os.unlink(outputfile)


def test_main():

    uu.decode(findfile('testrgb.uue'), 'test.rgb')
    uu.decode(findfile('greyrgb.uue'), 'greytest.rgb')

    # Test a 3 byte color image
    testimage('test.rgb')

    # Test a 1 byte greyscale image
    testimage('greytest.rgb')

    unlink('test.rgb')
    unlink('greytest.rgb')

if __name__ == '__main__':
    test_main()
