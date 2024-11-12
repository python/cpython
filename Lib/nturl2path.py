"""Convert a NT pathname to a file URL and vice versa.

This module only exists to provide OS-specific code
for urllib.requests, thus do not use directly.
"""
# Testing is done through test_urllib.

def url2pathname(url):
    """OS-specific conversion from a relative URL of the 'file' scheme
    to a file system path; not recommended for general use."""
    # e.g.
    #   ///C|/foo/bar/spam.foo
    # and
    #   ///C:/foo/bar/spam.foo
    # become
    #   C:\foo\bar\spam.foo
    import string, urllib.parse
    # Windows itself uses ":" even in URLs.
    url = url.replace(':', '|')
    if not '|' in url:
        # No drive specifier, just convert slashes
        if url[:4] == '////':
            # path is something like ////host/path/on/remote/host
            # convert this to \\host\path\on\remote\host
            # (notice halving of slashes at the start of the path)
            url = url[2:]
        # make sure not to convert quoted slashes :-)
        return urllib.parse.unquote(url.replace('/', '\\'))
    comp = url.split('|')
    if len(comp) != 2 or comp[0][-1] not in string.ascii_letters:
        error = 'Bad URL: ' + url
        raise OSError(error)
    drive = comp[0][-1].upper()
    tail = urllib.parse.unquote(comp[1].replace('/', '\\'))
    return drive + ':' + tail

def pathname2url(p):
    """OS-specific conversion from a file system path to a relative URL
    of the 'file' scheme; not recommended for general use."""
    # e.g.
    #   C:\foo\bar\spam.foo
    # becomes
    #   ///C:/foo/bar/spam.foo
    import ntpath
    import urllib.parse
    drive, tail = ntpath.splitdrive(p.replace('\\', '/'))
    if drive:
        # First, clean up some special forms. We are going to sacrifice the
        # additional information anyway.
        if drive[:4] == '//?/':
            drive = drive[4:]
            if drive[:4].upper() == 'UNC/':
                drive = '//' + drive[4:]

        if drive[1:2] == ':':
            # DOS drive specified. Add three slashes to the start, producing
            # an authority section with a zero-length authority, and a path
            # section starting with a single slash. The colon is *not* quoted.
            letter = urllib.parse.quote(drive[0].upper())
            drive = f'///{letter}:'
        else:
            # No DOS drive specified, just quote the pathname.
            drive = urllib.parse.quote(drive)

    return drive + urllib.parse.quote(tail)
