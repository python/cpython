"""The io module provides the Python interfaces to stream handling. The
builtin open function is defined in this module.

At the top of the I/O hierarchy is the abstract base class IOBase. It
defines the basic interface to a stream. Note, however, that there is no
separation between reading and writing to streams; implementations are
allowed to throw an IOError if they do not support a given operation.

Extending IOBase is RawIOBase which deals simply with the reading and
writing of raw bytes to a stream. FileIO subclasses RawIOBase to provide
an interface to OS files.

BufferedIOBase deals with buffering on a raw byte stream (RawIOBase). Its
subclasses, BufferedWriter, BufferedReader, and BufferedRWPair buffer
streams that are readable, writable, and both respectively.
BufferedRandom provides a buffered interface to random access
streams. BytesIO is a simple stream of in-memory bytes.

Another IOBase subclass, TextIOBase, deals with the encoding and decoding
of streams into text. TextIOWrapper, which extends it, is a buffered text
interface to a buffered raw stream (`BufferedIOBase`). Finally, StringIO
is a in-memory stream for text.

Argument names are not part of the specification, and only the arguments
of open() are intended to be used as keyword arguments.

data:

DEFAULT_BUFFER_SIZE

   An int containing the default buffer size used by the module's buffered
   I/O classes. open() uses the file's blksize (as obtained by os.stat) if
   possible.
"""
# New I/O library conforming to PEP 3116.

# XXX edge cases when switching between reading/writing
# XXX need to support 1 meaning line-buffered
# XXX whenever an argument is None, use the default value
# XXX read/write ops should check readable/writable
# XXX buffered readinto should work with arbitrary buffer objects
# XXX use incremental encoder for text output, at least for UTF-16 and UTF-8-SIG
# XXX check writable, readable and seekable in appropriate places


__author__ = ("Guido van Rossum <guido@python.org>, "
              "Mike Verdone <mike.verdone@gmail.com>, "
              "Mark Russell <mark.russell@zen.co.uk>, "
              "Antoine Pitrou <solipsis@pitrou.net>, "
              "Amaury Forgeotdarc <amauryfa@gmail.com>")

__all__ = ["BlockingIOError", "open", "IOBase", "RawIOBase", "FileIO",
           "BytesIO", "StringIO", "BufferedIOBase",
           "BufferedReader", "BufferedWriter", "BufferedRWPair",
           "BufferedRandom", "TextIOBase", "TextIOWrapper"]


import _io
import abc

# open() uses st_blksize whenever we can
DEFAULT_BUFFER_SIZE = _io.DEFAULT_BUFFER_SIZE
BlockingIOError = _io.BlockingIOError
UnsupportedOperation = _io.UnsupportedOperation
open = _io.open
OpenWrapper = _io.open

# Declaring ABCs in C is tricky so we do it here.
# Method descriptions and default implementations are inherited from the C
# version however.
class IOBase(_io._IOBase, metaclass=abc.ABCMeta):
    pass

class RawIOBase(_io._RawIOBase, IOBase):
    pass

class BufferedIOBase(_io._BufferedIOBase, IOBase):
    pass

class TextIOBase(_io._TextIOBase, IOBase):
    pass

FileIO = _io.FileIO
BytesIO = _io.BytesIO
StringIO = _io.StringIO
BufferedReader = _io.BufferedReader
BufferedWriter = _io.BufferedWriter
BufferedRWPair = _io.BufferedRWPair
BufferedRandom = _io.BufferedRandom
IncrementalNewlineDecoder = _io.IncrementalNewlineDecoder
TextIOWrapper = _io.TextIOWrapper

RawIOBase.register(FileIO)

BufferedIOBase.register(BytesIO)
BufferedIOBase.register(BufferedReader)
BufferedIOBase.register(BufferedWriter)
BufferedIOBase.register(BufferedRandom)
BufferedIOBase.register(BufferedRWPair)

TextIOBase.register(StringIO)
TextIOBase.register(TextIOWrapper)
