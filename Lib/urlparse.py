"""Parse (absolute and relative) URLs.

See RFC 1808: "Relative Uniform Resource Locators", by R. Fielding,
UC Irvine, June 1995.
"""

__all__ = ["urlparse", "urlunparse", "urljoin", "urldefrag",
           "urlsplit", "urlunsplit"]

# A classification of schemes ('' means apply by default)
uses_relative = ['ftp', 'http', 'gopher', 'nntp', 'wais', 'file',
                 'https', 'shttp',
                 'prospero', 'rtsp', 'rtspu', '']
uses_netloc = ['ftp', 'http', 'gopher', 'nntp', 'telnet', 'wais',
               'file',
               'https', 'shttp', 'snews',
               'prospero', 'rtsp', 'rtspu', '']
non_hierarchical = ['gopher', 'hdl', 'mailto', 'news', 'telnet', 'wais',
                    'snews', 'sip',
                    ]
uses_params = ['ftp', 'hdl', 'prospero', 'http',
               'https', 'shttp', 'rtsp', 'rtspu', 'sip',
               '']
uses_query = ['http', 'wais',
              'https', 'shttp',
              'gopher', 'rtsp', 'rtspu', 'sip',
              '']
uses_fragment = ['ftp', 'hdl', 'http', 'gopher', 'news', 'nntp', 'wais',
                 'https', 'shttp', 'snews',
                 'file', 'prospero', '']

# Characters valid in scheme names
scheme_chars = ('abcdefghijklmnopqrstuvwxyz'
                'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
                '0123456789'
                '+-.')

MAX_CACHE_SIZE = 20
_parse_cache = {}

def clear_cache():
    """Clear the parse cache."""
    global _parse_cache
    _parse_cache = {}


def urlparse(url, scheme='', allow_fragments=1):
    """Parse a URL into 6 components:
    <scheme>://<netloc>/<path>;<params>?<query>#<fragment>
    Return a 6-tuple: (scheme, netloc, path, params, query, fragment).
    Note that we don't break the components up in smaller bits
    (e.g. netloc is a single string) and we don't expand % escapes."""
    tuple = urlsplit(url, scheme, allow_fragments)
    scheme, netloc, url, query, fragment = tuple
    if scheme in uses_params and ';' in url:
        url, params = _splitparams(url)
    else:
        params = ''
    return scheme, netloc, url, params, query, fragment

def _splitparams(url):
    if '/'  in url:
        i = url.find(';', url.rfind('/'))
        if i < 0:
            return url, ''
    else:
        i = url.find(';')
    return url[:i], url[i+1:]

def urlsplit(url, scheme='', allow_fragments=1):
    """Parse a URL into 5 components:
    <scheme>://<netloc>/<path>?<query>#<fragment>
    Return a 5-tuple: (scheme, netloc, path, query, fragment).
    Note that we don't break the components up in smaller bits
    (e.g. netloc is a single string) and we don't expand % escapes."""
    key = url, scheme, allow_fragments
    cached = _parse_cache.get(key, None)
    if cached:
        return cached
    if len(_parse_cache) >= MAX_CACHE_SIZE: # avoid runaway growth
        clear_cache()
    netloc = query = fragment = ''
    i = url.find(':')
    if i > 0:
        if url[:i] == 'http': # optimize the common case
            scheme = url[:i].lower()
            url = url[i+1:]
            if url[:2] == '//':
                i = url.find('/', 2)
                if i < 0:
                    i = url.find('#')
                    if i < 0:
                        i = len(url)
                netloc = url[2:i]
                url = url[i:]
            if allow_fragments and '#' in url:
                url, fragment = url.split('#', 1)
            if '?' in url:
                url, query = url.split('?', 1)
            tuple = scheme, netloc, url, query, fragment
            _parse_cache[key] = tuple
            return tuple
        for c in url[:i]:
            if c not in scheme_chars:
                break
        else:
            scheme, url = url[:i].lower(), url[i+1:]
    if scheme in uses_netloc:
        if url[:2] == '//':
            i = url.find('/', 2)
            if i < 0:
                i = len(url)
            netloc, url = url[2:i], url[i:]
    if allow_fragments and scheme in uses_fragment and '#' in url:
        url, fragment = url.split('#', 1)
    if scheme in uses_query and '?' in url:
        url, query = url.split('?', 1)
    tuple = scheme, netloc, url, query, fragment
    _parse_cache[key] = tuple
    return tuple

def urlunparse((scheme, netloc, url, params, query, fragment)):
    """Put a parsed URL back together again.  This may result in a
    slightly different, but equivalent URL, if the URL that was parsed
    originally had redundant delimiters, e.g. a ? with an empty query
    (the draft states that these are equivalent)."""
    if params:
        url = "%s;%s" % (url, params)
    return urlunsplit((scheme, netloc, url, query, fragment))

def urlunsplit((scheme, netloc, url, query, fragment)):
    if netloc or (scheme and scheme in uses_netloc and url[:2] != '//'):
        if url and url[:1] != '/': url = '/' + url
        url = '//' + (netloc or '') + url
    if scheme:
        url = scheme + ':' + url
    if query:
        url = url + '?' + query
    if fragment:
        url = url + '#' + fragment
    return url

def urljoin(base, url, allow_fragments = 1):
    """Join a base URL and a possibly relative URL to form an absolute
    interpretation of the latter."""
    if not base:
        return url
    if not url:
        return base
    bscheme, bnetloc, bpath, bparams, bquery, bfragment = \
            urlparse(base, '', allow_fragments)
    scheme, netloc, path, params, query, fragment = \
            urlparse(url, bscheme, allow_fragments)
    if scheme != bscheme or scheme not in uses_relative:
        return url
    if scheme in uses_netloc:
        if netloc:
            return urlunparse((scheme, netloc, path,
                               params, query, fragment))
        netloc = bnetloc
    if path[:1] == '/':
        return urlunparse((scheme, netloc, path,
                           params, query, fragment))
    if not path:
        if not params:
            params = bparams
            if not query:
                query = bquery
        return urlunparse((scheme, netloc, bpath,
                           params, query, fragment))
    segments = bpath.split('/')[:-1] + path.split('/')
    # XXX The stuff below is bogus in various ways...
    if segments[-1] == '.':
        segments[-1] = ''
    while '.' in segments:
        segments.remove('.')
    while 1:
        i = 1
        n = len(segments) - 1
        while i < n:
            if (segments[i] == '..'
                and segments[i-1] not in ('', '..')):
                del segments[i-1:i+1]
                break
            i = i+1
        else:
            break
    if segments == ['', '..']:
        segments[-1] = ''
    elif len(segments) >= 2 and segments[-1] == '..':
        segments[-2:] = ['']
    return urlunparse((scheme, netloc, '/'.join(segments),
                       params, query, fragment))

def urldefrag(url):
    """Removes any existing fragment from URL.

    Returns a tuple of the defragmented URL and the fragment.  If
    the URL contained no fragments, the second element is the
    empty string.
    """
    if '#' in url:
        s, n, p, a, q, frag = urlparse(url)
        defrag = urlunparse((s, n, p, a, q, ''))
        return defrag, frag
    else:
        return url, ''


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
      http:?y         = <URL:http://a/b/c/d?y>
      http:g?y        = <URL:http://a/b/c/g?y>
      http:g?y/./x    = <URL:http://a/b/c/g?y/./x>
"""
# XXX The result for //g is actually http://g/; is this a problem?

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
        words = line.split()
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
