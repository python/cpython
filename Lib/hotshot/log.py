import _hotshot
import os.path
import parser
import symbol
import sys

from _hotshot import \
     WHAT_ENTER, \
     WHAT_EXIT, \
     WHAT_LINENO, \
     WHAT_DEFINE_FILE, \
     WHAT_ADD_INFO


__all__ = ["LogReader", "ENTER", "EXIT", "LINE"]


ENTER = WHAT_ENTER
EXIT  = WHAT_EXIT
LINE  = WHAT_LINENO


try:
    StopIteration
except NameError:
    StopIteration = IndexError


class LogReader:
    def __init__(self, logfn):
        # fileno -> filename
        self._filemap = {}
        # (fileno, lineno) -> filename, funcname
        self._funcmap = {}

        self._info = {}
        self._nextitem = _hotshot.logreader(logfn).next
        self._stack = []

    # Iteration support:
    # This adds an optional (& ignored) parameter to next() so that the
    # same bound method can be used as the __getitem__() method -- this
    # avoids using an additional method call which kills the performance.

    def next(self, index=0):
        try:
            what, tdelta, fileno, lineno = self._nextitem()
        except TypeError:
            # logreader().next() returns None at the end
            raise StopIteration()
        if what == WHAT_DEFINE_FILE:
            self._filemap[fileno] = tdelta
            return self.next()
        if what == WHAT_ADD_INFO:
            key = tdelta.lower()
            try:
                L = self._info[key]
            except KeyError:
                L = []
                self._info[key] = L
            L.append(lineno)
            if key == "current-directory":
                self.cwd = lineno
            return self.next()
        if what == WHAT_ENTER:
            t = self._decode_location(fileno, lineno)
            filename, funcname = t
            self._stack.append((filename, funcname, lineno))
        elif what == WHAT_EXIT:
            filename, funcname, lineno = self._stack.pop()
        else:
            filename, funcname, firstlineno = self._stack[-1]
        return what, (filename, lineno, funcname), tdelta

    if sys.version < "2.2":
        # Don't add this for newer Python versions; we only want iteration
        # support, not general sequence support.
        __getitem__ = next
    else:
        def __iter__(self):
            return self

    #
    #  helpers
    #

    def _decode_location(self, fileno, lineno):
        try:
            return self._funcmap[(fileno, lineno)]
        except KeyError:
            if self._loadfile(fileno):
                filename = funcname = None
            try:
                filename, funcname = self._funcmap[(fileno, lineno)]
            except KeyError:
                filename = self._filemap.get(fileno)
                funcname = None
                self._funcmap[(fileno, lineno)] = (filename, funcname)
        return filename, funcname

    def _loadfile(self, fileno):
        try:
            filename = self._filemap[fileno]
        except KeyError:
            print "Could not identify fileId", fileno
            return 1
        if filename is None:
            return 1
        absname = os.path.normcase(os.path.join(self.cwd, filename))

        try:
            fp = open(absname)
        except IOError:
            return
        st = parser.suite(fp.read())
        fp.close()

        # Scan the tree looking for def and lambda nodes, filling in
        # self._funcmap with all the available information.
        funcdef = symbol.funcdef
        lambdef = symbol.lambdef

        stack = [st.totuple(1)]

        while stack:
            tree = stack.pop()
            try:
                sym = tree[0]
            except (IndexError, TypeError):
                continue
            if sym == funcdef:
                self._funcmap[(fileno, tree[2][2])] = filename, tree[2][1]
            elif sym == lambdef:
                self._funcmap[(fileno, tree[1][2])] = filename, "<lambda>"
            stack.extend(list(tree[1:]))
