"""File-like objects that read from or write to a string buffer.

This implements (nearly) all stdio methods.

f = StringIO()      # ready for writing
f = StringIO(buf)   # ready for reading
f.close()           # explicitly release resources held
flag = f.isatty()   # always false
pos = f.tell()      # get current position
f.seek(pos)         # set current position
f.seek(pos, mode)   # mode 0: absolute; 1: relative; 2: relative to EOF
buf = f.read()      # read until EOF
buf = f.read(n)     # read up to n bytes
buf = f.readline()  # read until end of line ('\n') or EOF
list = f.readlines()# list of f.readline() results until EOF
f.truncate([size])  # truncate file at to at most size (default: current pos)
f.write(buf)        # write at current position
f.writelines(list)  # for line in list: f.write(line)
f.getvalue()        # return whole file's contents as a string

Notes:
- Using a real file is often faster (but less convenient).
- There's also a much faster implementation in C, called cStringIO, but
  it's not subclassable.
- fileno() is left unimplemented so that code which uses it triggers
  an exception early.
- Seeking far beyond EOF and then writing will insert real null
  bytes that occupy space in the buffer.
- There's a simple test set (see end of this file).
"""
import types
try:
    from errno import EINVAL
except ImportError:
    EINVAL = 22

__all__ = ["StringIO"]

class StringIO:
    """class StringIO([buffer])

    When a StringIO object is created, it can be initialized to an existing
    string by passing the string to the constructor. If no string is given,
    the StringIO will start empty.

    The StringIO object can accept either Unicode or 8-bit strings, but
    mixing the two may take some care. If both are used, 8-bit strings that
    cannot be interpreted as 7-bit ASCII (that use the 8th bit) will cause
    a UnicodeError to be raised when getvalue() is called.
    """
    def __init__(self, buf = ''):
        # Force self.buf to be a string or unicode
        if type(buf) not in types.StringTypes:
            buf = str(buf)
        self.buf = buf
        self.len = len(buf)
        self.buflist = []
        self.pos = 0
        self.closed = 0
        self.softspace = 0

    def __iter__(self):
        return iter(self.readline, '')

    def close(self):
        """Free the memory buffer."""
        if not self.closed:
            self.closed = 1
            del self.buf, self.pos

    def isatty(self):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        return 0

    def seek(self, pos, mode = 0):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if self.buflist:
            self.buf += ''.join(self.buflist)
            self.buflist = []
        if mode == 1:
            pos += self.pos
        elif mode == 2:
            pos += self.len
        self.pos = max(0, pos)

    def tell(self):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        return self.pos

    def read(self, n = -1):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if self.buflist:
            self.buf += ''.join(self.buflist)
            self.buflist = []
        if n < 0:
            newpos = self.len
        else:
            newpos = min(self.pos+n, self.len)
        r = self.buf[self.pos:newpos]
        self.pos = newpos
        return r

    def readline(self, length=None):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if self.buflist:
            self.buf += ''.join(self.buflist)
            self.buflist = []
        i = self.buf.find('\n', self.pos)
        if i < 0:
            newpos = self.len
        else:
            newpos = i+1
        if length is not None:
            if self.pos + length < newpos:
                newpos = self.pos + length
        r = self.buf[self.pos:newpos]
        self.pos = newpos
        return r

    def readlines(self, sizehint = 0):
        total = 0
        lines = []
        line = self.readline()
        while line:
            lines.append(line)
            total += len(line)
            if 0 < sizehint <= total:
                break
            line = self.readline()
        return lines

    def truncate(self, size=None):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if size is None:
            size = self.pos
        elif size < 0:
            raise IOError(EINVAL, "Negative size not allowed")
        elif size < self.pos:
            self.pos = size
        self.buf = self.getvalue()[:size]

    def write(self, s):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if not s: return
        # Force s to be a string or unicode
        if type(s) not in types.StringTypes:
            s = str(s)
        if self.pos > self.len:
            self.buflist.append('\0'*(self.pos - self.len))
            self.len = self.pos
        newpos = self.pos + len(s)
        if self.pos < self.len:
            if self.buflist:
                self.buf += ''.join(self.buflist)
                self.buflist = []
            self.buflist = [self.buf[:self.pos], s, self.buf[newpos:]]
            self.buf = ''
            if newpos > self.len:
                self.len = newpos
        else:
            self.buflist.append(s)
            self.len = newpos
        self.pos = newpos

    def writelines(self, list):
        self.write(''.join(list))

    def flush(self):
        if self.closed:
            raise ValueError, "I/O operation on closed file"

    def getvalue(self):
        """
        Retrieve the entire contents of the "file" at any time before
        the StringIO object's close() method is called.

        The StringIO object can accept either Unicode or 8-bit strings,
        but mixing the two may take some care. If both are used, 8-bit
        strings that cannot be interpreted as 7-bit ASCII (that use the
        8th bit) will cause a UnicodeError to be raised when getvalue()
        is called.
        """
        if self.buflist:
            self.buf += ''.join(self.buflist)
            self.buflist = []
        return self.buf


# A little test suite

def test():
    import sys
    if sys.argv[1:]:
        file = sys.argv[1]
    else:
        file = '/etc/passwd'
    lines = open(file, 'r').readlines()
    text = open(file, 'r').read()
    f = StringIO()
    for line in lines[:-2]:
        f.write(line)
    f.writelines(lines[-2:])
    if f.getvalue() != text:
        raise RuntimeError, 'write failed'
    length = f.tell()
    print 'File length =', length
    f.seek(len(lines[0]))
    f.write(lines[1])
    f.seek(0)
    print 'First line =', `f.readline()`
    here = f.tell()
    line = f.readline()
    print 'Second line =', `line`
    f.seek(-len(line), 1)
    line2 = f.read(len(line))
    if line != line2:
        raise RuntimeError, 'bad result after seek back'
    f.seek(len(line2), 1)
    list = f.readlines()
    line = list[-1]
    f.seek(f.tell() - len(line))
    line2 = f.read()
    if line != line2:
        raise RuntimeError, 'bad result after seek back from EOF'
    print 'Read', len(list), 'more lines'
    print 'File length =', f.tell()
    if f.tell() != length:
        raise RuntimeError, 'bad length'
    f.close()

if __name__ == '__main__':
    test()
