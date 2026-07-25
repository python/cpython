import codecs

def create_iconv_codec(name, encoding):
    from _codecs import iconv_encode, iconv_decode

    def encode(input, errors='strict'):
        return iconv_encode(encoding, input, errors)

    def decode(input, errors='strict'):
        return iconv_decode(encoding, input, errors, True)

    class IncrementalEncoder(codecs.IncrementalEncoder):
        def encode(self, input, final=False):
            return iconv_encode(encoding, input, self.errors)[0]

    class IncrementalDecoder(codecs.BufferedIncrementalDecoder):
        def _buffer_decode(self, input, errors, final):
            return iconv_decode(encoding, input, errors, final)

    class StreamWriter(codecs.StreamWriter):
        def encode(self, input, errors='strict'):
            return iconv_encode(encoding, input, errors)

    class StreamReader(codecs.StreamReader):
        def decode(self, input, errors, final=False):
            return iconv_decode(encoding, input, errors, final)

    return codecs.CodecInfo(
        name=name,
        encode=encode,
        decode=decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )
