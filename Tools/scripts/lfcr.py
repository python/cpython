#! /usr/bin/env python

"Replace LF with CRLF in argument files.  Print names of changed files."

import sys, re, os
for file in sys.argv[1:]:
    if os.path.isdir(file):
        print file, "Directory!"
        continue
    data = open(file, "rb").read()
    if '\0' in data:
        print file, "Binary!"
        continue
    newdata = re.sub("\r?\n", "\r\n", data)
    if newdata != data:
        print file
        f = open(file, "wb")
        f.write(newdata)
        f.close()
