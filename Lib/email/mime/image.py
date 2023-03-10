# Copyright (C) 2001-2006 Python Software Foundation
# Author: Barry Warsaw
# Contact: email-sig@python.org

"""Class representing image/* type MIME documents."""

__all__ = ['MIMEImage']

from email import encoders
from email.mime.nonmultipart import MIMENonMultipart


class MIMEImage(MIMENonMultipart):
    """Class for generating image/* type MIME documents."""

    def __init__(self, _imagedata, _subtype=None,
                 _encoder=encoders.encode_base64, *, policy=None, **_params):
        """Create an image/* type MIME document.

        _imagedata contains the bytes for the raw image data.  If the data
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
        _subtype = _infer_subtype(_imagedata) if _subtype is None else _subtype
        if _subtype is None:
            raise TypeError('Could not guess image MIME subtype')
        MIMENonMultipart.__init__(self, 'image', _subtype, policy=policy,
                                  **_params)
        self.set_payload(_imagedata)
        _encoder(self)


# Originally from the imghdr module.
def _infer_subtype(h: bytes) -> str:
    # JPEG data with JFIF or Exif markers; and raw JPEG
    if h[6:10] in (b'JFIF', b'Exif') or h.startswith(b'\xff\xd8\xff\xdb'):
        return 'jpeg'

    if h.startswith(b'\211PNG\r\n\032\n'):
        return 'png'

    # GIF ('87 and '89 variants)
    if h.startswith((b'GIF87a', b'GIF89a')):
        return 'gif'
    
    # TIFF (can be in Motorola or Intel byte order)
    if h.startswith((b'MM', b'II')):
        return 'tiff'

    # SGI image library
    if h.startswith(b'\001\332'):
        return 'rgb'

    if h.startswith(b'P') and len(h) >= 3 and h[2] in b' \t\n\r':
        # PBM (portable bitmap)
        if h[1] in b'14':
            return 'pbm'

        # PGM (portable graymap)
        if h[1] in b'25':
            return 'pgm'

        # PPM (portable pixmap)
        if h[1] in b'36':
            return 'ppm'

    # Sun raster file
    if h.startswith(b'\x59\xA6\x6A\x95'):
        return 'rast'

    # X bitmap (X10 or X11)
    if h.startswith(b'#define '):
        return 'xbm'

    if h.startswith(b'BM'):
        return 'bmp'

    if h.startswith(b'RIFF') and h[8:12] == b'WEBP':
        return 'webp'

    if h.startswith(b'\x76\x2f\x31\x01'):
        return 'exr'

    return None