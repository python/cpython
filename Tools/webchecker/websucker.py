#! /usr/bin/env python

"""A variant on webchecker that creates a mirror copy of a remote site."""

__version__ = "$Revision$"

import os
import sys
import urllib
import getopt

import webchecker

# Extract real version number if necessary
if __version__[0] == '$':
    _v = __version__.split()
    if len(_v) == 3:
        __version__ = _v[1]

def main():
    verbose = webchecker.VERBOSE
    try:
        opts, args = getopt.getopt(sys.argv[1:], "qv")
    except getopt.error, msg:
        print msg
        print "usage:", sys.argv[0], "[-qv] ... [rooturl] ..."
        return 2
    for o, a in opts:
        if o == "-q":
            verbose = 0
        if o == "-v":
            verbose = verbose + 1
    c = Sucker()
    c.setflags(verbose=verbose)
    c.urlopener.addheaders = [
            ('User-agent', 'websucker/%s' % __version__),
        ]
    for arg in args:
        print "Adding root", arg
        c.addroot(arg)
    print "Run..."
    c.run()

class Sucker(webchecker.Checker):

    checkext = 0
    nonames = 1

    # SAM 11/13/99: in general, URLs are now URL pairs.
    # Since we've suppressed name anchor checking,
    # we can ignore the second dimension.

    def readhtml(self, url_pair):
        url = url_pair[0]
        text = None
        path = self.savefilename(url)
        try:
            f = open(path, "rb")
        except IOError:
            f = self.openpage(url_pair)
            if f:
                info = f.info()
                nurl = f.geturl()
                if nurl != url:
                    url = nurl
                    path = self.savefilename(url)
                text = f.read()
                f.close()
                self.savefile(text, path)
                if not self.checkforhtml(info, url):
                    text = None
        else:
            if self.checkforhtml({}, url):
                text = f.read()
            f.close()
        return text, url

    def savefile(self, text, path):
        dir, base = os.path.split(path)
        makedirs(dir)
        try:
            f = open(path, "wb")
            f.write(text)
            f.close()
            self.message("saved %s", path)
        except IOError, msg:
            self.message("didn't save %s: %s", path, str(msg))

    def savefilename(self, url):
        type, rest = urllib.splittype(url)
        host, path = urllib.splithost(rest)
        path = path.lstrip("/")
        user, host = urllib.splituser(host)
        host, port = urllib.splitnport(host)
        host = host.lower()
        if not path or path[-1] == "/":
            path = path + "index.html"
        if os.sep != "/":
            path = os.sep.join(path.split("/"))
        path = os.path.join(host, path)
        return path

def makedirs(dir):
    if not dir:
        return
    if os.path.exists(dir):
        if not os.path.isdir(dir):
            try:
                os.rename(dir, dir + ".bak")
                os.mkdir(dir)
                os.rename(dir + ".bak", os.path.join(dir, "index.html"))
            except os.error:
                pass
        return
    head, tail = os.path.split(dir)
    if not tail:
        print "Huh?  Don't know how to make dir", dir
        return
    makedirs(head)
    os.mkdir(dir, 0777)

if __name__ == '__main__':
    sys.exit(main() or 0)
