import _xdr

verbose = 0
if __name__ == '__main__':
    verbose = 1

fd = 8.01
s = _xdr.pack_float(fd)
f = _xdr.unpack_float(s)

if verbose:
    print f
if int(100*f) <> int(100*fd):
    print 'pack_float() <> unpack_float()'

fd = 9900000.9
s = _xdr.pack_double(fd)
f = _xdr.unpack_double(s)

if verbose:
    print f

if int(100*f) <> int(100*fd):
    print 'pack_double() <> unpack_double()'
