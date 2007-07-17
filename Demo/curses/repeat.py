#! /usr/bin/env python

"""repeat <shell-command>

This simple program repeatedly (at 1-second intervals) executes the
shell command given on the command line and displays the output (or as
much of it as fits on the screen).  It uses curses to paint each new
output on top of the old output, so that if nothing changes, the
screen doesn't change.  This is handy to watch for changes in e.g. a
directory or process listing.

To end, hit Control-C.
"""

# Author: Guido van Rossum

# Disclaimer: there's a Linux program named 'watch' that does the same
# thing.  Honestly, I didn't know of its existence when I wrote this!

# To do: add features until it has the same functionality as watch(1);
# then compare code size and development time.

import os
import sys
import time
import curses

def main():
    if not sys.argv[1:]:
        print(__doc__)
        sys.exit(0)
    cmd = " ".join(sys.argv[1:])
    p = os.popen(cmd, "r")
    text = p.read()
    sts = p.close()
    if sts:
        print("Exit code:", sts, file=sys.stderr)
        sys.exit(sts)
    w = curses.initscr()
    try:
        while True:
            w.erase()
            try:
                w.addstr(text)
            except curses.error:
                pass
            w.refresh()
            time.sleep(1)
            p = os.popen(cmd, "r")
            text = p.read()
            sts = p.close()
            if sts:
                print("Exit code:", sts, file=sys.stderr)
                sys.exit(sts)
    finally:
        curses.endwin()

main()
