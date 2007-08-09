#! /usr/bin/env python

# Original code by Guido van Rossum; extensive changes by Sam Bayer,
# including code to check URL fragments.

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

- We follow links indicated by <A>, <FRAME> and <IMG> tags.  We also
honor the <BASE> tag.

- We now check internal NAME anchor links, as well as toplevel links.

- Checking external links is now done by default; use -x to *disable*
this feature.  External links are now checked during normal
processing.  (XXX The status of a checked link could be categorized
better.  Later...)

- If external links are not checked, you can use the -t flag to
provide specific overrides to -x.

Usage: webchecker.py [option] ... [rooturl] ...

Options:

-R        -- restart from checkpoint file
-d file   -- checkpoint filename (default %(DUMPFILE)s)
-m bytes  -- skip HTML pages larger than this size (default %(MAXPAGE)d)
-n        -- reports only, no checking (use with -R)
-q        -- quiet operation (also suppresses external links report)
-r number -- number of links processed per round (default %(ROUNDSIZE)d)
-t root   -- specify root dir which should be treated as internal (can repeat)
-v        -- verbose operation; repeating -v will increase verbosity
-x        -- don't check external links (these are often slow to check)
-a        -- don't check name anchors

Arguments:

rooturl   -- URL to start checking
             (default %(DEFROOT)s)

