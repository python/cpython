"""Guess the MIME type of a file.

This module defines two useful functions:

guess_type(url) -- guess the MIME type and encoding of a URL.

guess_extension(type) -- guess the extension for a given MIME type.

It also contains the following, for tuning the behavior:

Data:

knownfiles -- list of files to parse
inited -- flag set when init() has been called
suffixes_map -- dictionary mapping suffixes to suffixes
encodings_map -- dictionary mapping suffixes to encodings
types_map -- dictionary mapping suffixes to types

Functions:

init([files]) -- parse a list of files, default knownfiles
read_mime_types(file) -- parse one file, return a dictionary or None

"""

import string
import posixpath
import urllib

knownfiles = [
    "/usr/local/etc/httpd/conf/mime.types",
    "/usr/local/lib/netscape/mime.types",
    "/usr/local/etc/httpd/conf/mime.types",     # Apache 1.2
    "/usr/local/etc/mime.types",                # Apache 1.3
    ]

inited = 0

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
    if not inited:
        init()
    scheme, url = urllib.splittype(url)
    if scheme == 'data':
        # syntax of data URLs:
        # dataurl   := "data:" [ mediatype ] [ ";base64" ] "," data
        # mediatype := [ type "/" subtype ] *( ";" parameter )
        # data      := *urlchar
        # parameter := attribute "=" value
        # type/subtype defaults to "text/plain"
        comma = string.find(url, ',')
        if comma < 0:
            # bad data URL
            return None, None
        semi = string.find(url, ';', 0, comma)
        if semi >= 0:
            type = url[:semi]
        else:
            type = url[:comma]
        if '=' in type or '/' not in type:
            type = 'text/plain'
        return type, None               # never compressed, so encoding is None
    base, ext = posixpath.splitext(url)
    while suffix_map.has_key(ext):
        base, ext = posixpath.splitext(base + suffix_map[ext])
    if encodings_map.has_key(ext):
        encoding = encodings_map[ext]
        base, ext = posixpath.splitext(base)
    else:
        encoding = None
    if types_map.has_key(ext):
        return types_map[ext], encoding
    elif types_map.has_key(string.lower(ext)):
        return types_map[string.lower(ext)], encoding
    else:
        return None, encoding

def guess_extension(type):
    """Guess the extension for a file based on its MIME type.

    Return value is a string giving a filename extension, including the
    leading dot ('.').  The extension is not guaranteed to have been
    associated with any particular data stream, but would be mapped to the
    MIME type `type' by guess_type().  If no extension can be guessed for
    `type', None is returned.
    """
    global inited
    if not inited:
        init()
    type = string.lower(type)
    for ext, stype in types_map.items():
        if type == stype:
            return ext
    return None

def init(files=None):
    global inited
    for file in files or knownfiles:
        s = read_mime_types(file)
        if s:
            for key, value in s.items():
                types_map[key] = value
    inited = 1

def read_mime_types(file):
    try:
        f = open(file)
    except IOError:
        return None
    map = {}
    while 1:
        line = f.readline()
        if not line: break
        words = string.split(line)
        for i in range(len(words)):
            if words[i][0] == '#':
                del words[i:]
                break
        if not words: continue
        type, suffixes = words[0], words[1:]
        for suff in suffixes:
            map['.'+suff] = type
    f.close()
    return map

suffix_map = {
    '.tgz': '.tar.gz',
    '.taz': '.tar.gz',
    '.tz': '.tar.gz',
}

encodings_map = {
    '.gz': 'gzip',
    '.Z': 'compress',
    }

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
    '.cdf': 'application/x-netcdf',
    '.cpio': 'application/x-cpio',
    '.csh': 'application/x-csh',
    '.dll': 'application/octet-stream',
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
    '.py': 'text/x-python',
    '.pyc': 'application/x-python-code',
    '.ps': 'application/postscript',
    '.qt': 'video/quicktime',
    '.ras': 'image/x-cmu-raster',
    '.rgb': 'image/x-rgb',
    '.rdf': 'application/xml',
    '.roff': 'application/x-troff',
    '.rtf': 'application/rtf',
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
    '.xml': 'text/xml',
    '.xsl': 'application/xml',
    '.xpm': 'image/x-xpixmap',
    '.xwd': 'image/x-xwindowdump',
    '.zip': 'application/zip',
    }
