#!/usr/bin/python

import shutil
import getopt
import os
import sys

WORKDIR = "/tmp/_py"

def main():
    workdir = WORKDIR

    try:
        options, args = getopt.getopt(sys.argv[1:], "?hbo",
                [ "build-dir=", "output=", "help" ])
    except getopt.GetoptError:
        print(sys.exc_info()[1])
        sys.exit(1)

    if args:
        print("Additional arguments")
        sys.exit(1)

    for k, v in options:
        if k in ("-h", "-?", "--help"):
            print(USAGE)
            sys.exit(0)

        elif k in ("b", "--build-dir"):
            workdir=v

        else:
            raise SystemExit("Unknown option")

    root = os.path.join(workdir, "libraries", "Library", "Frameworks")
    shutil.make_archive("third-party", "zip", root_dir=root)

if __name__ == "__main__":
   main()
