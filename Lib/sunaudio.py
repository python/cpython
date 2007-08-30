"""Interpret sun audio headers."""

MAGIC = b'.snd'

class error(Exception):
    pass


def get_long_be(s):
    """Convert a 4-byte value to integer."""
    return (s[0]<<24) | (s[1]<<16) | (s[2]<<8) | s[3]


def gethdr(fp):
    """Read a sound header from an open file."""
    if fp.read(4) != MAGIC:
        raise error('gethdr: bad magic word')
    hdr_size = get_long_be(fp.read(4))
    data_size = get_long_be(fp.read(4))
    encoding = get_long_be(fp.read(4))
    sample_rate = get_long_be(fp.read(4))
    channels = get_long_be(fp.read(4))
    excess = hdr_size - 24
    if excess < 0:
        raise error('gethdr: bad hdr_size')
    if excess > 0:
        info = fp.read(excess)
    else:
        info = b''
    return (data_size, encoding, sample_rate, channels, info)


def printhdr(file):
    """Read and print the sound header of a named file."""
    f = open(file, 'rb')
    try:
        hdr = gethdr(f)
    finally:
        f.close()
    data_size, encoding, sample_rate, channels, info = hdr
    while info.endswith(b'\0'):
        info = info[:-1]
    print('File name:  ', file)
    print('Data size:  ', data_size)
    print('Encoding:   ', encoding)
    print('Sample rate:', sample_rate)
    print('Channels:   ', channels)
    print('Info:       ', repr(info))
