#
# nturl2path convert a NT pathname to a file URL and 
# vice versa  

def url2pathname(url):
	""" Convert a URL to a DOS path...
	Currently only works for absolute paths

		///C|/foo/bar/spam.foo

			becomes

		C:\foo\bar\spam.foo
	"""
	import string
	comp = string.splitfields(url, '|')
	if len(comp) != 2 or comp[0][-1] not in string.letters:
		error = 'Bad URL: ' + url
		raise IOError, error
	drive = string.upper(comp[0][-1])
	components = string.splitfields(comp[1], '/')
	path = drive + ':'
	for  comp in components:
		if comp:
			path = path + '\\' + comp
	return path

def pathname2url(p):

	""" Convert a DOS path name to a file url...
	Currently only works for absolute paths

		C:\foo\bar\spam.foo

			becomes

		///C|/foo/bar/spam.foo
	"""

	import string
	comp = string.splitfields(p, ':')
	if len(comp) != 2 or len(comp[0]) > 1:
		error = 'Bad path: ' + p
		raise IOError, error

	drive = string.upper(comp[0])
	components = string.splitfields(comp[1], '\\')
	path = '///' + drive + '|'
	for comp in components:
		if comp:
			path = path + '/' + comp
	return path
