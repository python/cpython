""" codecs -- Python Codec Registry, API and helpers.


Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import struct, __builtin__

### Registry and builtin stateless codec functions

try:
    from _codecs import *
except ImportError, why:
    raise SystemError,\
          'Failed to load the builtin codecs: %s' % why

__all__ = ["register", "lookup", "open", "EncodedFile", "BOM", "BOM_BE",
           "BOM_LE", "BOM32_BE", "BOM32_LE", "BOM64_BE", "BOM64_LE"]

### Constants

#
# Byte Order Mark (BOM) and its possible values (BOM_BE, BOM_LE)
#
BOM = struct.pack('=H', 0xFEFF)
#
BOM_BE = BOM32_BE = '\376\377'
#       corresponds to Unicode U+FEFF in UTF-16 on big endian
#       platforms == ZERO WIDTH NO-BREAK SPACE
BOM_LE = BOM32_LE = '\377\376'
#       corresponds to Unicode U+FFFE in UTF-16 on little endian
#       platforms == defined as being an illegal Unicode character

#
# 64-bit Byte Order Marks
#
BOM64_BE = '\000\000\376\377'
#       corresponds to Unicode U+0000FEFF in UCS-4
BOM64_LE = '\377\376\000\000'
#       corresponds to Unicode U+0000FFFE in UCS-4


### Codec base classes (defining the API)

class Codec:

    """ Defines the interface for stateless encoders/decoders.

        The .encode()/.decode() methods may implement different error
        handling schemes by providing the errors argument. These
        string values are defined:

         'strict' - raise a ValueError error (or a subclass)
         'ignore' - ignore the character and continue with the next
         'replace' - replace with a suitable replacement character;
                    Python will use the official U+FFFD REPLACEMENT
                    CHARACTER for the builtin Unicode codecs.

    """
    def encode(self, input, errors='strict'):

        """ Encodes the object input and returns a tuple (output
            object, length consumed).

            errors defines the error handling to apply. It defaults to
            'strict' handling.

            The method may not store state in the Codec instance. Use
            StreamCodec for codecs which have to keep state in order to
            make encoding/decoding efficient.

            The encoder must be able to handle zero length input and
            return an empty object of the output object type in this
            situation.

        """
        raise NotImplementedError

    def decode(self, input, errors='strict'):

        """ Decodes the object input and returns a tuple (output
            object, length consumed).

            input must be an object which provides the bf_getreadbuf
            buffer slot. Python strings, buffer objects and memory
            mapped files are examples of objects providing this slot.

            errors defines the error handling to apply. It defaults to
            'strict' handling.

            The method may not store state in the Codec instance. Use
            StreamCodec for codecs which have to keep state in order to
            make encoding/decoding efficient.

            The decoder must be able to handle zero length input and
            return an empty object of the output object type in this
            situation.

        """
        raise NotImplementedError

#
# The StreamWriter and StreamReader class provide generic working
# interfaces which can be used to implement new encoding submodules
# very easily. See encodings/utf_8.py for an example on how this is
# done.
#

class StreamWriter(Codec):

    def __init__(self, stream, errors='strict'):

        """ Creates a StreamWriter instance.

            stream must be a file-like object open for writing
            (binary) data.

            The StreamWriter may implement different error handling
            schemes by providing the errors keyword argument. These
            parameters are defined:

             'strict' - raise a ValueError (or a subclass)
             'ignore' - ignore the character and continue with the next
             'replace'- replace with a suitable replacement character

        """
        self.stream = stream
        self.errors = errors

    def write(self, object):

        """ Writes the object's contents encoded to self.stream.
        """
        data, consumed = self.encode(object, self.errors)
        self.stream.write(data)

    def writelines(self, list):

        """ Writes the concatenated list of strings to the stream
            using .write().
        """
        self.write(''.join(list))

    def reset(self):

        """ Flushes and resets the codec buffers used for keeping state.

            Calling this method should ensure that the data on the
            output is put into a clean state, that allows appending
            of new fresh data without having to rescan the whole
            stream to recover state.

        """
        pass

    def __getattr__(self, name,
                    getattr=getattr):

        """ Inherit all other methods from the underlying stream.
        """
        return getattr(self.stream, name)

###

class StreamReader(Codec):

    def __init__(self, stream, errors='strict'):

        """ Creates a StreamReader instance.

            stream must be a file-like object open for reading
            (binary) data.

            The StreamReader may implement different error handling
            schemes by providing the errors keyword argument. These
            parameters are defined:

             'strict' - raise a ValueError (or a subclass)
             'ignore' - ignore the character and continue with the next
             'replace'- replace with a suitable replacement character;

        """
        self.stream = stream
        self.errors = errors

    def read(self, size=-1):

        """ Decodes data from the stream self.stream and returns the
            resulting object.

            size indicates the approximate maximum number of bytes to
            read from the stream for decoding purposes. The decoder
            can modify this setting as appropriate. The default value
            -1 indicates to read and decode as much as possible.  size
            is intended to prevent having to decode huge files in one
            step.

            The method should use a greedy read strategy meaning that
            it should read as much data as is allowed within the
            definition of the encoding and the given size, e.g.  if
            optional encoding endings or state markers are available
            on the stream, these should be read too.

        """
        # Unsliced reading:
        if size < 0:
            return self.decode(self.stream.read(), self.errors)[0]

        # Sliced reading:
        read = self.stream.read
        decode = self.decode
        data = read(size)
        i = 0
        while 1:
            try:
                object, decodedbytes = decode(data, self.errors)
            except ValueError, why:
                # This method is slow but should work under pretty much
                # all conditions; at most 10 tries are made
                i = i + 1
                newdata = read(1)
                if not newdata or i > 10:
                    raise
                data = data + newdata
            else:
                return object

    def readline(self, size=None):

        """ Read one line from the input stream and return the
            decoded data.

            Note: Unlike the .readlines() method, this method inherits
            the line breaking knowledge from the underlying stream's
            .readline() method -- there is currently no support for
            line breaking using the codec decoder due to lack of line
            buffering. Sublcasses should however, if possible, try to
            implement this method using their own knowledge of line
            breaking.

            size, if given, is passed as size argument to the stream's
            .readline() method.

        """
        if size is None:
            line = self.stream.readline()
        else:
            line = self.stream.readline(size)
        return self.decode(line, self.errors)[0]


    def readlines(self, sizehint=None):

        """ Read all lines available on the input stream
            and return them as list of lines.

            Line breaks are implemented using the codec's decoder
            method and are included in the list entries.

            sizehint, if given, is passed as size argument to the
            stream's .read() method.

        """
        if sizehint is None:
            data = self.stream.read()
        else:
            data = self.stream.read(sizehint)
        return self.decode(data, self.errors)[0].splitlines(1)

    def reset(self):

        """ Resets the codec buffers used for keeping state.

            Note that no stream repositioning should take place.
            This method is primarily intended to be able to recover
            from decoding errors.

        """
        pass

    def __getattr__(self, name,
                    getattr=getattr):

        """ Inherit all other methods from the underlying stream.
        """
        return getattr(self.stream, name)

###

class StreamReaderWriter:

    """ StreamReaderWriter instances allow wrapping streams which
        work in both read and write modes.

        The design is such that one can use the factory functions
        returned by the codec.lookup() function to construct the
        instance.

    """
    # Optional attributes set by the file wrappers below
    encoding = 'unknown'

    def __init__(self, stream, Reader, Writer, errors='strict'):

        """ Creates a StreamReaderWriter instance.

            stream must be a Stream-like object.

            Reader, Writer must be factory functions or classes
            providing the StreamReader, StreamWriter interface resp.

            Error handling is done in the same way as defined for the
            StreamWriter/Readers.

        """
        self.stream = stream
        self.reader = Reader(stream, errors)
        self.writer = Writer(stream, errors)
        self.errors = errors

    def read(self, size=-1):

        return self.reader.read(size)

    def readline(self, size=None):

        return self.reader.readline(size)

    def readlines(self, sizehint=None):

        return self.reader.readlines(sizehint)

    def write(self, data):

        return self.writer.write(data)

    def writelines(self, list):

        return self.writer.writelines(list)

    def reset(self):

        self.reader.reset()
        self.writer.reset()

    def __getattr__(self, name,
                    getattr=getattr):

        """ Inherit all other methods from the underlying stream.
        """
        return getattr(self.stream, name)

###

class StreamRecoder:

    """ StreamRecoder instances provide a frontend - backend
        view of encoding data.

        They use the complete set of APIs returned by the
        codecs.lookup() function to implement their task.

        Data written to the stream is first decoded into an
        intermediate format (which is dependent on the given codec
        combination) and then written to the stream using an instance
        of the provided Writer class.

        In the other direction, data is read from the stream using a
        Reader instance and then return encoded data to the caller.

    """
    # Optional attributes set by the file wrappers below
    data_encoding = 'unknown'
    file_encoding = 'unknown'

    def __init__(self, stream, encode, decode, Reader, Writer,
                 errors='strict'):

        """ Creates a StreamRecoder instance which implements a two-way
            conversion: encode and decode work on the frontend (the
            input to .read() and output of .write()) while
            Reader and Writer work on the backend (reading and
            writing to the stream).

            You can use these objects to do transparent direct
            recodings from e.g. latin-1 to utf-8 and back.

            stream must be a file-like object.

            encode, decode must adhere to the Codec interface, Reader,
            Writer must be factory functions or classes providing the
            StreamReader, StreamWriter interface resp.

            encode and decode are needed for the frontend translation,
            Reader and Writer for the backend translation. Unicode is
            used as intermediate encoding.

            Error handling is done in the same way as defined for the
            StreamWriter/Readers.

        """
        self.stream = stream
        self.encode = encode
        self.decode = decode
        self.reader = Reader(stream, errors)
        self.writer = Writer(stream, errors)
        self.errors = errors

    def read(self, size=-1):

        data = self.reader.read(size)
        data, bytesencoded = self.encode(data, self.errors)
        return data

    def readline(self, size=None):

        if size is None:
            data = self.reader.readline()
        else:
            data = self.reader.readline(size)
        data, bytesencoded = self.encode(data, self.errors)
        return data

    def readlines(self, sizehint=None):

        if sizehint is None:
            data = self.reader.read()
        else:
            data = self.reader.read(sizehint)
        data, bytesencoded = self.encode(data, self.errors)
        return data.splitlines(1)

    def write(self, data):

        data, bytesdecoded = self.decode(data, self.errors)
        return self.writer.write(data)

    def writelines(self, list):

        data = ''.join(list)
        data, bytesdecoded = self.decode(data, self.errors)
        return self.writer.write(data)

    def reset(self):

        self.reader.reset()
        self.writer.reset()

    def __getattr__(self, name,
                    getattr=getattr):

        """ Inherit all other methods from the underlying stream.
        """
        return getattr(self.stream, name)

### Shortcuts

def open(filename, mode='rb', encoding=None, errors='strict', buffering=1):

    """ Open an encoded file using the given mode and return
        a wrapped version providing transparent encoding/decoding.

        Note: The wrapped version will only accept the object format
        defined by the codecs, i.e. Unicode objects for most builtin
        codecs. Output is also codec dependent and will usually by
        Unicode as well.

        Files are always opened in binary mode, even if no binary mode
        was specified. Thisis done to avoid data loss due to encodings
        using 8-bit values. The default file mode is 'rb' meaning to
        open the file in binary read mode.

        encoding specifies the encoding which is to be used for the
        the file.

        errors may be given to define the error handling. It defaults
        to 'strict' which causes ValueErrors to be raised in case an
        encoding error occurs.

        buffering has the same meaning as for the builtin open() API.
        It defaults to line buffered.

        The returned wrapped file object provides an extra attribute
        .encoding which allows querying the used encoding. This
        attribute is only available if an encoding was specified as
        parameter.

    """
    if encoding is not None and \
       'b' not in mode:
        # Force opening of the file in binary mode
        mode = mode + 'b'
    file = __builtin__.open(filename, mode, buffering)
    if encoding is None:
        return file
    (e, d, sr, sw) = lookup(encoding)
    srw = StreamReaderWriter(file, sr, sw, errors)
    # Add attributes to simplify introspection
    srw.encoding = encoding
    return srw

def EncodedFile(file, data_encoding, file_encoding=None, errors='strict'):

    """ Return a wrapped version of file which provides transparent
        encoding translation.

        Strings written to the wrapped file are interpreted according
        to the given data_encoding and then written to the original
        file as string using file_encoding. The intermediate encoding
        will usually be Unicode but depends on the specified codecs.

        Strings are read from the file using file_encoding and then
        passed back to the caller as string using data_encoding.

        If file_encoding is not given, it defaults to data_encoding.

        errors may be given to define the error handling. It defaults
        to 'strict' which causes ValueErrors to be raised in case an
        encoding error occurs.

        The returned wrapped file object provides two extra attributes
        .data_encoding and .file_encoding which reflect the given
        parameters of the same name. The attributes can be used for
        introspection by Python programs.

    """
    if file_encoding is None:
        file_encoding = data_encoding
    encode, decode = lookup(data_encoding)[:2]
    Reader, Writer = lookup(file_encoding)[2:]
    sr = StreamRecoder(file,
                       encode, decode, Reader, Writer,
                       errors)
    # Add attributes to simplify introspection
    sr.data_encoding = data_encoding
    sr.file_encoding = file_encoding
    return sr

### Helpers for codec lookup

def getencoder(encoding):

    """ Lookup up the codec for the given encoding and return
        its encoder function.

        Raises a LookupError in case the encoding cannot be found.

    """
    return lookup(encoding)[0]

def getdecoder(encoding):

    """ Lookup up the codec for the given encoding and return
        its decoder function.

        Raises a LookupError in case the encoding cannot be found.

    """
    return lookup(encoding)[1]

def getreader(encoding):

    """ Lookup up the codec for the given encoding and return
        its StreamReader class or factory function.

        Raises a LookupError in case the encoding cannot be found.

    """
    return lookup(encoding)[2]

def getwriter(encoding):

    """ Lookup up the codec for the given encoding and return
        its StreamWriter class or factory function.

        Raises a LookupError in case the encoding cannot be found.

    """
    return lookup(encoding)[3]

### Helpers for charmap-based codecs

def make_identity_dict(rng):

    """ make_identity_dict(rng) -> dict

        Return a dictionary where elements of the rng sequence are
        mapped to themselves.

    """
    res = {}
    for i in rng:
        res[i]=i
    return res

def make_encoding_map(decoding_map):

    """ Creates an encoding map from a decoding map.

        If a target mapping in the decoding map occurrs multiple
        times, then that target is mapped to None (undefined mapping),
        causing an exception when encountered by the charmap codec
        during translation.

        One example where this happens is cp875.py which decodes
        multiple character to \u001a.

    """
    m = {}
    for k,v in decoding_map.items():
        if not m.has_key(v):
            m[v] = k
        else:
            m[v] = None
    return m

# Tell modulefinder that using codecs probably needs the encodings
# package
_false = 0
if _false:
    import encodings

### Tests

if __name__ == '__main__':

    import sys

    # Make stdout translate Latin-1 output into UTF-8 output
    sys.stdout = EncodedFile(sys.stdout, 'latin-1', 'utf-8')

    # Have stdin translate Latin-1 input into UTF-8 input
    sys.stdin = EncodedFile(sys.stdin, 'utf-8', 'latin-1')
