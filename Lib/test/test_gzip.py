
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

f = gzip.GzipFile(filename, 'w') ; f.write(data1) ; f.close()

f = gzip.GzipFile(filename, 'r') ; d = f.read() ; f.close()
assert d == data1

# Append to the previous file
f = gzip.GzipFile(filename, 'a') ; f.write(data2) ; f.close()

f = gzip.GzipFile(filename, 'r') ; d = f.read() ; f.close()
assert d == data1+data2

os.unlink( filename )
