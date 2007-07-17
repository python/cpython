#! /usr/bin/env python

"""RCS Proxy.

Provide a simplified interface on RCS files, locally or remotely.
The functionality is geared towards implementing some sort of
remote CVS like utility.  It is modeled after the similar module
FSProxy.

The module defines two classes:

RCSProxyLocal  -- used for local access
RCSProxyServer -- used on the server side of remote access

The corresponding client class, RCSProxyClient, is defined in module
rcsclient.

The remote classes are instantiated with an IP address and an optional
verbosity flag.
"""

import server
import md5
import os
import fnmatch
import string
import tempfile
import rcslib


class DirSupport:

    def __init__(self):
        self._dirstack = []

    def __del__(self):
        self._close()

    def _close(self):
        while self._dirstack:
            self.back()

    def pwd(self):
        return os.getcwd()

    def cd(self, name):
        save = os.getcwd()
        os.chdir(name)
        self._dirstack.append(save)

    def back(self):
        if not self._dirstack:
            raise os.error("empty directory stack")
        dir = self._dirstack[-1]
        os.chdir(dir)
        del self._dirstack[-1]

    def listsubdirs(self, pat = None):
        files = os.listdir(os.curdir)
        files = list(filter(os.path.isdir, files))
        return self._filter(files, pat)

    def isdir(self, name):
        return os.path.isdir(name)

    def mkdir(self, name):
        os.mkdir(name, 0o777)

    def rmdir(self, name):
        os.rmdir(name)


class RCSProxyLocal(rcslib.RCS, DirSupport):

    def __init__(self):
        rcslib.RCS.__init__(self)
        DirSupport.__init__(self)

    def __del__(self):
        DirSupport.__del__(self)
        rcslib.RCS.__del__(self)

    def sumlist(self, list = None):
        return self._list(self.sum, list)

    def sumdict(self, list = None):
        return self._dict(self.sum, list)

    def sum(self, name_rev):
        f = self._open(name_rev)
        BUFFERSIZE = 1024*8
        sum = md5.new()
        while 1:
            buffer = f.read(BUFFERSIZE)
            if not buffer:
                break
            sum.update(buffer)
        self._closepipe(f)
        return sum.digest()

    def get(self, name_rev):
        f = self._open(name_rev)
        data = f.read()
        self._closepipe(f)
        return data

    def put(self, name_rev, data, message=None):
        name, rev = self._unmangle(name_rev)
        f = open(name, 'w')
        f.write(data)
        f.close()
        self.checkin(name_rev, message)
        self._remove(name)

    def _list(self, function, list = None):
        """INTERNAL: apply FUNCTION to all files in LIST.

        Return a list of the results.

        The list defaults to all files in the directory if None.

        """
        if list is None:
            list = self.listfiles()
        res = []
        for name in list:
            try:
                res.append((name, function(name)))
            except (os.error, IOError):
                res.append((name, None))
        return res

    def _dict(self, function, list = None):
        """INTERNAL: apply FUNCTION to all files in LIST.

        Return a dictionary mapping files to results.

        The list defaults to all files in the directory if None.

        """
        if list is None:
            list = self.listfiles()
        dict = {}
        for name in list:
            try:
                dict[name] = function(name)
            except (os.error, IOError):
                pass
        return dict


class RCSProxyServer(RCSProxyLocal, server.SecureServer):

    def __init__(self, address, verbose = server.VERBOSE):
        RCSProxyLocal.__init__(self)
        server.SecureServer.__init__(self, address, verbose)

    def _close(self):
        server.SecureServer._close(self)
        RCSProxyLocal._close(self)

    def _serve(self):
        server.SecureServer._serve(self)
        # Retreat into start directory
        while self._dirstack: self.back()


def test_server():
    import string
    import sys
    if sys.argv[1:]:
        port = string.atoi(sys.argv[1])
    else:
        port = 4127
    proxy = RCSProxyServer(('', port))
    proxy._serverloop()


def test():
    import sys
    if not sys.argv[1:] or sys.argv[1] and sys.argv[1][0] in '0123456789':
        test_server()
        sys.exit(0)
    proxy = RCSProxyLocal()
    what = sys.argv[1]
    if hasattr(proxy, what):
        attr = getattr(proxy, what)
        if hasattr(attr, '__call__'):
            print(attr(*sys.argv[2:]))
        else:
            print(repr(attr))
    else:
        print("%s: no such attribute" % what)
        sys.exit(2)


if __name__ == '__main__':
    test()
