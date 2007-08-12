#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

"""
Run a Python script under hotshot's control.

Adapted from a posting on python-dev by Walter Dörwald

usage %prog [ %prog args ] filename [ filename args ]

Any arguments after the filename are used as sys.argv for the filename.
"""

import sys
import optparse
import os
import hotshot
import hotshot.stats

PROFILE = "hotshot.prof"

def run_hotshot(filename, profile, args):
    prof = hotshot.Profile(profile)
    sys.path.insert(0, os.path.dirname(filename))
    sys.argv = [filename] + args
    fp = open(filename)
    try:
        script = fp.read()
    finally:
        fp.close()
    prof.run("exec(%r)" % script)
    prof.close()
    stats = hotshot.stats.load(profile)
    stats.sort_stats("time", "calls")

    # print_stats uses unadorned print statements, so the only way
    # to force output to stderr is to reassign sys.stdout temporarily
    save_stdout = sys.stdout
    sys.stdout = sys.stderr
    stats.print_stats()
    sys.stdout = save_stdout

    return 0

def main(args):
    parser = optparse.OptionParser(__doc__)
    parser.disable_interspersed_args()
    parser.add_option("-p", "--profile", action="store", default=PROFILE,
                      dest="profile", help='Specify profile file to use')
    (options, args) = parser.parse_args(args)

    if len(args) == 0:
        parser.print_help("missing script to execute")
        return 1

    filename = args[0]
    return run_hotshot(filename, options.profile, args[1:])

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
