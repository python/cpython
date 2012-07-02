#! /usr/bin/env python

"""Show file statistics by extension."""

from __future__ import print_function

import os
import sys

class Stats:

    def __init__(self):
        self.stats = {}

    def statargs(self, args):
        for arg in args:
            if os.path.isdir(arg):
                self.statdir(arg)
            elif os.path.isfile(arg):
                self.statfile(arg)
            else:
                sys.stderr.write("Can't find %s\n" % arg)
                self.addstats("<???>", "unknown", 1)

    def statdir(self, dir):
        self.addstats("<dir>", "dirs", 1)
        try:
            names = sorted(os.listdir(dir))
        except os.error as err:
            sys.stderr.write("Can't list %s: %s\n" % (dir, err))
            self.addstats("<dir>", "unlistable", 1)
            return
        for name in names:
            if name.startswith(".#"):
                continue # Skip CVS temp files
            if name.endswith("~"):
                continue# Skip Emacs backup files
            full = os.path.join(dir, name)
            if os.path.islink(full):
                self.addstats("<lnk>", "links", 1)
            elif os.path.isdir(full):
                self.statdir(full)
            else:
                self.statfile(full)

    def statfile(self, filename):
        head, ext = os.path.splitext(filename)
        head, base = os.path.split(filename)
        if ext == base:
            ext = "" # E.g. .cvsignore is deemed not to have an extension
        ext = os.path.normcase(ext)
        if not ext:
            ext = "<none>"
        self.addstats(ext, "files", 1)
        try:
            f = open(filename, "rb")
        except IOError as err:
            sys.stderr.write("Can't open %s: %s\n" % (filename, err))
            self.addstats(ext, "unopenable", 1)
            return
        data = f.read()
        f.close()
        self.addstats(ext, "bytes", len(data))
        if b'\0' in data:
            self.addstats(ext, "binary", 1)
            return
        if not data:
            self.addstats(ext, "empty", 1)
        #self.addstats(ext, "chars", len(data))
        lines = data.splitlines()
        self.addstats(ext, "lines", len(lines))
        del lines
        words = data.split()
        self.addstats(ext, "words", len(words))

    def addstats(self, ext, key, n):
        d = self.stats.setdefault(ext, {})
        d[key] = d.get(key, 0) + n

    def report(self):
        exts = sorted(self.stats.keys())
        # Get the column keys
        columns = {}
        for ext in exts:
            columns.update(self.stats[ext])
        cols = sorted(columns.keys())
        colwidth = {}
        colwidth["ext"] = max([len(ext) for ext in exts])
        minwidth = 6
        self.stats["TOTAL"] = {}
        for col in cols:
            total = 0
            cw = max(minwidth, len(col))
            for ext in exts:
                value = self.stats[ext].get(col)
                if value is None:
                    w = 0
                else:
                    w = len("%d" % value)
                    total += value
                cw = max(cw, w)
            cw = max(cw, len(str(total)))
            colwidth[col] = cw
            self.stats["TOTAL"][col] = total
        exts.append("TOTAL")
        for ext in exts:
            self.stats[ext]["ext"] = ext
        cols.insert(0, "ext")
        def printheader():
            for col in cols:
                print("%*s" % (colwidth[col], col), end=" ")
            print()
        printheader()
        for ext in exts:
            for col in cols:
                value = self.stats[ext].get(col, "")
                print("%*s" % (colwidth[col], value), end=" ")
            print()
        printheader() # Another header at the bottom

def main():
    args = sys.argv[1:]
    if not args:
        args = [os.curdir]
    s = Stats()
    s.statargs(args)
    s.report()

if __name__ == "__main__":
    main()
