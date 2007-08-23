# _emx_link.py

# Written by Andrew I MacIntyre, December 2002.

"""_emx_link.py is a simplistic emulation of the Unix link(2) library routine
for creating so-called hard links.  It is intended to be imported into
the os module in place of the unimplemented (on OS/2) Posix link()
function (os.link()).

We do this on OS/2 by implementing a file copy, with link(2) semantics:-
  - the target cannot already exist;
  - we hope that the actual file open (if successful) is actually
    atomic...

Limitations of this approach/implementation include:-
  - no support for correct link counts (EMX stat(target).st_nlink
    is always 1);
  - thread safety undefined;
  - default file permissions (r+w) used, can't be over-ridden;
  - implemented in Python so comparatively slow, especially for large
    source files;
  - need sufficient free disk space to store the copy.

Behaviour:-
  - any exception should propagate to the caller;
  - want target to be an exact copy of the source, so use binary mode;
  - returns None, same as os.link() which is implemented in posixmodule.c;
  - target removed in the event of a failure where possible;
  - given the motivation to write this emulation came from trying to
    support a Unix resource lock implementation, where minimal overhead
    during creation of the target is desirable and the files are small,
    we read a source block before attempting to create the target so that
    we're ready to immediately write some data into it.
"""

import os
import errno

__all__ = ['link']

def link(source, target):
    """link(source, target) -> None

    Attempt to hard link the source file to the target file name.
    On OS/2, this creates a complete copy of the source file.
    """

    s = os.open(source, os.O_RDONLY | os.O_BINARY)
    if os.isatty(s):
        raise OSError(errno.EXDEV, 'Cross-device link')
    data = os.read(s, 1024)

    try:
        t = os.open(target, os.O_WRONLY | os.O_BINARY | os.O_CREAT | os.O_EXCL)
    except OSError:
        os.close(s)
        raise

    try:
        while data:
            os.write(t, data)
            data = os.read(s, 1024)
    except OSError:
        os.close(s)
        os.close(t)
        os.unlink(target)
        raise

    os.close(s)
    os.close(t)

if __name__ == '__main__':
    import sys
    try:
        link(sys.argv[1], sys.argv[2])
    except IndexError:
        print('Usage: emx_link <source> <target>')
    except OSError:
        print('emx_link: %s' % str(sys.exc_info()[1]))
