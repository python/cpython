import urlparse

errors = 0

RFC1808_BASE = "http://a/b/c/d;p?q#f"

for url, expected in [('http://www.python.org',
                       ('http', 'www.python.org', '', '', '', '')),
                      ('http://www.python.org#abc',
                       ('http', 'www.python.org', '', '', '', 'abc')),
                      ('http://www.python.org/#abc',
                       ('http', 'www.python.org', '/', '', '', 'abc')),
                      (RFC1808_BASE,
                       ('http', 'a', '/b/c/d', 'p', 'q', 'f')),
                      ('file:///tmp/junk.txt',
                       ('file', '', '/tmp/junk.txt', '', '', '')),
                      ]:
    result = urlparse.urlparse(url)
    print "%-13s = %r" % (url, result)
    if result != expected:
        errors += 1
        print "urlparse(%r)" % url
        print ("expected %r,\n"
               "     got %r") % (expected, result)
    # put it back together and it should be the same
    result2 = urlparse.urlunparse(result)
    assert(result2 == url)
print

def checkJoin(relurl, expected):
    global errors
    result = urlparse.urljoin(RFC1808_BASE, relurl)
    print "%-13s = %r" % (relurl, result)
    if result != expected:
        errors += 1
        print "urljoin(%r, %r)" % (RFC1808_BASE, relurl)
        print ("expected %r,\n"
               "     got %r") % (expected, result)

print "urlparse.urljoin() tests"
print

# "normal" cases from RFC 1808:
checkJoin('g:h', 'g:h')
checkJoin('g', 'http://a/b/c/g')
checkJoin('./g', 'http://a/b/c/g')
checkJoin('g/', 'http://a/b/c/g/')
checkJoin('/g', 'http://a/g')
checkJoin('//g', 'http://g')
checkJoin('?y', 'http://a/b/c/d;p?y')
checkJoin('g?y', 'http://a/b/c/g?y')
checkJoin('g?y/./x', 'http://a/b/c/g?y/./x')
checkJoin('#s', 'http://a/b/c/d;p?q#s')
checkJoin('g#s', 'http://a/b/c/g#s')
checkJoin('g#s/./x', 'http://a/b/c/g#s/./x')
checkJoin('g?y#s', 'http://a/b/c/g?y#s')
checkJoin(';x', 'http://a/b/c/d;x')
checkJoin('g;x', 'http://a/b/c/g;x')
checkJoin('g;x?y#s', 'http://a/b/c/g;x?y#s')
checkJoin('.', 'http://a/b/c/')
checkJoin('./', 'http://a/b/c/')
checkJoin('..', 'http://a/b/')
checkJoin('../', 'http://a/b/')
checkJoin('../g', 'http://a/b/g')
checkJoin('../..', 'http://a/')
checkJoin('../../', 'http://a/')
checkJoin('../../g', 'http://a/g')

# "abnormal" cases from RFC 1808:
checkJoin('', 'http://a/b/c/d;p?q#f')
checkJoin('../../../g', 'http://a/../g')
checkJoin('../../../../g', 'http://a/../../g')
checkJoin('/./g', 'http://a/./g')
checkJoin('/../g', 'http://a/../g')
checkJoin('g.', 'http://a/b/c/g.')
checkJoin('.g', 'http://a/b/c/.g')
checkJoin('g..', 'http://a/b/c/g..')
checkJoin('..g', 'http://a/b/c/..g')
checkJoin('./../g', 'http://a/b/g')
checkJoin('./g/.', 'http://a/b/c/g/')
checkJoin('g/./h', 'http://a/b/c/g/h')
checkJoin('g/../h', 'http://a/b/c/h')

# RFC 1808 and RFC 1630 disagree on these (according to RFC 1808),
# so we'll not actually run these tests (which expect 1808 behavior).
#checkJoin('http:g', 'http:g')
#checkJoin('http:', 'http:')

print errors, "errors"

# One more test backported from 2.3
for u in ['Python', './Python']:
    if urlparse.urlunparse(urlparse.urlparse(u)) != u:
        print "*** urlparse/urlunparse failure for", `u`
