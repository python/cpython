from test_support import verbose
import _xdr

fd = 8.01
print '_xdr.pack_float()'
s = _xdr.pack_float(fd)
if verbose:
    print `s`
print '_xdr.unpack_float()'
f = _xdr.unpack_float(s)
if verbose:
    print f
if int(100*f) <> int(100*fd):
    print 'pack_float() <> unpack_float()'

fd = 9900000.9
print '_xdr.pack_double()'
s = _xdr.pack_double(fd)
if verbose:
    print `s`
print '_xdr.unpack_double()'
f = _xdr.unpack_double(s)
if verbose:
    print f

if int(100*f) <> int(100*fd):
    print 'pack_double() <> unpack_double()'
