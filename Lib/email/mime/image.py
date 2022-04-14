# Copyright (C) 2001-2006 Python Software Foundation
# Author: Barry Warsaw
# Contact: email-sig@python.org

"""Class representing image/* type MIME documents."""

__all__ = ['MIMEImage']

from email import encoders
from email.mime.nonmultipart import MIMENonMultipart


# Originally from the imghdr module.
def _what(h):
    for tf in tests:
        if res := tf(h):
            return res
    else:
        return None

tests = []

def _test_jpeg(h):
    """JPEG data with JFIF or Exif markers; and raw JPEG"""
    if h[6:10] in (b'JFIF', b'Exif'):
        return 'jpeg'
    elif h[:4] == b'\xff\xd8\xff\xdb':
        return 'jpeg'

tests.append(_test_jpeg)

def _test_png(h):
    if h.startswith(b'\211PNG\r\n\032\n'):
        return 'png'

tests.append(_test_png)

def _test_gif(h):
    """GIF ('87 and '89 variants)"""
    if h[:6] in (b'GIF87a', b'GIF89a'):
        return 'gif'

tests.append(_test_gif)

def _test_tiff(h):
    """TIFF (can be in Motorola or Intel byte order)"""
    if h[:2] in (b'MM', b'II'):
        return 'tiff'

tests.append(_test_tiff)

def _test_rgb(h):
    """SGI image library"""
    if h.startswith(b'\001\332'):
        return 'rgb'

tests.append(_test_rgb)

def _test_pbm(h):
    """PBM (portable bitmap)"""
    if len(h) >= 3 and \
        h[0] == ord(b'P') and h[1] in b'14' and h[2] in b' \t\n\r':
        return 'pbm'

tests.append(_test_pbm)

def _test_pgm(h):
    """PGM (portable graymap)"""
    if len(h) >= 3 and \
        h[0] == ord(b'P') and h[1] in b'25' and h[2] in b' \t\n\r':
        return 'pgm'

tests.append(_test_pgm)

def _test_ppm(h):
    """PPM (portable pixmap)"""
    if len(h) >= 3 and \
        h[0] == ord(b'P') and h[1] in b'36' and h[2] in b' \t\n\r':
        return 'ppm'

tests.append(_test_ppm)

def _test_rast(h):
    """Sun raster file"""
    if h.startswith(b'\x59\xA6\x6A\x95'):
        return 'rast'

tests.append(_test_rast)

def _test_xbm(h):
    """X bitmap (X10 or X11)"""
    if h.startswith(b'#define '):
        return 'xbm'

tests.append(_test_xbm)

def _test_bmp(h):
    if h.startswith(b'BM'):
        return 'bmp'

tests.append(_test_bmp)

def _test_webp(h):
    if h.startswith(b'RIFF') and h[8:12] == b'WEBP':
        return 'webp'

tests.append(_test_webp)

def _test_exr(h):
    if h.startswith(b'\x76\x2f\x31\x01'):
        return 'exr'

tests.append(_test_exr)


class MIMEImage(MIMENonMultipart):
    """Class for generating image/* type MIME documents."""

    def __init__(self, _imagedata, _subtype=None,
                 _encoder=encoders.encode_base64, *, policy=None, **_params):
        """Create an image/* type MIME document.

        _imagedata is a string containing the raw image data.  If the data
        type can be detected (jpeg, png, gif, tiff, rgb, pbm, pgm, ppm,
        rast, xbm, bmp, webp, and exr attempted), then the subtype will be
        automatically included in the Content-Type header. Otherwise, you can
        specify the specific image subtype via the _subtype parameter.

        _encoder is a function which will perform the actual encoding for
        transport of the image data.  It takes one argument, which is this
        Image instance.  It should use get_payload() and set_payload() to
        change the payload to the encoded form.  It should also add any
        Content-Transfer-Encoding or other headers to the message as
        necessary.  The default encoding is Base64.

        Any additional keyword arguments are passed to the base class
        constructor, which turns them into parameters on the Content-Type
        header.
        """
        if _subtype is None:
            if (_subtype := _what(_imagedata)) is None:
                raise TypeError('Could not guess image MIME subtype')
        MIMENonMultipart.__init__(self, 'image', _subtype, policy=policy,
                                  **_params)
        self.set_payload(_imagedata)
        _encoder(self)
