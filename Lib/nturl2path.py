"""Convert a NT pathname to a file URL and vice versa."""

def url2pathname(url):
	r"""Convert a URL to a DOS path.

		///C|/foo/bar/spam.foo

			becomes

		C:\foo\bar\spam.foo
	"""
	import string, urllib
	if not '|' in url:
	    # No drive specifier, just convert slashes
	    if url[:4] == '////':
	        # path is something like ////host/path/on/remote/host
	        # convert this to \\host\path\on\remote\host
	        # (notice halving of slashes at the start of the path)
	        url = url[2:]
	    components = string.split(url, '/')
	    # make sure not to convert quoted slashes :-)
	    return urllib.unquote(string.join(components, '\\'))
	comp = string.split(url, '|')
	if len(comp) != 2 or comp[0][-1] not in string.letters:
		error = 'Bad URL: ' + url
		raise IOError, error
	drive = string.upper(comp[0][-1])
	components = string.split(comp[1], '/')
	path = drive + ':'
	for  comp in components:
		if comp:
			path = path + '\\' + urllib.unquote(comp)
	return path

def pathname2url(p):
	r"""Convert a DOS path name to a file url.

		C:\foo\bar\spam.foo

			becomes

		///C|/foo/bar/spam.foo
	"""

	import string, urllib
	if not ':' in p:
	    # No drive specifier, just convert slashes and quote the name
	    if p[:2] == '\\\\':
	        # path is something like \\host\path\on\remote\host
	        # convert this to ////host/path/on/remote/host
	        # (notice doubling of slashes at the start of the path)
	        p = '\\\\' + p
	    components = string.split(p, '\\')
	    return urllib.quote(string.join(components, '/'))
	comp = string.split(p, ':')
	if len(comp) != 2 or len(comp[0]) > 1:
		error = 'Bad path: ' + p
		raise IOError, error

	drive = urllib.quote(string.upper(comp[0]))
	components = string.split(comp[1], '\\')
	path = '///' + drive + '|'
	for comp in components:
		if comp:
			path = path + '/' + urllib.quote(comp)
	return path
