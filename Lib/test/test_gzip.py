from test.test_support import verify, TESTFN
import sys, os
import gzip

filename = TESTFN

data1 = """  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

"""

data2 = """/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/
/* See http://www.winimage.com/zLibDll for Windows */
"""

f = gzip.GzipFile(filename, 'wb') ; f.write(data1 * 50)

# Try flush and fileno.
f.flush()
f.fileno()
if hasattr(os, 'fsync'):
    os.fsync(f.fileno())
f.close()

# Try reading.
f = gzip.GzipFile(filename, 'r') ; d = f.read() ; f.close()
verify(d == data1*50)

# Append to the previous file
f = gzip.GzipFile(filename, 'ab') ; f.write(data2 * 15) ; f.close()

f = gzip.GzipFile(filename, 'rb') ; d = f.read() ; f.close()
verify(d == (data1*50) + (data2*15))

# Try .readline() with varying line lengths

f = gzip.GzipFile(filename, 'rb')
line_length = 0
while 1:
    L = f.readline(line_length)
    if L == "" and line_length != 0: break
    verify(len(L) <= line_length)
    line_length = (line_length + 1) % 50
f.close()

# Try .readlines()

f = gzip.GzipFile(filename, 'rb')
L = f.readlines()
f.close()

f = gzip.GzipFile(filename, 'rb')
while 1:
    L = f.readlines(150)
    if L == []: break
f.close()

# Try seek, read test

f = gzip.GzipFile(filename)
while 1:
    oldpos = f.tell()
    line1 = f.readline()
    if not line1: break
    newpos = f.tell()
    f.seek(oldpos)  # negative seek
    if len(line1)>10:
        amount = 10
    else:
        amount = len(line1)
    line2 = f.read(amount)
    verify(line1[:amount] == line2)
    f.seek(newpos)  # positive seek
f.close()

# Try seek, write test
f = gzip.GzipFile(filename, 'w')
for pos in range(0, 256, 16):
    f.seek(pos)
    f.write('GZ\n')
f.close()

f = gzip.GzipFile(filename, 'r')
verify(f.myfileobj.mode == 'rb')
f.close()

os.unlink(filename)
