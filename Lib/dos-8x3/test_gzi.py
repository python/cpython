
import sys, os
import gzip, tempfile

filename = tempfile.mktemp()

data1 = """  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

"""

data2 = """/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.cdrom.com/pub/infozip/zlib/ */
/* See http://www.winimage.com/zLibDll for Windows */
"""

f = gzip.GzipFile(filename, 'wb') ; f.write(data1 * 50) ; f.close()

f = gzip.GzipFile(filename, 'rb') ; d = f.read() ; f.close()
assert d == data1*50

# Append to the previous file
f = gzip.GzipFile(filename, 'ab') ; f.write(data2 * 15) ; f.close()

f = gzip.GzipFile(filename, 'rb') ; d = f.read() ; f.close()
assert d == (data1*50) + (data2*15)

# Try .readline() with varying line lengths

f = gzip.GzipFile(filename, 'rb')
line_length = 0
while 1:
    L = f.readline( line_length )
    if L == "" and line_length != 0: break
    assert len(L) <= line_length
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


os.unlink( filename )
