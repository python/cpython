"""A class to build directory diff tools on."""

import os

import dircache
import cmpcache
import statcache
from stat import *

class dircmp:
    """Directory comparison class."""

    def new(self, a, b):
        """Initialize."""
        self.a = a
        self.b = b
        # Properties that caller may change before calling self.run():
        self.hide = [os.curdir, os.pardir] # Names never to be shown
        self.ignore = ['RCS', 'tags'] # Names ignored in comparison

        return self

    def run(self):
        """Compare everything except common subdirectories."""
        self.a_list = filter(dircache.listdir(self.a), self.hide)
        self.b_list = filter(dircache.listdir(self.b), self.hide)
        self.a_list.sort()
        self.b_list.sort()
        self.phase1()
        self.phase2()
        self.phase3()

    def phase1(self):
        """Compute common names."""
        self.a_only = []
        self.common = []
        for x in self.a_list:
            if x in self.b_list:
                self.common.append(x)
            else:
                self.a_only.append(x)

        self.b_only = []
        for x in self.b_list:
            if x not in self.common:
                self.b_only.append(x)

    def phase2(self):
        """Distinguish files, directories, funnies."""
        self.common_dirs = []
        self.common_files = []
        self.common_funny = []

        for x in self.common:
            a_path = os.path.join(self.a, x)
            b_path = os.path.join(self.b, x)

            ok = 1
            try:
                a_stat = statcache.stat(a_path)
            except os.error, why:
                # print 'Can\'t stat', a_path, ':', why[1]
                ok = 0
            try:
                b_stat = statcache.stat(b_path)
            except os.error, why:
                # print 'Can\'t stat', b_path, ':', why[1]
                ok = 0

            if ok:
                a_type = S_IFMT(a_stat[ST_MODE])
                b_type = S_IFMT(b_stat[ST_MODE])
                if a_type != b_type:
                    self.common_funny.append(x)
                elif S_ISDIR(a_type):
                    self.common_dirs.append(x)
                elif S_ISREG(a_type):
                    self.common_files.append(x)
                else:
                    self.common_funny.append(x)
            else:
                self.common_funny.append(x)

    def phase3(self):
        """Find out differences between common files."""
        xx = cmpfiles(self.a, self.b, self.common_files)
        self.same_files, self.diff_files, self.funny_files = xx

    def phase4(self):
        """Find out differences between common subdirectories.
        A new dircmp object is created for each common subdirectory,
        these are stored in a dictionary indexed by filename.
        The hide and ignore properties are inherited from the parent."""
        self.subdirs = {}
        for x in self.common_dirs:
            a_x = os.path.join(self.a, x)
            b_x = os.path.join(self.b, x)
            self.subdirs[x] = newdd = dircmp().new(a_x, b_x)
            newdd.hide = self.hide
            newdd.ignore = self.ignore
            newdd.run()

    def phase4_closure(self):
        """Recursively call phase4() on subdirectories."""
        self.phase4()
        for x in self.subdirs.keys():
            self.subdirs[x].phase4_closure()

    def report(self):
        """Print a report on the differences between a and b."""
        # Assume that phases 1 to 3 have been executed
        # Output format is purposely lousy
        print 'diff', self.a, self.b
        if self.a_only:
            print 'Only in', self.a, ':', self.a_only
        if self.b_only:
            print 'Only in', self.b, ':', self.b_only
        if self.same_files:
            print 'Identical files :', self.same_files
        if self.diff_files:
            print 'Differing files :', self.diff_files
        if self.funny_files:
            print 'Trouble with common files :', self.funny_files
        if self.common_dirs:
            print 'Common subdirectories :', self.common_dirs
        if self.common_funny:
            print 'Common funny cases :', self.common_funny

    def report_closure(self):
        """Print reports on self and on subdirs.
        If phase 4 hasn't been done, no subdir reports are printed."""
        self.report()
        try:
            x = self.subdirs
        except AttributeError:
            return # No subdirectories computed
        for x in self.subdirs.keys():
            print
            self.subdirs[x].report_closure()

    def report_phase4_closure(self):
        """Report and do phase 4 recursively."""
        self.report()
        self.phase4()
        for x in self.subdirs.keys():
            print
            self.subdirs[x].report_phase4_closure()


def cmpfiles(a, b, common):
    """Compare common files in two directories.
    Return:
        - files that compare equal
        - files that compare different
        - funny cases (can't stat etc.)"""

    res = ([], [], [])
    for x in common:
        res[cmp(os.path.join(a, x), os.path.join(b, x))].append(x)
    return res


def cmp(a, b):
    """Compare two files.
    Return:
        0 for equal
        1 for different
        2 for funny cases (can't stat, etc.)"""

    try:
        if cmpcache.cmp(a, b): return 0
        return 1
    except os.error:
        return 2


def filter(list, skip):
    """Return a copy with items that occur in skip removed."""

    result = []
    for item in list:
        if item not in skip: result.append(item)
    return result


def demo():
    """Demonstration and testing."""

    import sys
    import getopt
    options, args = getopt.getopt(sys.argv[1:], 'r')
    if len(args) != 2:
        raise getopt.error, 'need exactly two args'
    dd = dircmp().new(args[0], args[1])
    dd.run()
    if ('-r', '') in options:
        dd.report_phase4_closure()
    else:
        dd.report()

if __name__ == "__main__":
    demo()
