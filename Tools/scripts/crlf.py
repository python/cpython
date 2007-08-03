#! /usr/bin/env python
"Replace CRLF with LF in argument files.  Print names of changed files."

import sys, os

def main():
    for filename in sys.argv[1:]:
        if os.path.isdir(filename):
            print(filename, "Directory!")
            continue
        data = open(filename, "rb").read()
        if '\0' in data:
            print(filename, "Binary!")
            continue
        newdata = data.replace("\r\n", "\n")
        if newdata != data:
            print(filename)
            f = open(filename, "wb")
            f.write(newdata)
            f.close()

if __name__ == '__main__':
    main()
