"""Guess the MIME type of a file.

This module defines two useful functions:

guess_type(url) -- guess the MIME type and encoding of a URL.

guess_extension(type) -- guess the extension for a given MIME type.

It also contains the following, for tuning the behavior:

Data:

knownfiles -- list of files to parse
inited -- flag set when init() has been called
suffix_map -- dictionary mapping suffixes to suffixes
encodings_map -- dictionary mapping suffixes to encodings
types_map -- dictionary mapping suffixes to types

Functions:

init([files]) -- parse a list of files, default knownfiles
read_mime_types(file) -- parse one file, return a dictionary or None

"""

import os
import posixpath
import urllib

__all__ = ["guess_type","guess_extension","read_mime_types","init"]

knownfiles = [
    "/usr/local/etc/httpd/conf/mime.types",
    "/usr/local/lib/netscape/mime.types",
    "/usr/local/etc/httpd/conf/mime.types",     # Apache 1.2
    "/usr/local/etc/mime.types",                # Apache 1.3
    ]

inited = 0


class MimeTypes:
    """MIME-types datastore.

    This datastore can handle information from mime.types-style files
    and supports basic determination of MIME type from a filename or
    URL, and can guess a reasonable extension given a MIME type.
    """

    def __init__(self, filenames=()):
        if not inited:
            init()
        self.encodings_map = encodings_map.copy()
        self.suffix_map = suffix_map.copy()
        self.types_map = types_map.copy()
        for name in filenames:
            self.read(name)

    def guess_type(self, url):
        """Guess the type of a file based on its URL.

        Return value is a tuple (type, encoding) where type is None if
        the type can't be guessed (no or unknown suffix) or a string
        of the form type/subtype, usable for a MIME Content-type
        header; and encoding is None for no encoding or the name of
        the program used to encode (e.g. compress or gzip).  The
        mappings are table driven.  Encoding suffixes are case
        sensitive; type suffixes are first tried case sensitive, then
        case insensitive.

        The suffixes .tgz, .taz and .tz (case sensitive!) are all
        mapped to '.tar.gz'.  (This is table-driven too, using the
        dictionary suffix_map.)
        """
        scheme, url = urllib.splittype(url)
        if scheme == 'data':
            # syntax of data URLs:
            # dataurl   := "data:" [ mediatype ] [ ";base64" ] "," data
            # mediatype := [ type "/" subtype ] *( ";" parameter )
            # data      := *urlchar
            # parameter := attribute "=" value
            # type/subtype defaults to "text/plain"
            comma = url.find(',')
            if comma < 0:
                # bad data URL
                return None, None
            semi = url.find(';', 0, comma)
            if semi >= 0:
                type = url[:semi]
            else:
                type = url[:comma]
            if '=' in type or '/' not in type:
                type = 'text/plain'
            return type, None           # never compressed, so encoding is None
        base, ext = posixpath.splitext(url)
        while self.suffix_map.has_key(ext):
            base, ext = posixpath.splitext(base + self.suffix_map[ext])
        if self.encodings_map.has_key(ext):
            encoding = self.encodings_map[ext]
            base, ext = posixpath.splitext(base)
        else:
            encoding = None
        types_map = self.types_map
        if types_map.has_key(ext):
            return types_map[ext], encoding
        elif types_map.has_key(ext.lower()):
            return types_map[ext.lower()], encoding
        else:
            return None, encoding

    def guess_extension(self, type):
        """Guess the extension for a file based on its MIME type.

        Return value is a string giving a filename extension,
        including the leading dot ('.').  The extension is not
        guaranteed to have been associated with any particular data
        stream, but would be mapped to the MIME type `type' by
        guess_type().  If no extension can be guessed for `type', None
        is returned.
        """
        type = type.lower()
        for ext, stype in self.types_map.items():
            if type == stype:
                return ext
        return None

    def read(self, filename):
        """Read a single mime.types-format file, specified by pathname."""
        fp = open(filename)
        self.readfp(fp)
        fp.close()

    def readfp(self, fp):
        """Read a single mime.types-format file."""
        map = self.types_map
        while 1:
            line = fp.readline()
            if not line:
                break
            words = line.split()
            for i in range(len(words)):
                if words[i][0] == '#':
                    del words[i:]
                    break
            if not words:
                continue
            type, suffixes = words[0], words[1:]
            for suff in suffixes:
                map['.' + suff] = type


def guess_type(url):
    """Guess the type of a file based on its URL.

    Return value is a tuple (type, encoding) where type is None if the
    type can't be guessed (no or unknown suffix) or a string of the
    form type/subtype, usable for a MIME Content-type header; and
    encoding is None for no encoding or the name of the program used
    to encode (e.g. compress or gzip).  The mappings are table
    driven.  Encoding suffixes are case sensitive; type suffixes are
    first tried case sensitive, then case insensitive.

    The suffixes .tgz, .taz and .tz (case sensitive!) are all mapped
    to ".tar.gz".  (This is table-driven too, using the dictionary
    suffix_map).
    """
    init()
    return guess_type(url)


def guess_extension(type):
    """Guess the extension for a file based on its MIME type.

    Return value is a string giving a filename extension, including the
    leading dot ('.').  The extension is not guaranteed to have been
    associated with any particular data stream, but would be mapped to the
    MIME type `type' by guess_type().  If no extension can be guessed for
    `type', None is returned.
    """
    init()
    return guess_extension(type)


def init(files=None):
    global guess_extension, guess_type
    global suffix_map, types_map, encodings_map
    global inited
    inited = 1
    db = MimeTypes()
    if files is None:
        files = knownfiles
    for file in files:
        if os.path.isfile(file):
            db.readfp(open(file))
    encodings_map = db.encodings_map
    suffix_map = db.suffix_map
    types_map = db.types_map
    guess_extension = db.guess_extension
    guess_type = db.guess_type


def read_mime_types(file):
    try:
        f = open(file)
    except IOError:
        return None
    db = MimeTypes()
    db.readfp(f)
    return db.types_map


suffix_map = {
    '.tgz': '.tar.gz',
    '.taz': '.tar.gz',
    '.tz': '.tar.gz',
    }

encodings_map = {
    '.gz': 'gzip',
    '.Z': 'compress',
    }

# Before adding new types, make sure they are either registered with IANA, at
# http://www.isi.edu/in-notes/iana/assignments/media-types
# or extensions, i.e. using the x- prefix
types_map = {
    '.a': 'application/octet-stream',
    '.ai': 'application/postscript',
    '.aif': 'audio/x-aiff',
    '.aifc': 'audio/x-aiff',
    '.aiff': 'audio/x-aiff',
    '.au': 'audio/basic',
    '.avi': 'video/x-msvideo',
    '.bcpio': 'application/x-bcpio',
    '.bin': 'application/octet-stream',
    '.bmp': 'image/x-ms-bmp',
    '.cdf': 'application/x-netcdf',
    '.cpio': 'application/x-cpio',
    '.csh': 'application/x-csh',
    '.css': 'text/css',
    '.dll': 'application/octet-stream',
    '.doc': 'application/msword',
    '.dvi': 'application/x-dvi',
    '.exe': 'application/octet-stream',
    '.eps': 'application/postscript',
    '.etx': 'text/x-setext',
    '.gif': 'image/gif',
    '.gtar': 'application/x-gtar',
    '.hdf': 'application/x-hdf',
    '.htm': 'text/html',
    '.html': 'text/html',
    '.ief': 'image/ief',
    '.jpe': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.jpg': 'image/jpeg',
    '.js': 'application/x-javascript',
    '.latex': 'application/x-latex',
    '.man': 'application/x-troff-man',
    '.me': 'application/x-troff-me',
    '.mif': 'application/x-mif',
    '.mov': 'video/quicktime',
    '.movie': 'video/x-sgi-movie',
    '.mp2': 'audio/mpeg',
    '.mp3': 'audio/mpeg',
    '.mpe': 'video/mpeg',
    '.mpeg': 'video/mpeg',
    '.mpg': 'video/mpeg',
    '.ms': 'application/x-troff-ms',
    '.nc': 'application/x-netcdf',
    '.o': 'application/octet-stream',
    '.obj': 'application/octet-stream',
    '.oda': 'application/oda',
    '.pbm': 'image/x-portable-bitmap',
    '.pdf': 'application/pdf',
    '.pgm': 'image/x-portable-graymap',
    '.pnm': 'image/x-portable-anymap',
    '.png': 'image/png',
    '.ppm': 'image/x-portable-pixmap',
    '.ps': 'application/postscript',
    '.py': 'text/x-python',
    '.pyc': 'application/x-python-code',
    '.pyo': 'application/x-python-code',
    '.qt': 'video/quicktime',
    '.ras': 'image/x-cmu-raster',
    '.rgb': 'image/x-rgb',
    '.rdf': 'application/xml',
    '.roff': 'application/x-troff',
    '.rtx': 'text/richtext',
    '.sgm': 'text/x-sgml',
    '.sgml': 'text/x-sgml',
    '.sh': 'application/x-sh',
    '.shar': 'application/x-shar',
    '.snd': 'audio/basic',
    '.so': 'application/octet-stream',
    '.src': 'application/x-wais-source',
    '.sv4cpio': 'application/x-sv4cpio',
    '.sv4crc': 'application/x-sv4crc',
    '.t': 'application/x-troff',
    '.tar': 'application/x-tar',
    '.tcl': 'application/x-tcl',
    '.tex': 'application/x-tex',
    '.texi': 'application/x-texinfo',
    '.texinfo': 'application/x-texinfo',
    '.tif': 'image/tiff',
    '.tiff': 'image/tiff',
    '.tr': 'application/x-troff',
    '.tsv': 'text/tab-separated-values',
    '.txt': 'text/plain',
    '.ustar': 'application/x-ustar',
    '.wav': 'audio/x-wav',
    '.xbm': 'image/x-xbitmap',
    '.xls': 'application/excel',
    '.xml': 'text/xml',
    '.xsl': 'application/xml',
    '.xpm': 'image/x-xpixmap',
    '.xwd': 'image/x-xwindowdump',
    '.zip': 'application/zip',
    '.mp3': 'audio/mpeg',
    '.ra': 'audio/x-pn-realaudio',
    '.pdf': 'application/pdf',
    '.c': 'text/plain',
    '.bat': 'text/plain',
    '.h': 'text/plain',
    '.pl': 'text/plain',
    '.ksh': 'text/plain',
    '.ram': 'application/x-pn-realaudio',
    '.cdf': 'application/x-cdf',
    '.doc': 'application/msword',
    '.dot': 'application/msword',
    '.wiz': 'application/msword',
    '.xlb': 'application/vnd.ms-excel',
    '.xls': 'application/vnd.ms-excel',
    '.ppa': 'application/vnd.ms-powerpoint',
    '.ppt': 'application/vnd.ms-powerpoint',
    '.pps': 'application/vnd.ms-powerpoint',
    '.pot': 'application/vnd.ms-powerpoint',
    '.pwz': 'application/vnd.ms-powerpoint',
    '.eml':   'message/rfc822',
    '.nws':   'message/rfc822',
    '.mht':   'message/rfc822',
    '.mhtml': 'message/rfc822',
    '.css': 'text/css',
    '.p7c': 'application/pkcs7-mime',
    '.p12': 'application/x-pkcs12',
    '.pfx': 'application/x-pkcs12',
    '.js':  'application/x-javascript',
    '.m1v': 'video/mpeg',
    '.mpa': 'video/mpeg',
    '.vcf': 'text/x-vcard',
    '.xml': 'text/xml',
    }

if __name__ == '__main__':
    import sys
    print guess_type(sys.argv[1])
