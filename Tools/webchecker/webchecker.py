#! /usr/bin/env python

"""Web tree checker.

This utility is handy to check a subweb of the world-wide web for
errors.  A subweb is specified by giving one or more ``root URLs''; a
page belongs to the subweb if one of the root URLs is an initial
prefix of it.

File URL extension:

In order to easy the checking of subwebs via the local file system,
the interpretation of ``file:'' URLs is extended to mimic the behavior
of your average HTTP daemon: if a directory pathname is given, the
file index.html in that directory is returned if it exists, otherwise
a directory listing is returned.  Now, you can point webchecker to the
document tree in the local file system of your HTTP daemon, and have
most of it checked.  In fact the default works this way if your local
web tree is located at /usr/local/etc/httpd/htdpcs (the default for
the NCSA HTTP daemon and probably others).

Reports printed:

When done, it reports links to pages outside the web (unless -q is
specified), and pages with bad links within the subweb.  When
interrupted, it print those same reports for the pages that it has
checked already.

In verbose mode, additional messages are printed during the
information gathering phase.  By default, it prints a summary of its
work status every 50 URLs (adjustable with the -r option), and it
reports errors as they are encountered.  Use the -q option to disable
this output.

Checkpoint feature:

Whether interrupted or not, it dumps its state (a Python pickle) to a
checkpoint file and the -R option allows it to restart from the
checkpoint (assuming that the pages on the subweb that were already
processed haven't changed).  Even when it has run till completion, -R
can still be useful -- it will print the reports again, and -Rq prints
the errors only.  In this case, the checkpoint file is not written
again.  The checkpoint file can be set with the -d option.

The checkpoint file is written as a Python pickle.  Remember that
Python's pickle module is currently quite slow.  Give it the time it
needs to load and save the checkpoint file.  When interrupted while
writing the checkpoint file, the old checkpoint file is not
overwritten, but all work done in the current run is lost.

Miscellaneous:

- Webchecker honors the "robots.txt" convention.  Thanks to Skip
Montanaro for his robotparser.py module (included in this directory)!
The agent name is hardwired to "webchecker".  URLs that are disallowed
by the robots.txt file are reported as external URLs.

- Because the HTML parser is a bit slow, very large HTML files are
skipped.  The size limit can be set with the -m option.

- Before fetching a page, it guesses its type based on its extension.
If it is a known extension and the type is not text/http, the page is
not fetched.  This is a huge optimization but occasionally it means
links can be missed.  The mimetypes.py module (also in this directory)
has a built-in table mapping most currently known suffixes, and in
addition attempts to read the mime.types configuration files in the
default locations of Netscape and the NCSA HTTP daemon.

- It only follows links indicated by <A> tags.  It doesn't follow
links in <FORM> or <IMG> or whatever other tags might contain
hyperlinks.  It does honor the <BASE> tag.

- Checking external links is not done by default; use -x to enable
this feature.  This is done because checking external links usually
takes a lot of time.  When enabled, this check is executed during the
report generation phase (so -x is ignored when -q is specified).  Even
when -x is enabled, only ``http:'' URLs are checked.


Usage: webchecker.py [option] ... [rooturl] ...

Options:

-R        -- restart from checkpoint file
-d file   -- checkpoint filename (default %(DUMPFILE)s)
-m bytes  -- skip HTML pages larger than this size (default %(MAXPAGE)d)
-q        -- quiet operation (also suppresses external links report)
-r number -- number of links processed per round (default %(ROUNDSIZE)d)
-v        -- verbose operation; repeating -v will increase verbosity
-x        -- check external links (during report phase)

Arguments:

rooturl   -- URL to start checking
             (default %(DEFROOT)s)

"""

__version__ = "0.1"


import sys
import os
from types import *
import string
import StringIO
import getopt
import pickle

import urllib
import urlparse
import htmllib
import formatter

import mimetypes
import robotparser