"""


__version__ = "$Revision$"


import sys
import os
from types import *
import io
import getopt
import pickle

import urllib
import urlparse
import sgmllib
import cgi

import mimetypes
import robotparser

# Extract real version number if necessary
if __version__[0] == '$':
    _v = __version__.split()
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
NONAMES = 0                             # Force name anchor checking


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
        opts, args = getopt.getopt(sys.argv[1:], 'Rd:m:nqr:t:vxa')
    except getopt.error as msg:
        sys.stdout = sys.stderr
        print(msg)
        print(__doc__%globals())
        sys.exit(2)

    # The extra_roots variable collects extra roots.
    extra_roots = []
    nonames = NONAMES

    for o, a in opts:
        if o == '-R':
            restart = 1
        if o == '-d':
            dumpfile = a
        if o == '-m':
            maxpage = int(a)
        if o == '-n':
            norun = 1
        if o == '-q':
            verbose = 0
        if o == '-r':
            roundsize = int(a)
        if o == '-t':
            extra_roots.append(a)
        if o == '-a':
            nonames = not nonames
        if o == '-v':
            verbose = verbose + 1
        if o == '-x':
            checkext = not checkext

    if verbose > 0:
        print(AGENTNAME, "version", __version__)

    if restart:
        c = load_pickle(dumpfile=dumpfile, verbose=verbose)
    else:
        c = Checker()

    c.setflags(checkext=checkext, verbose=verbose,
               maxpage=maxpage, roundsize=roundsize,
               nonames=nonames
               )

    if not restart and not args:
        args.append(DEFROOT)

    for arg in args:
        c.addroot(arg)

    # The -t flag is only needed if external links are not to be
    # checked. So -t values are ignored unless -x was specified.
    if not checkext:
        for root in extra_roots:
            # Make sure it's terminated by a slash,
            # so that addroot doesn't discard the last
            # directory component.
            if root[-1] != "/":
                root = root + "/"
            c.addroot(root, add_to_do = 0)

    try:

        if not norun:
            try:
                c.run()
            except KeyboardInterrupt:
                if verbose > 0:
                    print("[run interrupted]")

        try:
            c.report()
        except KeyboardInterrupt:
            if verbose > 0:
                print("[report interrupted]")

    finally:
        if c.save_pickle(dumpfile):
            if dumpfile == DUMPFILE:
                print("Use ``%s -R'' to restart." % sys.argv[0])
            else:
                print("Use ``%s -R -d %s'' to restart." % (sys.argv[0],
                                                           dumpfile))


def load_pickle(dumpfile=DUMPFILE, verbose=VERBOSE):
    if verbose > 0:
        print("Loading checkpoint from %s ..." % dumpfile)
    f = open(dumpfile, "rb")
    c = pickle.load(f)
    f.close()
    if verbose > 0:
        print("Done.")
        print("Root:", "\n      ".join(c.roots))
    return c


class Checker:

    checkext = CHECKEXT
    verbose = VERBOSE
    maxpage = MAXPAGE
    roundsize = ROUNDSIZE
    nonames = NONAMES

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

        # Add a name table, so that the name URLs can be checked. Also
        # serves as an implicit cache for which URLs are done.
        self.name_table = {}

        self.round = 0
        # The following are not pickled:
        self.robots = {}
        self.errors = {}
        self.urlopener = MyURLopener()
        self.changed = 0

    def note(self, level, format, *args):
        if self.verbose > level:
            if args:
                format = format%args
            self.message(format)

    def message(self, format, *args):
        if args:
            format = format%args
        print(format)

    def __getstate__(self):
        return (self.roots, self.todo, self.done, self.bad, self.round)

    def __setstate__(self, state):
        self.reset()
        (self.roots, self.todo, self.done, self.bad, self.round) = state
        for root in self.roots:
            self.addrobot(root)
        for url in self.bad.keys():
            self.markerror(url)

    def addroot(self, root, add_to_do = 1):
        if root not in self.roots:
            troot = root
            scheme, netloc, path, params, query, fragment = \
                    urlparse.urlparse(root)
            i = path.rfind("/") + 1
            if 0 < i < len(path):
                path = path[:i]
                troot = urlparse.urlunparse((scheme, netloc, path,
                                             params, query, fragment))
            self.roots.append(troot)
            self.addrobot(root)
            if add_to_do:
                self.newlink((root, ""), ("<root>", root))

    def addrobot(self, root):
        root = urlparse.urljoin(root, "/")
        if self.robots.has_key(root): return
        url = urlparse.urljoin(root, "/robots.txt")
        self.robots[root] = rp = robotparser.RobotFileParser()
        self.note(2, "Parsing %s", url)
        rp.debug = self.verbose > 3
        rp.set_url(url)
        try:
            rp.read()
        except (OSError, IOError) as msg:
            self.note(1, "I/O error parsing %s: %s", url, msg)

    def run(self):
        while self.todo:
            self.round = self.round + 1
            self.note(0, "\nRound %d (%s)\n", self.round, self.status())
            urls = self.todo.keys()
            urls.sort()
            del urls[self.roundsize:]
            for url in urls:
                self.dopage(url)

    def status(self):
        return "%d total, %d to do, %d done, %d bad" % (
            len(self.todo)+len(self.done),
            len(self.todo), len(self.done),
            len(self.bad))

    def report(self):
        self.message("")
        if not self.todo: s = "Final"
        else: s = "Interim"
        self.message("%s Report (%s)", s, self.status())
        self.report_errors()

    def report_errors(self):
        if not self.bad:
            self.message("\nNo errors")
            return
        self.message("\nError Report:")
        sources = self.errors.keys()
        sources.sort()
        for source in sources:
            triples = self.errors[source]
            self.message("")
            if len(triples) > 1:
                self.message("%d Errors in %s", len(triples), source)
            else:
                self.message("Error in %s", source)
            # Call self.format_url() instead of referring
            # to the URL directly, since the URLs in these
            # triples is now a (URL, fragment) pair. The value
            # of the "source" variable comes from the list of
            # origins, and is a URL, not a pair.
            for url, rawlink, msg in triples:
                if rawlink != self.format_url(url): s = " (%s)" % rawlink
                else: s = ""
                self.message("  HREF %s%s\n    msg %s",
                             self.format_url(url), s, msg)

    def dopage(self, url_pair):

        # All printing of URLs uses format_url(); argument changed to
        # url_pair for clarity.
        if self.verbose > 1:
            if self.verbose > 2:
                self.show("Check ", self.format_url(url_pair),
                          "  from", self.todo[url_pair])
            else:
                self.message("Check %s", self.format_url(url_pair))
        url, local_fragment = url_pair
        if local_fragment and self.nonames:
            self.markdone(url_pair)
            return
        try:
            page = self.getpage(url_pair)
        except sgmllib.SGMLParseError as msg:
            msg = self.sanitize(msg)
            self.note(0, "Error parsing %s: %s",
                          self.format_url(url_pair), msg)
            # Dont actually mark the URL as bad - it exists, just
            # we can't parse it!
            page = None
        if page:
            # Store the page which corresponds to this URL.
            self.name_table[url] = page
            # If there is a fragment in this url_pair, and it's not
            # in the list of names for the page, call setbad(), since
            # it's a missing anchor.
            if local_fragment and local_fragment not in page.getnames():
                self.setbad(url_pair, ("Missing name anchor `%s'" % local_fragment))
            for info in page.getlinkinfos():
                # getlinkinfos() now returns the fragment as well,
                # and we store that fragment here in the "todo" dictionary.
                link, rawlink, fragment = info
                # However, we don't want the fragment as the origin, since
                # the origin is logically a page.
                origin = url, rawlink
                self.newlink((link, fragment), origin)
        else:
            # If no page has been created yet, we want to
            # record that fact.
            self.name_table[url_pair[0]] = None
        self.markdone(url_pair)

    def newlink(self, url, origin):
        if self.done.has_key(url):
            self.newdonelink(url, origin)
        else:
            self.newtodolink(url, origin)

    def newdonelink(self, url, origin):
        if origin not in self.done[url]:
            self.done[url].append(origin)

        # Call self.format_url(), since the URL here
        # is now a (URL, fragment) pair.
        self.note(3, "  Done link %s", self.format_url(url))

        # Make sure that if it's bad, that the origin gets added.
        if self.bad.has_key(url):
            source, rawlink = origin
            triple = url, rawlink, self.bad[url]
            self.seterror(source, triple)

    def newtodolink(self, url, origin):
        # Call self.format_url(), since the URL here
        # is now a (URL, fragment) pair.
        if self.todo.has_key(url):
            if origin not in self.todo[url]:
                self.todo[url].append(origin)
            self.note(3, "  Seen todo link %s", self.format_url(url))
        else:
            self.todo[url] = [origin]
            self.note(3, "  New todo link %s", self.format_url(url))

    def format_url(self, url):
        link, fragment = url
        if fragment: return link + "#" + fragment
        else: return link

    def markdone(self, url):
        self.done[url] = self.todo[url]
        del self.todo[url]
        self.changed = 1

    def inroots(self, url):
        for root in self.roots:
            if url[:len(root)] == root:
                return self.isallowed(root, url)
        return 0

    def isallowed(self, root, url):
        root = urlparse.urljoin(root, "/")
        return self.robots[root].can_fetch(AGENTNAME, url)

    def getpage(self, url_pair):
        # Incoming argument name is a (URL, fragment) pair.
        # The page may have been cached in the name_table variable.
        url, fragment = url_pair
        if self.name_table.has_key(url):
            return self.name_table[url]

        scheme, path = urllib.splittype(url)
        if scheme in ('mailto', 'news', 'javascript', 'telnet'):
            self.note(1, " Not checking %s URL" % scheme)
            return None
        isint = self.inroots(url)

        # Ensure that openpage gets the URL pair to
        # print out its error message and record the error pair
        # correctly.
        if not isint:
            if not self.checkext:
                self.note(1, " Not checking ext link")
                return None
            f = self.openpage(url_pair)
            if f:
                self.safeclose(f)
            return None
        text, nurl = self.readhtml(url_pair)

        if nurl != url:
            self.note(1, " Redirected to %s", nurl)
            url = nurl
        if text:
            return Page(text, url, maxpage=self.maxpage, checker=self)

    # These next three functions take (URL, fragment) pairs as
    # arguments, so that openpage() receives the appropriate tuple to
    # record error messages.
    def readhtml(self, url_pair):
        url, fragment = url_pair
        text = None
        f, url = self.openhtml(url_pair)
        if f:
            text = f.read()
            f.close()
        return text, url

    def openhtml(self, url_pair):
        url, fragment = url_pair
        f = self.openpage(url_pair)
        if f:
            url = f.geturl()
            info = f.info()
            if not self.checkforhtml(info, url):
                self.safeclose(f)
                f = None
        return f, url

    def openpage(self, url_pair):
        url, fragment = url_pair
        try:
            return self.urlopener.open(url)
        except (OSError, IOError) as msg:
            msg = self.sanitize(msg)
            self.note(0, "Error %s", msg)
            if self.verbose > 0:
                self.show(" HREF ", url, "  from", self.todo[url_pair])
            self.setbad(url_pair, msg)
            return None

    def checkforhtml(self, info, url):
        if info.has_key('content-type'):
            ctype = cgi.parse_header(info['content-type'])[0].lower()
            if ';' in ctype:
                # handle content-type: text/html; charset=iso8859-1 :
                ctype = ctype.split(';', 1)[0].strip()
        else:
            if url[-1:] == "/":
                return 1
            ctype, encoding = mimetypes.guess_type(url)
        if ctype == 'text/html':
            return 1
        else:
            self.note(1, " Not HTML, mime type %s", ctype)
            return 0

    def setgood(self, url):
        if self.bad.has_key(url):
            del self.bad[url]
            self.changed = 1
            self.note(0, "(Clear previously seen error)")

    def setbad(self, url, msg):
        if self.bad.has_key(url) and self.bad[url] == msg:
            self.note(0, "(Seen this error before)")
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
            # Because of the way the URLs are now processed, I need to
            # check to make sure the URL hasn't been entered in the
            # error list.  The first element of the triple here is a
            # (URL, fragment) pair, but the URL key is not, since it's
            # from the list of origins.
            if triple not in self.errors[url]:
                self.errors[url].append(triple)
        except KeyError:
            self.errors[url] = [triple]

    # The following used to be toplevel functions; they have been
    # changed into methods so they can be overridden in subclasses.

    def show(self, p1, link, p2, origins):
        self.message("%s %s", p1, link)
        i = 0
        for source, rawlink in origins:
            i = i+1
            if i == 2:
                p2 = ' '*len(p2)
            if rawlink != link: s = " (%s)" % rawlink
            else: s = ""
            self.message("%s %s%s", p2, source, s)

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
            self.note(0, "\nNo need to save checkpoint")
        elif not dumpfile:
            self.note(0, "No dumpfile, won't save checkpoint")
        else:
            self.note(0, "\nSaving checkpoint to %s ...", dumpfile)
            newfile = dumpfile + ".new"
            f = open(newfile, "wb")
            pickle.dump(self, f)
            f.close()
            try:
                os.unlink(dumpfile)
            except os.error:
                pass
            os.rename(newfile, dumpfile)
            self.note(0, "Done.")
            return 1


class Page:

    def __init__(self, text, url, verbose=VERBOSE, maxpage=MAXPAGE, checker=None):
        self.text = text
        self.url = url
        self.verbose = verbose
        self.maxpage = maxpage
        self.checker = checker

        # The parsing of the page is done in the __init__() routine in
        # order to initialize the list of names the file
        # contains. Stored the parser in an instance variable. Passed
        # the URL to MyHTMLParser().
        size = len(self.text)
        if size > self.maxpage:
            self.note(0, "Skip huge file %s (%.0f Kbytes)", self.url, (size*0.001))
            self.parser = None
            return
        self.checker.note(2, "  Parsing %s (%d bytes)", self.url, size)
        self.parser = MyHTMLParser(url, verbose=self.verbose,
                                   checker=self.checker)
        self.parser.feed(self.text)
        self.parser.close()

    def note(self, level, msg, *args):
        if self.checker:
            self.checker.note(level, msg, *args)
        else:
            if self.verbose >= level:
                if args:
                    msg = msg%args
                print(msg)

    # Method to retrieve names.
    def getnames(self):
        if self.parser:
            return self.parser.names
        else:
            return []

    def getlinkinfos(self):
        # File reading is done in __init__() routine.  Store parser in
        # local variable to indicate success of parsing.

        # If no parser was stored, fail.
        if not self.parser: return []

        rawlinks = self.parser.getlinks()
        base = urlparse.urljoin(self.url, self.parser.getbase() or "")
        infos = []
        for rawlink in rawlinks:
            t = urlparse.urlparse(rawlink)
            # DON'T DISCARD THE FRAGMENT! Instead, include
            # it in the tuples which are returned. See Checker.dopage().
            fragment = t[-1]
            t = t[:-1] + ('',)
            rawlink = urlparse.urlunparse(t)
            link = urlparse.urljoin(base, rawlink)
            infos.append((link, rawlink, fragment))

        return infos


class MyStringIO(io.StringIO):

    def __init__(self, url, info):
        self.__url = url
        self.__info = info
        super(MyStringIO, self).__init__(self)

    def info(self):
        return self.__info

    def geturl(self):
        return self.__url


class MyURLopener(urllib.FancyURLopener):

    http_error_default = urllib.URLopener.http_error_default

    def __init__(*args):
        self = args[0]
        urllib.FancyURLopener.__init__(*args)
        self.addheaders = [
            ('User-agent', 'Python-webchecker/%s' % __version__),
            ]

    def http_error_401(self, url, fp, errcode, errmsg, headers):
        return None

    def open_file(self, url):
        path = urllib.url2pathname(urllib.unquote(url))
        if os.path.isdir(path):
            if path[-1] != os.sep:
                url = url + '/'
            indexpath = os.path.join(path, "index.html")
            if os.path.exists(indexpath):
                return self.open_file(url + "index.html")
            try:
                names = os.listdir(path)
            except os.error as msg:
                exc_type, exc_value, exc_tb = sys.exc_info()
                raise IOError, msg, exc_tb
            names.sort()
            s = MyStringIO("file:"+url, {'content-type': 'text/html'})
            s.write('<BASE HREF="file:%s">\n' %
                    urllib.quote(os.path.join(path, "")))
            for name in names:
                q = urllib.quote(name)
                s.write('<A HREF="%s">%s</A>\n' % (q, q))
            s.seek(0)
            return s
        return urllib.FancyURLopener.open_file(self, url)


class MyHTMLParser(sgmllib.SGMLParser):

    def __init__(self, url, verbose=VERBOSE, checker=None):
        self.myverbose = verbose # now unused
        self.checker = checker
        self.base = None
        self.links = {}
        self.names = []
        self.url = url
        sgmllib.SGMLParser.__init__(self)

    def check_name_id(self, attributes):
        """ Check the name or id attributes on an element.
        """
        # We must rescue the NAME or id (name is deprecated in XHTML)
        # attributes from the anchor, in order to
        # cache the internal anchors which are made
        # available in the page.
        for name, value in attributes:
            if name == "name" or name == "id":
                if value in self.names:
                    self.checker.message("WARNING: duplicate ID name %s in %s",
                                         value, self.url)
                else: self.names.append(value)
                break

    def unknown_starttag(self, tag, attributes):
        """ In XHTML, you can have id attributes on any element.
        """
        self.check_name_id(attributes)

    def start_a(self, attributes):
        self.link_attr(attributes, 'href')
        self.check_name_id(attributes)

    def end_a(self): pass

    def do_area(self, attributes):
        self.link_attr(attributes, 'href')
        self.check_name_id(attributes)

    def do_body(self, attributes):
        self.link_attr(attributes, 'background', 'bgsound')
        self.check_name_id(attributes)

    def do_img(self, attributes):
        self.link_attr(attributes, 'src', 'lowsrc')
        self.check_name_id(attributes)

    def do_frame(self, attributes):
        self.link_attr(attributes, 'src', 'longdesc')
        self.check_name_id(attributes)

    def do_iframe(self, attributes):
        self.link_attr(attributes, 'src', 'longdesc')
        self.check_name_id(attributes)

    def do_link(self, attributes):
        for name, value in attributes:
            if name == "rel":
                parts = value.lower().split()
                if (  parts == ["stylesheet"]
                      or parts == ["alternate", "stylesheet"]):
                    self.link_attr(attributes, "href")
                    break
        self.check_name_id(attributes)

    def do_object(self, attributes):
        self.link_attr(attributes, 'data', 'usemap')
        self.check_name_id(attributes)

    def do_script(self, attributes):
        self.link_attr(attributes, 'src')
        self.check_name_id(attributes)

    def do_table(self, attributes):
        self.link_attr(attributes, 'background')
        self.check_name_id(attributes)

    def do_td(self, attributes):
        self.link_attr(attributes, 'background')
        self.check_name_id(attributes)

    def do_th(self, attributes):
        self.link_attr(attributes, 'background')
        self.check_name_id(attributes)

    def do_tr(self, attributes):
        self.link_attr(attributes, 'background')
        self.check_name_id(attributes)

    def link_attr(self, attributes, *args):
        for name, value in attributes:
            if name in args:
                if value: value = value.strip()
                if value: self.links[value] = None

    def do_base(self, attributes):
        for name, value in attributes:
            if name == 'href':
                if value: value = value.strip()
                if value:
                    if self.checker:
                        self.checker.note(1, "  Base %s", value)
                    self.base = value
        self.check_name_id(attributes)

    def getlinks(self):
        return self.links.keys()

    def getbase(self):
        return self.base


if __name__ == '__main__':
    main()
