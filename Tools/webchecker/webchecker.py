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

Report printed:

When done, it reports pages with bad links within the subweb.  When
interrupted, it reports for the pages that it has checked already.

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

- You may find the (Tk-based) GUI version easier to use.  See wcgui.py.

- Webchecker honors the "robots.txt" convention.  Thanks to Skip
Montanaro for his robotparser.py module (included in this directory)!
The agent name is hardwired to "webchecker".  URLs that are disallowed
by the robots.txt file are reported as external URLs.

- Because the SGML parser is a bit slow, very large SGML files are
skipped.  The size limit can be set with the -m option.

- When the server or protocol does not tell us a file's type, we guess
it based on the URL's suffix.  The mimetypes.py module (also in this
directory) has a built-in table mapping most currently known suffixes,
and in addition attempts to read the mime.types configuration files in
the default locations of Netscape and the NCSA HTTP daemon.

- We follows links indicated by <A>, <FRAME> and <IMG> tags.  We also
honor the <BASE> tag.

- Checking external links is now done by default; use -x to *disable*
this feature.  External links are now checked during normal
processing.  (XXX The status of a checked link could be categorized
better.  Later...)


Usage: webchecker.py [option] ... [rooturl] ...

Options:

-R        -- restart from checkpoint file
-d file   -- checkpoint filename (default %(DUMPFILE)s)
-m bytes  -- skip HTML pages larger than this size (default %(MAXPAGE)d)
-n        -- reports only, no checking (use with -R)
-q        -- quiet operation (also suppresses external links report)
-r number -- number of links processed per round (default %(ROUNDSIZE)d)
-v        -- verbose operation; repeating -v will increase verbosity
-x        -- don't check external links (these are often slow to check)

Arguments:

rooturl   -- URL to start checking
             (default %(DEFROOT)s)

