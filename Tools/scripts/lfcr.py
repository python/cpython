#! /usr/bin/env python

"Replace LF with CRLF in argument files.  Print names of changed files."

import sys, re, os

def main():
    for filename in sys.argv[1:]:
        if os.path.isdir(filename):
            print filename, "Directory!"
            continue
        data = open(filename, "rb").read()
        if '\0' in data:
            print filename, "Binary!"
            continue
        newdata = re.sub("\r?\n", "\r\n", data)
        if newdata != data:
            print filename
            f = open(filename, "wb")
            f.write(newdata)
            f.close()

if __name__ == '__main__':
    main()
