#! /usr/bin/env python

"""Show file statistics by extension."""

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
                sys.stderr.write("Can't find %s\n" % file)
                self.addstats("<???>", "unknown", 1)

    def statdir(self, dir):
        self.addstats("<dir>", "dirs", 1)
        try:
            names = os.listdir(dir)
        except os.error, err:
            sys.stderr.write("Can't list %s: %s\n" % (file, err))
            self.addstats(ext, "unlistable", 1)
            return
        names.sort()
        for name in names:
            full = os.path.join(dir, name)
            if os.path.islink(full):
                self.addstats("<lnk>", "links", 1)
            elif os.path.isdir(full):
                self.statdir(full)
            else:
                self.statfile(full)

    def statfile(self, file):
        head, ext = os.path.splitext(file)
        head, base = os.path.split(file)
        if ext == base:
            ext = "" # .cvsignore is deemed not to have an extension
        self.addstats(ext, "files", 1)
        try:
            f = open(file, "rb")
        except IOError, err:
            sys.stderr.write("Can't open %s: %s\n" % (file, err))
            self.addstats(ext, "unopenable", 1)
            return
        data = f.read()
        f.close()
        self.addstats(ext, "bytes", len(data))
        if '\0' in data:
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
        totals = {}
        exts = self.stats.keys()
        exts.sort()
        # Get the column keys
        columns = {}
        for ext in exts:
            columns.update(self.stats[ext])
        cols = columns.keys()
        cols.sort()
        minwidth = 7
        extwidth = max([len(ext) for ext in exts])
        print "%*s" % (extwidth, "ext"),
        for col in cols:
            width = max(len(col), minwidth)
            print "%*s" % (width, col),
        print
        for ext in exts:
            print "%*s" % (extwidth, ext),
            for col in cols:
                width = max(len(col), minwidth)
                value = self.stats[ext].get(col)
                if value is None:
                    s = ""
                else:
                    s = "%d" % value
                    totals[col] = totals.get(col, 0) + value
                print "%*s" % (width, s),
            print
        print "%*s" % (extwidth, "TOTAL"),
        for col in cols:
            width = max(len(col), minwidth)
            print "%*s" % (width, totals[col]),
        print

def main():
    args = sys.argv[1:]
    if not args:
        args = [os.curdir]
    s = Stats()
    s.statargs(args)
    s.report()

if __name__ == "__main__":
    main()
