# Convert "arbitrary" image files to rgb files (SGI's image format).
# Input may be compressed.
# The uncompressed file type may be PBM, PGM, PPM, GIF, TIFF, or Sun raster.
# An exception is raised if the file is not of a recognized type.
# Returned filename is either the input filename or a temporary filename;
# in the latter case the caller must ensure that it is removed.
# Other temporary files used are removed by the function.
from warnings import warnpy3k
warnpy3k("the torgb module has been removed in Python 3.0", stacklevel=2)
del warnpy3k

import os
import tempfile
import pipes
import imghdr

table = {}

t = pipes.Template()
t.append('fromppm $IN $OUT', 'ff')
table['ppm'] = t

t = pipes.Template()
t.append('(PATH=$PATH:/ufs/guido/bin/sgi; exec pnmtoppm)', '--')
t.append('fromppm $IN $OUT', 'ff')
table['pnm'] = t
table['pgm'] = t
table['pbm'] = t

t = pipes.Template()
t.append('fromgif $IN $OUT', 'ff')
table['gif'] = t

t = pipes.Template()
t.append('tifftopnm', '--')
t.append('(PATH=$PATH:/ufs/guido/bin/sgi; exec pnmtoppm)', '--')
t.append('fromppm $IN $OUT', 'ff')
table['tiff'] = t

t = pipes.Template()
t.append('rasttopnm', '--')
t.append('(PATH=$PATH:/ufs/guido/bin/sgi; exec pnmtoppm)', '--')
t.append('fromppm $IN $OUT', 'ff')
table['rast'] = t

t = pipes.Template()
t.append('djpeg', '--')
t.append('(PATH=$PATH:/ufs/guido/bin/sgi; exec pnmtoppm)', '--')
t.append('fromppm $IN $OUT', 'ff')
table['jpeg'] = t

uncompress = pipes.Template()
uncompress.append('uncompress', '--')


class error(Exception):
    pass

def torgb(filename):
    temps = []
    ret = None
    try:
        ret = _torgb(filename, temps)
    finally:
        for temp in temps[:]:
            if temp != ret:
                try:
                    os.unlink(temp)
                except os.error:
                    pass
                temps.remove(temp)
    return ret

def _torgb(filename, temps):
    if filename[-2:] == '.Z':
        (fd, fname) = tempfile.mkstemp()
        os.close(fd)
        temps.append(fname)
        sts = uncompress.copy(filename, fname)
        if sts:
            raise error, filename + ': uncompress failed'
    else:
        fname = filename
    try:
        ftype = imghdr.what(fname)
    except IOError, msg:
        if type(msg) == type(()) and len(msg) == 2 and \
                type(msg[0]) == type(0) and type(msg[1]) == type(''):
            msg = msg[1]
        if type(msg) is not type(''):
            msg = repr(msg)
        raise error, filename + ': ' + msg
    if ftype == 'rgb':
        return fname
    if ftype is None or not table.has_key(ftype):
        raise error, '%s: unsupported image file type %r' % (filename, ftype)
    (fd, temp) = tempfile.mkstemp()
    os.close(fd)
    sts = table[ftype].copy(fname, temp)
    if sts:
        raise error, filename + ': conversion to rgb failed'
    return temp