"""


__version__ = "$Revision$"


import sys
import os
from types import *
import string
import StringIO
import getopt
import pickle

import urllib
import urlparse
import sgmllib

import mimetypes
import robotparser

# Extract real version number if necessary
if __version__[0] == '$':
    _v = string.split(__version__)
    if len(_v) == 3:
        __version__ = _v[1]


# Tunable parameters
DEFROOT = "file:/usr/local/etc/httpd/htdocs/"   # Default root URL
CHECKEXT = 1                            # Check external references (1 deep)
VERBOSE = 1                             # Verbosity level (0-3)
MAXPAGE = 150000                        # Ignore files bigger than this
ROUNDSIZE = 50                          # Number of links processed per round
DUMPFILE = "@webchecker.pickle"         # Pickled checkpoint
AGENTNAME = "webchecker"                # Agent name for robots.txt parser


# Global variables


def main():
    checkext = CHECKEXT
    verbose = VERBOSE
    maxpage = MAXPAGE
    roundsize = ROUNDSIZE
    dumpfile = DUMPFILE
    restart = 0
    norun = 0

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'Rd:m:nqr:vx')
    except getopt.error, msg:
        sys.stdout = sys.stderr
        print msg
        print __doc__%globals()
        sys.exit(2)
    for o, a in opts:
        if o == '-R':
            restart = 1
        if o == '-d':
            dumpfile = a
        if o == '-m':
            maxpage = string.atoi(a)
        if o == '-n':
            norun = 1
        if o == '-q':
            verbose = 0
        if o == '-r':
            roundsize = string.atoi(a)
        if o == '-v':
            verbose = verbose + 1
        if o == '-x':
            checkext = not checkext

    if verbose > 0:
        print AGENTNAME, "version", __version__

    if restart:
        c = load_pickle(dumpfile=dumpfile, verbose=verbose)
    else:
        c = Checker()

    c.setflags(checkext=checkext, verbose=verbose,
               maxpage=maxpage, roundsize=roundsize)

    if not restart and not args:
        args.append(DEFROOT)

    for arg in args:
        c.addroot(arg)

    try:

        if not norun:
            try:
                c.run()
            except KeyboardInterrupt:
                if verbose > 0:
                    print "[run interrupted]"

        try:
            c.report()
        except KeyboardInterrupt:
            if verbose > 0:
                print "[report interrupted]"

    finally:
        if c.save_pickle(dumpfile):
            if dumpfile == DUMPFILE:
                print "Use ``%s -R'' to restart." % sys.argv[0]
            else:
                print "Use ``%s -R -d %s'' to restart." % (sys.argv[0],
                                                           dumpfile)


def load_pickle(dumpfile=DUMPFILE, verbose=VERBOSE):
    if verbose > 0:
        print "Loading checkpoint from %s ..." % dumpfile
    f = open(dumpfile, "rb")
    c = pickle.load(f)
    f.close()
    if verbose > 0:
        print "Done."
        print "Root:", string.join(c.roots, "\n      ")
    return c


class Checker:

    checkext = CHECKEXT
    verbose = VERBOSE
    maxpage = MAXPAGE
    roundsize = ROUNDSIZE

    validflags = tuple(dir())

    def __init__(self):
        self.reset()

    def setflags(self, **kw):
        for key in kw.keys():
            if key not in self.validflags:
                raise NameError, "invalid keyword argument: %s" % str(key)
        for key, value in kw.items():
            setattr(self, key, value)

    def reset(self):
        self.roots = []
        self.todo = {}
        self.done = {}
        self.bad = {}
        self.round = 0
        # The following are not pickled:
        self.robots = {}
        self.errors = {}
        self.urlopener = MyURLopener()
        self.changed = 0

    def __getstate__(self):
        return (self.roots, self.todo, self.done, self.bad, self.round)

    def __setstate__(self, state):
        self.reset()
        (self.roots, self.todo, self.done, self.bad, self.round) = state
        for root in self.roots:
            self.addrobot(root)
        for url in self.bad.keys():
            self.markerror(url)

    def addroot(self, root):
        if root not in self.roots:
            troot = root
            scheme, netloc, path, params, query, fragment = \
                    urlparse.urlparse(root)
            i = string.rfind(path, "/") + 1
            if 0 < i < len(path):
                path = path[:i]
                troot = urlparse.urlunparse((scheme, netloc, path,
                                             params, query, fragment))
            self.roots.append(troot)
            self.addrobot(root)
            self.newlink(root, ("<root>", root))

    def addrobot(self, root):
        root = urlparse.urljoin(root, "/")
        if self.robots.has_key(root): return
        url = urlparse.urljoin(root, "/robots.txt")
        self.robots[root] = rp = robotparser.RobotFileParser()
        if self.verbose > 2:
            print "Parsing", url
            rp.debug = self.verbose > 3
        rp.set_url(url)
        try:
            rp.read()
        except IOError, msg:
            if self.verbose > 1:
                print "I/O error parsing", url, ":", msg

    def run(self):
        while self.todo:
            self.round = self.round + 1
            if self.verbose > 0:
                print
                print "Round %d (%s)" % (self.round, self.status())
                print 
            urls = self.todo.keys()[:self.roundsize]
            for url in urls:
                self.dopage(url)

    def status(self):
        return "%d total, %d to do, %d done, %d bad" % (
            len(self.todo)+len(self.done),
            len(self.todo), len(self.done),
            len(self.bad))

    def report(self):
        print
        if not self.todo: print "Final",
        else: print "Interim",
        print "Report (%s)" % self.status()
        self.report_errors()

    def report_errors(self):
        if not self.bad:
            print
            print "No errors"
            return
        print
        print "Error Report:"
        sources = self.errors.keys()
        sources.sort()
        for source in sources:
            triples = self.errors[source]
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
        if self.verbose > 1:
            if self.verbose > 2:
                self.show("Check ", url, "  from", self.todo[url])
            else:
                print "Check ", url
        page = self.getpage(url)
        if page:
            for info in page.getlinkinfos():
                link, rawlink = info
                origin = url, rawlink
                self.newlink(link, origin)
        self.markdone(url)

    def newlink(self, url, origin):
        if self.done.has_key(url):
            self.newdonelink(url, origin)
        else:
            self.newtodolink(url, origin)

    def newdonelink(self, url, origin):
        self.done[url].append(origin)
        if self.verbose > 3:
            print "  Done link", url

    def newtodolink(self, url, origin):
        if self.todo.has_key(url):
            self.todo[url].append(origin)
            if self.verbose > 3:
                print "  Seen todo link", url
        else:
            self.todo[url] = [origin]
            if self.verbose > 3:
                print "  New todo link", url

    def markdone(self, url):
        self.done[url] = self.todo[url]
        del self.todo[url]
        self.changed = 1

    def inroots(self, url):
        for root in self.roots:
            if url[:len(root)] == root:
                root = urlparse.urljoin(root, "/")
                return self.robots[root].can_fetch(AGENTNAME, url)
        return 0

    def getpage(self, url):
        if url[:7] == 'mailto:' or url[:5] == 'news:':
            if self.verbose > 1: print " Not checking mailto/news URL"
            return None
        isint = self.inroots(url)
        if not isint:
            if not self.checkext:
                if self.verbose > 1: print " Not checking ext link"
                return None
            f = self.openpage(url)
            if f:
                self.safeclose(f)
            return None
        text, nurl = self.readhtml(url)
        if nurl != url:
            if self.verbose > 1:
                print " Redirected to", nurl
            url = nurl
        if text:
            return Page(text, url, verbose=self.verbose, maxpage=self.maxpage)

    def readhtml(self, url):
        text = None
        f, url = self.openhtml(url)
        if f:
            text = f.read()
            f.close()
        return text, url

    def openhtml(self, url):
        f = self.openpage(url)
        if f:
            url = f.geturl()
            info = f.info()
            if not self.checkforhtml(info, url):
                self.safeclose(f)
                f = None
        return f, url

    def openpage(self, url):
        try:
            return self.urlopener.open(url)
        except IOError, msg:
            msg = self.sanitize(msg)
            if self.verbose > 0:
                print "Error ", msg
            if self.verbose > 0:
                self.show(" HREF ", url, "  from", self.todo[url])
            self.setbad(url, msg)
            return None

    def checkforhtml(self, info, url):
        if info.has_key('content-type'):
            ctype = string.lower(info['content-type'])
        else:
            if url[-1:] == "/":
                return 1
            ctype, encoding = mimetypes.guess_type(url)
        if ctype == 'text/html':
            return 1
        else:
            if self.verbose > 1:
                print " Not HTML, mime type", ctype
            return 0

    def setgood(self, url):
        if self.bad.has_key(url):
            del self.bad[url]
            self.changed = 1
            if self.verbose > 0:
                print "(Clear previously seen error)"

    def setbad(self, url, msg):
        if self.bad.has_key(url) and self.bad[url] == msg:
            if self.verbose > 0:
                print "(Seen this error before)"
            return
        self.bad[url] = msg
        self.changed = 1
        self.markerror(url)
        
    def markerror(self, url):
        try:
            origins = self.todo[url]
        except KeyError:
            origins = self.done[url]
        for source, rawlink in origins:
            triple = url, rawlink, self.bad[url]
            self.seterror(source, triple)

    def seterror(self, url, triple):
        try:
            self.errors[url].append(triple)
        except KeyError:
            self.errors[url] = [triple]

    # The following used to be toplevel functions; they have been
    # changed into methods so they can be overridden in subclasses.

    def show(self, p1, link, p2, origins):
        print p1, link
        i = 0
        for source, rawlink in origins:
            i = i+1
            if i == 2:
                p2 = ' '*len(p2)
            print p2, source,
            if rawlink != link: print "(%s)" % rawlink,
            print

    def sanitize(self, msg):
        if isinstance(IOError, ClassType) and isinstance(msg, IOError):
            # Do the other branch recursively
            msg.args = self.sanitize(msg.args)
        elif isinstance(msg, TupleType):
            if len(msg) >= 4 and msg[0] == 'http error' and \
               isinstance(msg[3], InstanceType):
                # Remove the Message instance -- it may contain
                # a file object which prevents pickling.
                msg = msg[:3] + msg[4:]
        return msg

    def safeclose(self, f):
        try:
            url = f.geturl()
        except AttributeError:
            pass
        else:
            if url[:4] == 'ftp:' or url[:7] == 'file://':
                # Apparently ftp connections don't like to be closed
                # prematurely...
                text = f.read()
        f.close()

    def save_pickle(self, dumpfile=DUMPFILE):
        if not self.changed:
            if self.verbose > 0:
                print
                print "No need to save checkpoint"
        elif not dumpfile:
            if self.verbose > 0:
                print "No dumpfile, won't save checkpoint"
        else:
            if self.verbose > 0:
                print
                print "Saving checkpoint to %s ..." % dumpfile
            newfile = dumpfile + ".new"
            f = open(newfile, "wb")
            pickle.dump(self, f)
            f.close()
            try:
                os.unlink(dumpfile)
            except os.error:
                pass
            os.rename(newfile, dumpfile)
            if self.verbose > 0:
                print "Done."
            return 1


class Page:

    def __init__(self, text, url, verbose=VERBOSE, maxpage=MAXPAGE):
        self.text = text
        self.url = url
        self.verbose = verbose
        self.maxpage = maxpage

    def getlinkinfos(self):
        size = len(self.text)
        if size > self.maxpage:
            if self.verbose > 0:
                print "Skip huge file", self.url
                print "  (%.0f Kbytes)" % (size*0.001)
            return []
        if self.verbose > 2:
            print "  Parsing", self.url, "(%d bytes)" % size
        parser = MyHTMLParser(verbose=self.verbose)
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

    def __init__(*args):
        self = args[0]
        apply(urllib.FancyURLopener.__init__, args)
        self.addheaders = [
            ('User-agent', 'Python-webchecker/%s' % __version__),
            ]

    def http_error_401(self, url, fp, errcode, errmsg, headers):
        return None

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


class MyHTMLParser(sgmllib.SGMLParser):

    def __init__(self, verbose=VERBOSE):
        self.base = None
        self.links = {}
        self.myverbose = verbose
        sgmllib.SGMLParser.__init__(self)

    def start_a(self, attributes):
        self.link_attr(attributes, 'href')

    def end_a(self): pass

    def do_area(self, attributes):
        self.link_attr(attributes, 'href')

    def do_img(self, attributes):
        self.link_attr(attributes, 'src', 'lowsrc')

    def do_frame(self, attributes):
        self.link_attr(attributes, 'src')

    def link_attr(self, attributes, *args):
        for name, value in attributes:
            if name in args:
                if value: value = string.strip(value)
                if value: self.links[value] = None

    def do_base(self, attributes):
        for name, value in attributes:
            if name == 'href':
                if value: value = string.strip(value)
                if value:
                    if self.myverbose > 1:
                        print "  Base", value
                    self.base = value

    def getlinks(self):
        return self.links.keys()

    def getbase(self):
        return self.base


if __name__ == '__main__':
    main()
