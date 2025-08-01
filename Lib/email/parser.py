# Copyright (C) 2001 Python Software Foundation
# Author: Barry Warsaw, Thomas Wouters, Anthony Baxter
# Contact: email-sig@python.org

"""A parser of RFC 2822 and MIME email messages."""

__all__ = ['Parser', 'HeaderParser', 'BytesParser', 'BytesHeaderParser',
           'FeedParser', 'BytesFeedParser']

from io import StringIO, TextIOWrapper

from email.feedparser import FeedParser, BytesFeedParser
from email._policybase import compat32

_FEED_CHUNK_SIZE = 8192


class Parser:
    def __init__(self, _class=None, *, policy=compat32):
        """Parser of RFC 2822 and MIME email messages.

        Creates an in-memory object tree representing the email message, which
        can then be manipulated and turned over to a Generator to return the
        textual representation of the message.

        The string must be formatted as a block of RFC 2822 headers and header
        continuation lines, optionally preceded by a 'Unix-from' header.  The
        header block is terminated either by the end of the string or by a
        blank line.

        _class is the class to instantiate for new message objects when they
        must be created.  This class must have a constructor that can take
        zero arguments.  Default is Message.Message.

        The policy keyword specifies a policy object that controls a number of
        aspects of the parser's operation.  The default policy maintains
        backward compatibility.

        """
        self._class = _class
        self.policy = policy

    def _parse_chunks(self, chunk_generator, headersonly=False):
        """Internal method / implementation detail

        Parses chunks from a chunk generator into a FeedParser
        """
        feedparser = FeedParser(self._class, policy=self.policy)
        if headersonly:
            feedparser._set_headersonly()
        for data in chunk_generator:
            feedparser.feed(data)
        return feedparser.close()

    def parse(self, fp, headersonly=False):
        """Create a message structure from the data in a file.

        Reads all the data from the file and returns the root of the message
        structure.  Optional headersonly is a flag specifying whether to stop
        parsing after reading the headers or not.  The default is False,
        meaning it parses the entire contents of the file.
        """
        def _fp_get_chunks():
            while data := fp.read(_FEED_CHUNK_SIZE):
                yield data
        _chunk_generator = _fp_get_chunks()

        return self._parse_chunks(_chunk_generator, headersonly)

    def parsestr(self, text, headersonly=False):
        """Create a message structure from a string.

        Returns the root of the message structure.  Optional headersonly is a
        flag specifying whether to stop parsing after reading the headers or
        not.  The default is False, meaning it parses the entire contents of
        the file.
        """
        _chunk_generator = (
            text[offset:offset + _FEED_CHUNK_SIZE]
            for offset in range(0, len(text), _FEED_CHUNK_SIZE)
        )

        return self._parse_chunks(_chunk_generator, headersonly)


class HeaderParser(Parser):
    def parse(self, fp, headersonly=True):
        return Parser.parse(self, fp, True)

    def parsestr(self, text, headersonly=True):
        return Parser.parsestr(self, text, True)


class BytesParser:

    def __init__(self, *args, **kw):
        """Parser of binary RFC 2822 and MIME email messages.

        Creates an in-memory object tree representing the email message, which
        can then be manipulated and turned over to a Generator to return the
        textual representation of the message.

        The input must be formatted as a block of RFC 2822 headers and header
        continuation lines, optionally preceded by a 'Unix-from' header.  The
        header block is terminated either by the end of the input or by a
        blank line.

        _class is the class to instantiate for new message objects when they
        must be created.  This class must have a constructor that can take
        zero arguments.  Default is Message.Message.
        """
        self.parser = Parser(*args, **kw)

    def parse(self, fp, headersonly=False):
        """Create a message structure from the data in a binary file.

        Reads all the data from the file and returns the root of the message
        structure.  Optional headersonly is a flag specifying whether to stop
        parsing after reading the headers or not.  The default is False,
        meaning it parses the entire contents of the file.
        """
        fp = TextIOWrapper(fp, encoding='ascii', errors='surrogateescape')
        try:
            return self.parser.parse(fp, headersonly)
        finally:
            fp.detach()


    def parsebytes(self, text, headersonly=False):
        """Create a message structure from a byte string.

        Returns the root of the message structure.  Optional headersonly is a
        flag specifying whether to stop parsing after reading the headers or
        not.  The default is False, meaning it parses the entire contents of
        the file.
        """
        _chunk_generator = (
            text[offset:offset + _FEED_CHUNK_SIZE].decode(
                'ASCII', errors='surrogateescape')
            for offset in range(0, len(text), _FEED_CHUNK_SIZE)
        )

        return self.parser._parse_chunks(_chunk_generator, headersonly)


class BytesHeaderParser(BytesParser):
    def parse(self, fp, headersonly=True):
        return BytesParser.parse(self, fp, headersonly=True)

    def parsebytes(self, text, headersonly=True):
        return BytesParser.parsebytes(self, text, headersonly=True)
