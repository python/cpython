#! /usr/bin/env python

"""A variant on webchecker that creates a mirror copy of a remote site."""

__version__ = "$Revision$"

import os
import sys
import string
import urllib
import getopt

import webchecker

# Extract real version number if necessary
if __version__[0] == '$':
    _v = string.split(__version__)
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

    def readhtml(self, url):
        text = None
        path = self.savefilename(url)
        try:
            f = open(path, "rb")
        except IOError:
            f = self.openpage(url)
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
