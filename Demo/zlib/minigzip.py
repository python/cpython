#!/usr/bin/env python
# Demo program for zlib; it compresses or decompresses files, but *doesn't*
# delete the original.  This doesn't support all of gzip's options.

FTEXT, FHCRC, FEXTRA, FNAME, FCOMMENT = 1, 2, 4, 8, 16

def write32(output, value):
    output.write(chr(value & 255)) ; value=value / 256
    output.write(chr(value & 255)) ; value=value / 256
    output.write(chr(value & 255)) ; value=value / 256
    output.write(chr(value & 255))

def read32(input):
    v=ord(input.read(1))
    v=v+ (ord(input.read(1))<<8 )
    v=v+ (ord(input.read(1))<<16)
    v=v+ (ord(input.read(1))<<24)
    return v

import zlib, sys
if len(sys.argv)!=2:
    print 'Usage: minigzip.py <filename>'
    print '  The file will be compressed or decompressed.'
    sys.exit(0)

filename=sys.argv[1]
compressing=1 ; outputname=filename+'.gz'
if filename[-3:]=='.gz':
    compressing=0 ; outputname=filename[:-3]
input=open(filename) ; output=open(outputname, 'w')

if compressing:
    output.write('\037\213\010')        # Write the header, ...
    output.write(chr(FNAME))            # ... flag byte ...

    import os                           # ... modification time ...
    statval=os.stat(filename)
    mtime=statval[8]
    write32(output, mtime)
    output.write('\002')                # ... slowest compression alg. ...
    output.write('\377')                # ... OS (=unknown) ...
    output.write(filename+'\000')       # ... original filename ...

    crcval=zlib.crc32("")
    compobj=zlib.compressobj(9, zlib.DEFLATED, -zlib.MAX_WBITS,
                             zlib.DEF_MEM_LEVEL, 0)
    while (1):
        data=input.read(1024)
        if data=="": break
        crcval=zlib.crc32(data, crcval)
        output.write(compobj.compress(data))
    output.write(compobj.flush())
    write32(output, crcval)             # ... the CRC ...
    write32(output, statval[6])         # and the file size.

else:
    magic=input.read(2)
    if magic!='\037\213':
        print 'Not a gzipped file' ; sys.exit(0)
    if ord(input.read(1))!=8:
        print 'Unknown compression method' ; sys.exit(0)
    flag=ord(input.read(1))
    input.read(4+1+1)                   # Discard modification time,
                                        # extra flags, and OS byte.
    if flag & FEXTRA:
        # Read & discard the extra field, if present
        xlen=ord(input.read(1))
        xlen=xlen+256*ord(input.read(1))
        input.read(xlen)
    if flag & FNAME:
        # Read and discard a null-terminated string containing the filename
        while (1):
            s=input.read(1)
            if s=='\000': break
    if flag & FCOMMENT:
        # Read and discard a null-terminated string containing a comment
        while (1):
            s=input.read(1)
            if s=='\000': break
    if flag & FHCRC:
        input.read(2)                   # Read & discard the 16-bit header CRC
    decompobj=zlib.decompressobj(-zlib.MAX_WBITS)
    crcval=zlib.crc32("")
    length=0
    while (1):
        data=input.read(1024)
        if data=="": break
        decompdata=decompobj.decompress(data)
        print len(decompdata)
        output.write(decompdata) ; length=length+len(decompdata)
        crcval=zlib.crc32(decompdata, crcval)
    decompdata=decompobj.flush()
    output.write(decompdata) ; length=length+len(decompdata)
    crcval=zlib.crc32(decompdata, crcval)

    # We've read to the end of the file, so we have to rewind in order
    # to reread the 8 bytes containing the CRC and the file size.  The
    # decompressor is smart and knows when to stop, so feeding it
    # extra data is harmless.
    input.seek(-8, 2)
    crc32=read32(input)
    isize=read32(input)
    if crc32!=crcval: print 'CRC check failed.'
    if isize!=length: print 'Incorrect length of data produced'

input.close() ; output.close()
