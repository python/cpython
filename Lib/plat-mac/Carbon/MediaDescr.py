# Parsers/generators for QuickTime media descriptions
import struct

Error = 'MediaDescr.Error'

class _MediaDescriptionCodec:
    def __init__(self, trunc, size, names, fmt):
        self.trunc = trunc
        self.size = size
        self.names = names
        self.fmt = fmt

    def decode(self, data):
        if self.trunc:
            data = data[:self.size]
        values = struct.unpack(self.fmt, data)
        if len(values) != len(self.names):
            raise Error, ('Format length does not match number of names', descr)
        rv = {}
        for i in range(len(values)):
            name = self.names[i]
            value = values[i]
            if type(name) == type(()):
                name, cod, dec = name
                value = dec(value)
            rv[name] = value
        return rv

    def encode(dict):
        list = [self.fmt]
        for name in self.names:
            if type(name) == type(()):
                name, cod, dec = name
            else:
                cod = dec = None
            value = dict[name]
            if cod:
                value = cod(value)
            list.append(value)
        rv = struct.pack(*list)
        return rv

# Helper functions
def _tofixed(float):
    hi = int(float)
    lo = int(float*0x10000) & 0xffff
    return (hi<<16)|lo

def _fromfixed(fixed):
    hi = (fixed >> 16) & 0xffff
    lo = (fixed & 0xffff)
    return hi + (lo / float(0x10000))

def _tostr31(str):
    return chr(len(str)) + str + '\0'*(31-len(str))

def _fromstr31(str31):
    return str31[1:1+ord(str31[0])]

SampleDescription = _MediaDescriptionCodec(
        1,      # May be longer, truncate
        16,     # size
        ('descSize', 'dataFormat', 'resvd1', 'resvd2', 'dataRefIndex'), # Attributes
        "l4slhh"        # Format
)

SoundDescription = _MediaDescriptionCodec(
        1,
        36,
        ('descSize', 'dataFormat', 'resvd1', 'resvd2', 'dataRefIndex',
        'version', 'revlevel', 'vendor', 'numChannels', 'sampleSize',
        'compressionID', 'packetSize', ('sampleRate', _tofixed, _fromfixed)),
        "l4slhhhh4shhhhl"       # Format
)

SoundDescriptionV1 = _MediaDescriptionCodec(
        1,
        52,
        ('descSize', 'dataFormat', 'resvd1', 'resvd2', 'dataRefIndex',
        'version', 'revlevel', 'vendor', 'numChannels', 'sampleSize',
        'compressionID', 'packetSize', ('sampleRate', _tofixed, _fromfixed), 'samplesPerPacket',
        'bytesPerPacket', 'bytesPerFrame', 'bytesPerSample'),
        "l4slhhhh4shhhhlllll"   # Format
)

ImageDescription = _MediaDescriptionCodec(
        1,      # May be longer, truncate
        86,     # size
        ('idSize', 'cType', 'resvd1', 'resvd2', 'dataRefIndex', 'version',
         'revisionLevel', 'vendor', 'temporalQuality', 'spatialQuality',
         'width', 'height', ('hRes', _tofixed, _fromfixed), ('vRes', _tofixed, _fromfixed),
        'dataSize', 'frameCount', ('name', _tostr31, _fromstr31),
         'depth', 'clutID'),
        'l4slhhhh4sllhhlllh32shh',
)

# XXXX Others, like TextDescription and such, remain to be done.
