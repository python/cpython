# Parse (absolute and relative) URLs according to latest internet draft:

# Uniform Resource Identifiers Working Group                  R. Fielding
# INTERNET-DRAFT                                                UC Irvine
# Expires February 24, 1995                               August 24, 1994
#
#                   Relative Uniform Resource Locators
#                  <draft-ietf-uri-relative-url-00.txt>

# Standard/builtin Python modules
import string

# A classification of schemes ('' means apply by default)
uses_relative = ['ftp', 'http', 'gopher', 'nntp', 'wais', 'file',
		 'prospero', '']
uses_netloc = ['ftp', 'http', 'gopher', 'nntp', 'telnet', 'wais',
	       'file', 'prospero', '']
non_hierarchical = ['gopher', 'mailto', 'news', 'telnet', 'wais']
uses_params = ['ftp', 'prospero', '']
uses_query = ['http', 'wais', '']
uses_fragment = ['ftp', 'http', 'gopher', 'news', 'nntp', 'wais',
		 'file', 'prospero', '']

# Characters valid in scheme names
scheme_chars = string.letters + string.digits + '+-.'

# Parse a URL into 6 components:
# <scheme>://<netloc>/<path>;<params>?<query>#<fragment>
# Return a 6-tuple: (scheme, netloc, path, params, query, fragment).
# Note that we don't break the components up in smaller bits
# (e.g. netloc is a single string) and we don't expand % escapes.
def urlparse(url, scheme = '', allow_framents = 1):
	netloc = ''
	path = ''
	params = ''
	query = ''
	fragment = ''
	i = string.find(url, ':')
	if i > 0:
		for c in url[:i]:
			if c not in scheme_chars:
				break
		else:
			scheme, url = string.lower(url[:i]), url[i+1:]
	if scheme in uses_netloc:
		if url[:2] == '//':
			i = string.find(url, '/', 2)
			if i < 0:
				i = len(url)
			netloc, url = url[2:i], url[i:]
	if allow_framents and scheme in uses_fragment:
		i = string.rfind(url, '#')
		if i >= 0:
			url, fragment = url[:i], url[i+1:]
	if scheme in uses_query:
		i = string.find(url, '?')
		if i >= 0:
			url, query = url[:i], url[i+1:]
	if scheme in uses_params:
		i = string.find(url, ';')
		if i >= 0:
			url, params = url[:i], url[i+1:]
	return scheme, netloc, url, params, query, fragment

# Put a parsed URL back together again.  This may result in a slightly
# different, but equivalent URL, if the URL that was parsed originally
# had redundant delimiters, e.g. a ? with an empty query (the draft
# states that these are equivalent).
def urlunparse((scheme, netloc, url, params, query, fragment)):
	if netloc:
		if url[:1] != '/': url = '/' + url
		url = '//' + netloc + url
	if scheme:
		url = scheme + ':' + url
	if params:
		url = url + ';' + params
	if query:
		url = url + '?' + query
	if fragment:
		url = url + '#' + fragment
	return url

# Join a base URL and a possibly relative URL to form an absolute
# interpretation of the latter.
def urljoin(base, url, allow_framents = 1):
	if not base:
		return url
	bscheme, bnetloc, bpath, bparams, bquery, bfragment = \
		  urlparse(base, '', allow_framents)
	scheme, netloc, path, params, query, fragment = \
		urlparse(url, bscheme, allow_framents)
	# XXX Unofficial hack: default netloc to bnetloc even if
	# schemes differ
	if scheme != bscheme and not netloc and \
	   scheme in uses_relative and bscheme in uses_relative and \
	   scheme in uses_netloc and bscheme in uses_netloc:
	   netloc = bnetloc
	   # Strip the port number
	   i = string.find(netloc, '@')
	   if i < 0: i = 0
	   i = string.find(netloc, ':', i)
	   if i >= 0:
		   netloc = netloc[:i]
	if scheme != bscheme or scheme not in uses_relative:
		return urlunparse((scheme, netloc, path,
				   params, query, fragment))
	if scheme in uses_netloc:
		if netloc:
			return urlunparse((scheme, netloc, path,
					   params, query, fragment))
		netloc = bnetloc
	if path[:1] == '/':
		return urlunparse((scheme, netloc, path,
				   params, query, fragment))
	if not path:
		path = bpath
		if not query:
			query = bquery
		return urlunparse((scheme, netloc, path,
				   params, query, fragment))
	i = string.rfind(bpath, '/')
	if i >= 0:
		path = bpath[:i] + '/' + path
	segments = string.splitfields(path, '/')
	if segments[-1] == '.':
		segments[-1] = ''
	while '.' in segments:
		segments.remove('.')
	while 1:
		i = 1
		n = len(segments) - 1
		while i < n:
			if segments[i] == '..' and segments[i-1]:
				del segments[i-1:i+1]
				break
			i = i+1
		else:
			break
	if len(segments) >= 2 and segments[-1] == '..':
		segments[-2:] = ['']
	path = string.joinfields(segments, '/')
	return urlunparse((scheme, netloc, path,
			   params, query, fragment))

test_input = """
      http://a/b/c/d

      g:h        = <URL:g:h>
      http:g     = <URL:http://a/b/c/g>
      http:      = <URL:http://a/b/c/d>
      g          = <URL:http://a/b/c/g>
      ./g        = <URL:http://a/b/c/g>
      g/         = <URL:http://a/b/c/g/>
      /g         = <URL:http://a/g>
      //g        = <URL:http://g>
      ?y         = <URL:http://a/b/c/d?y>
      g?y        = <URL:http://a/b/c/g?y>
      g?y/./x    = <URL:http://a/b/c/g?y/./x>
      .          = <URL:http://a/b/c/>
      ./         = <URL:http://a/b/c/>
      ..         = <URL:http://a/b/>
      ../        = <URL:http://a/b/>
      ../g       = <URL:http://a/b/g>
      ../..      = <URL:http://a/>
      ../../g    = <URL:http://a/g>
      ../../../g = <URL:http://a/../g>
      ./../g     = <URL:http://a/b/g>
      ./g/.      = <URL:http://a/b/c/g/>
      /./g       = <URL:http://a/./g>
      g/./h      = <URL:http://a/b/c/g/h>
      g/../h     = <URL:http://a/b/c/h>
      http:g     = <URL:http://a/b/c/g>
      http:      = <URL:http://a/b/c/d>
"""

def test():
	import sys
	base = ''
	if sys.argv[1:]:
		fn = sys.argv[1]
		if fn == '-':
			fp = sys.stdin
		else:
			fp = open(fn)
	else:
		import StringIO
		fp = StringIO.StringIO(test_input)
	while 1:
		line = fp.readline()
		if not line: break
		words = string.split(line)
		if not words:
			continue
		url = words[0]
		parts = urlparse(url)
		print '%-10s : %s' % (url, parts)
		abs = urljoin(base, url)
		if not base:
			base = abs
		wrapped = '<URL:%s>' % abs
		print '%-10s = %s' % (url, wrapped)
		if len(words) == 3 and words[1] == '=':
			if wrapped != words[2]:
				print 'EXPECTED', words[2], '!!!!!!!!!!'

if __name__ == '__main__':
	test()