# Tunable parameters
DEFROOT = "file:/usr/local/etc/httpd/htdocs/"	# Default root URL
MAXPAGE = 50000				# Ignore files bigger than this
ROUNDSIZE = 50				# Number of links processed per round
DUMPFILE = "@webchecker.pickle"		# Pickled checkpoint
AGENTNAME = "webchecker"		# Agent name for robots.txt parser


# Global variables
verbose = 1
maxpage = MAXPAGE
roundsize = ROUNDSIZE


def main():
    global verbose, maxpage, roundsize
    dumpfile = DUMPFILE
    restart = 0
    checkext = 0

    try:
	opts, args = getopt.getopt(sys.argv[1:], 'Rd:m:qr:vx')
    except getopt.error, msg:
	sys.stdout = sys.stderr
	print msg
	sys.exit(2)
    for o, a in opts:
	if o == '-R':
	    restart = 1
	if o == '-d':
	    dumpfile = a
	if o == '-m':
	    maxpage = string.atoi(a)
	if o == '-q':
	    verbose = 0
	if o == '-r':
	    roundsize = string.atoi(a)
	if o == '-v':
	    verbose = verbose + 1
	if o == '-x':
	    checkext = 1

    if verbose:
	print AGENTNAME, "version", __version__

    if restart:
	if verbose > 0:
	    print "Loading checkpoint from %s ..." % dumpfile
	f = open(dumpfile, "rb")
	c = pickle.load(f)
	f.close()
	if verbose > 0:
	    print "Done."
	    print "Root:", string.join(c.roots, "\n      ")
    else:
	c = Checker()
	if not args:
	    args.append(DEFROOT)

    for arg in args:
	c.addroot(arg)

    if not c.todo:
	needsave = 0
    else:
	needsave = 1
    try:
	c.run()
    except KeyboardInterrupt:
	if verbose > 0:
	    print "[run interrupted]"
    try:
	c.report(checkext)
    except KeyboardInterrupt:
	if verbose > 0:
	    print "[report interrupted]"
    if not needsave:
	if verbose > 0:
	    print
	    print "No need to save checkpoint"
    elif dumpfile:
	if verbose > 0:
	    print
	    print "Saving checkpoint to %s ..." % dumpfile
	newfile = dumpfile + ".new"
	f = open(newfile, "wb")
	pickle.dump(c, f)
	f.flush()
	f.close()
	try:
	    os.unlink(dumpfile)
	except os.error:
	    pass
	os.rename(newfile, dumpfile)
	if verbose > 0:
	    print "Done."
	    if dumpfile == DUMPFILE:
		print "Use ``%s -R'' to restart." % sys.argv[0]
	    else:
		print "Use ``%s -R -d %s'' to restart." % (sys.argv[0],
							   dumpfile)


