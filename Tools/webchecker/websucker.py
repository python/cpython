#! /usr/bin/env python

"""A variant on webchecker that creates a mirror copy of a remote site."""

__version__ = "0.1"

import os
import sys
import string
import urllib
import getopt

import webchecker
verbose = webchecker.verbose

def main():
    global verbose
    try:
	opts, args = getopt.getopt(sys.argv[1:], "qv")
    except getopt.error, msg:
	print msg
	print "usage:", sys.argv[0], "[-v] ... [rooturl] ..."
	return 2
    for o, a in opts:
	if o == "-q":
	    webchecker.verbose = verbose = 0
	if o == "-v":
	    webchecker.verbose = verbose = verbose + 1
    c = Sucker(0)
    c.urlopener.addheaders = [
	    ('User-agent', 'websucker/%s' % __version__),
	]
    for arg in args:
	print "Adding root", arg
	c.addroot(arg)
    print "Run..."
    c.run()

class Sucker(webchecker.Checker):

    # Alas, had to copy this to make one change...
    def getpage(self, url):
	if url[:7] == 'mailto:' or url[:5] == 'news:':
	    if verbose > 1: print " Not checking mailto/news URL"
	    return None
	isint = self.inroots(url)
	if not isint and not self.checkext:
	    if verbose > 1: print " Not checking ext link"
	    return None
	path = self.savefilename(url)
	saved = 0
	try:
	    f = open(path, "rb")
	except IOError:
	    try:
		f = self.urlopener.open(url)
	    except IOError, msg:
		msg = webchecker.sanitize(msg)
		if verbose > 0:
		    print "Error ", msg
		if verbose > 0:
		    webchecker.show(" HREF ", url, "  from", self.todo[url])
		self.setbad(url, msg)
		return None
	    if not isint:
		if verbose > 1: print " Not gathering links from ext URL"
		safeclose(f)
		return None
	    nurl = f.geturl()
	    if nurl != url:
		path = self.savefilename(nurl)
	    info = f.info()
	else:
	    if verbose: print "Loading cached URL", url
	    saved = 1
	    nurl = url
	    info = {}
	    if url[-1:] == "/":
		info["content-type"] = "text/html"
	text = f.read()
	if not saved: self.savefile(text, path)
	if info.has_key('content-type'):
	    ctype = string.lower(info['content-type'])
	else:
	    ctype = None
	if nurl != url:
	    if verbose > 1:
		print " Redirected to", nurl
	if not ctype:
	    ctype, encoding = webchecker.mimetypes.guess_type(nurl)
	if ctype != 'text/html':
	    webchecker.safeclose(f)
	    if verbose > 1:
		print " Not HTML, mime type", ctype
	    return None
	f.close()
	return webchecker.Page(text, nurl)

    def savefile(self, text, path):
	dir, base = os.path.split(path)
	makedirs(dir)
	f = open(path, "wb")
	f.write(text)
	f.close()
	print "saved", path

    def savefilename(self, url):
	type, rest = urllib.splittype(url)
	host, path = urllib.splithost(rest)
	while path[:1] == "/": path = path[1:]
	user, host = urllib.splituser(host)
	host, port = urllib.splitnport(host)
	host = string.lower(host)
	path = os.path.join(host, path)
	if path[-1] == "/": path = path + "index.html"
	if os.sep != "/":
	    path = string.join(string.split(path, "/"), os.sep)
	return path

def makedirs(dir):
    if not dir or os.path.exists(dir):
	return
    head, tail = os.path.split(dir)
    if not tail:
	print "Huh?  Don't know how to make dir", dir
	return
    makedirs(head)
    os.mkdir(dir, 0777)

if __name__ == '__main__':
    sys.exit(main() or 0)
