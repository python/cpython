#! /usr/bin/env python

"""repeat [-i SECONDS] <shell-command>

This simple program repeatedly (at 1-second intervals) executes the
shell command given on the command line and displays the output (or as
much of it as fits on the screen).  It uses curses to paint each new
output on top of the old output, so that if nothing changes, the
screen doesn't change.  This is handy to watch for changes in e.g. a
directory or process listing.

The -i option lets you override the sleep time between executions.

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
import getopt

def main():
    interval = 1.0
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:")
    except getopt.error as err:
        print(err, file=sys.stderr)
        sys.exit(2)
    if not args:
        print(__doc__)
        sys.exit(0)
    for opt, arg in opts:
        if opt == "-i":
            interval = float(arg)
        if opt == "-h":
            print(__doc__)
            sys.exit(0)
    cmd = " ".join(args)
    cmd_really = cmd + " 2>&1"
    p = os.popen(cmd_really, "r")
    text = p.read()
    sts = p.close()
    text = addsts(interval, cmd, text, sts)
    w = curses.initscr()
    try:
        while True:
            w.erase()
            try:
                w.addstr(text)
            except curses.error:
                pass
            w.refresh()
            time.sleep(interval)
            p = os.popen(cmd_really, "r")
            text = p.read()
            sts = p.close()
            text = addsts(interval, cmd, text, sts)
    finally:
        curses.endwin()

def addsts(interval, cmd, text, sts):
    now = time.strftime("%H:%M:%S")
    text = "%s, every %g sec: %s\n%s" % (now, interval, cmd, text)
    if sts:
        msg = "Exit status: %d; signal: %d" % (sts>>8, sts&0xFF)
        if text and not text.endswith("\n"):
            msg = "\n" + msg
        text += msg
    return text

main()