class Checker:

    def __init__(self):
	self.roots = []
	self.todo = {}
	self.done = {}
	self.ext = {}
	self.bad = {}
	self.urlopener = MyURLopener()
	self.round = 0
	self.robots = {}

    def __getstate__(self):
	return (self.roots, self.todo, self.done,
		self.ext, self.bad, self.round)

    def __setstate__(self, state):
	(self.roots, self.todo, self.done,
	 self.ext, self.bad, self.round) = state
	for root in self.roots:
	    self.addrobot(root)

    def addroot(self, root):
	if root not in self.roots:
	    self.roots.append(root)
	    self.todo[root] = []
	    self.addrobot(root)

    def addrobot(self, root):
	url = urlparse.urljoin(root, "/robots.txt")
	self.robots[root] = rp = robotparser.RobotFileParser()
	if verbose > 2:
	    print "Parsing", url
	    rp.debug = 1
	rp.set_url(url)
	try:
	    rp.read()
	except IOError, msg:
	    if verbose > 1:
		print "I/O error parsing", url, ":", msg

    def run(self):
	while self.todo:
	    self.round = self.round + 1
	    if verbose > 0:
		print
		print "Round", self.round,
		print "(%d to do, %d done, %d external, %d bad)" % (
		    len(self.todo), len(self.done),
		    len(self.ext), len(self.bad))
		print
	    urls = self.todo.keys()[:roundsize]
	    for url in urls:
		self.dopage(url)
		self.done[url] = self.todo[url]
		del self.todo[url]

    def report(self, checkext=0):
	print
	if not self.todo: print "Final",
	else: print "Interim",
	print "Report (%d to do, %d done, %d external, %d bad)" % (
	    len(self.todo), len(self.done),
	    len(self.ext), len(self.bad))
	if verbose > 0:
	    self.report_extrefs(checkext)
	# Report errors last because the output may get truncated
	self.report_errors()

    def report_extrefs(self, checkext=0):
	if not self.ext:
	    print
	    print "No external URLs"
	    return
	print
	if checkext:
	    print "External URLs (checking validity):"
	else:
	    print "External URLs (not checked):"
	print
	urls = self.ext.keys()
	urls.sort()
	for url in urls:
	    show("HREF ", url, " from", self.ext[url])
	    if not checkext:
		continue
	    if url[:7] == 'mailto:':
		if verbose > 2: print "Not checking", url
		continue
	    if verbose > 2: print "Checking", url, "..."
	    try:
		f = self.urlopener.open(url)
		f.close()
		if verbose > 3: print "OK"
	    except IOError, msg:
		msg = sanitize(msg)
		print "Error", msg
		self.bad[url] = msg

    def report_errors(self):
	if not self.bad:
	    print
	    print "No errors"
	    return
	print
	print "Error Report:"
	urls = self.bad.keys()
	urls.sort()
	bysource = {}
	for url in urls:
	    try:
		origins = self.done[url]
	    except KeyError:
		try:
		    origins = self.todo[url]
		except KeyError:
		    origins = self.ext[url]
	    for source, rawlink in origins:
		triple = url, rawlink, self.bad[url]
		try:
		    bysource[source].append(triple)
		except KeyError:
		    bysource[source] = [triple]
	sources = bysource.keys()
	sources.sort()
	for source in sources:
	    triples = bysource[source]
	    print
	    if len(triples) > 1:
		print len(triples), "Errors in", source
	    else:
		print "Error in", source
	    for url, rawlink, msg in triples:
		print "  HREF", url,
		if rawlink != url: print "(%s)" % rawlink,
		print
		print "   msg", msg

    def dopage(self, url):
	if verbose > 1:
	    if verbose > 2:
		show("Page  ", url, "  from", self.todo[url])
	    else:
		print "Page  ", url
	page = self.getpage(url)
	if not page:
	    return
	for info in page.getlinkinfos():
	    link, rawlink = info
	    origin = url, rawlink
	    if not self.inroots(link):
		try:
		    self.ext[link].append(origin)
		    if verbose > 3:
			print "  New ext link", link,
			if link != rawlink: print "(%s)" % rawlink,
			print
		except KeyError:
		    if verbose > 3:
			print "  Seen ext link", link,
			if link != rawlink: print "(%s)" % rawlink,
			print
		    self.ext[link] = [origin]
	    elif self.done.has_key(link):
		if verbose > 3:
		    print "  Done link", link
		self.done[link].append(origin)
	    elif self.todo.has_key(link):
		if verbose > 3:
		    print "  Seen todo link", link
		self.todo[link].append(origin)
	    else:
		if verbose > 3:
		    print "  New todo link", link
		self.todo[link] = [origin]

    def inroots(self, url):
	for root in self.roots:
	    if url[:len(root)] == root:
		return self.robots[root].can_fetch(AGENTNAME, url)
	return 0

    def getpage(self, url):
	ctype, encoding = mimetypes.guess_type(url)
	if encoding:
	    if verbose > 2:
		print "  Won't bother, URL suggests encoding %s" % `encoding`
	    return None
	if ctype and ctype != 'text/html':
	    if verbose > 2:
		print "  Won't bother, URL suggests mime type %s" % `ctype`
	    return None
	try:
	    f = self.urlopener.open(url)
	except IOError, msg:
	    msg = sanitize(msg)
	    if verbose > 0:
		print "Error ", msg
	    if verbose > 0:
		show(" HREF ", url, "  from", self.todo[url])
	    self.bad[url] = msg
	    return None
	nurl = f.geturl()
	info = f.info()
	if info.has_key('content-type'):
	    ctype = string.lower(info['content-type'])
	if nurl != url:
	    if verbose > 1:
		print " Redirected to", nurl
	    if not ctype:
		ctype, encoding = mimetypes.guess_type(nurl)
	if ctype != 'text/html':
	    f.close()
	    if verbose > 2:
		print "  Not HTML, mime type", ctype
	    return None
	text = f.read()
	f.close()
	return Page(text, nurl)


class Page:

    def __init__(self, text, url):
	self.text = text
	self.url = url

    def getlinkinfos(self):
	size = len(self.text)
	if size > maxpage:
	    if verbose > 0:
		print "Skip huge file", self.url
		print "  (%.0f Kbytes)" % (size*0.001)
	    return []
	if verbose > 2:
	    print "  Parsing", self.url, "(%d bytes)" % size
	parser = MyHTMLParser(formatter.NullFormatter())
	parser.feed(self.text)
	parser.close()
	rawlinks = parser.getlinks()
	base = urlparse.urljoin(self.url, parser.getbase() or "")
	infos = []
	for rawlink in rawlinks:
	    t = urlparse.urlparse(rawlink)
	    t = t[:-1] + ('',)
	    rawlink = urlparse.urlunparse(t)
	    link = urlparse.urljoin(base, rawlink)
	    infos.append((link, rawlink))
	return infos


class MyStringIO(StringIO.StringIO):

    def __init__(self, url, info):
	self.__url = url
	self.__info = info
	StringIO.StringIO.__init__(self)

    def info(self):
	return self.__info

    def geturl(self):
	return self.__url


class MyURLopener(urllib.FancyURLopener):

    http_error_default = urllib.URLopener.http_error_default

    def open_file(self, url):
	path = urllib.url2pathname(urllib.unquote(url))
	if path[-1] != os.sep:
	    url = url + '/'
	if os.path.isdir(path):
	    indexpath = os.path.join(path, "index.html")
	    if os.path.exists(indexpath):
		return self.open_file(url + "index.html")
	    try:
		names = os.listdir(path)
	    except os.error, msg:
		raise IOError, msg, sys.exc_traceback
	    names.sort()
	    s = MyStringIO("file:"+url, {'content-type': 'text/html'})
	    s.write('<BASE HREF="file:%s">\n' %
		    urllib.quote(os.path.join(path, "")))
	    for name in names:
		q = urllib.quote(name)
		s.write('<A HREF="%s">%s</A>\n' % (q, q))
	    s.seek(0)
	    return s
	return urllib.FancyURLopener.open_file(self, path)


class MyHTMLParser(htmllib.HTMLParser):

    def __init__(*args):
	self = args[0]
	self.base = None
	self.links = []
	apply(htmllib.HTMLParser.__init__, args)

    def start_a(self, attributes):
	for name, value in attributes:
	    if name == 'href' and value and value not in self.links:
		self.links.append(string.strip(value))

    def do_base(self, attributes):
	for name, value in attributes:
	    if name == 'href' and value:
		if verbose > 1:
		    print "  Base", value
		self.base = value

    def getlinks(self):
	return self.links

    def getbase(self):
	return self.base


def show(p1, link, p2, origins):
    print p1, link
    i = 0
    for source, rawlink in origins:
	i = i+1
	if i == 2:
	    p2 = ' '*len(p2)
	print p2, source,
	if rawlink != link: print "(%s)" % rawlink,
	print


def sanitize(msg):
    if (type(msg) == TupleType and
	len(msg) >= 4 and
	msg[0] == 'http error' and
	type(msg[3]) == InstanceType):
	# Remove the Message instance -- it may contain
	# a file object which prevents pickling.
	msg = msg[:3] + msg[4:]
    return msg


if __name__ == '__main__':
    main()
