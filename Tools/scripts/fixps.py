#!/usr/bin/env python

# Fix Python script(s) to reference the interpreter via /usr/bin/env python.
# Warning: this overwrites the file without making a backup.

import sys
import re


def main():
    for filename in sys.argv[1:]:
        try:
            f = open(filename, 'r')
        except IOError as msg:
            print(filename, ': can\'t open :', msg)
            continue
        line = f.readline()
        if not re.match('^#! */usr/local/bin/python', line):
            print(filename, ': not a /usr/local/bin/python script')
            f.close()
            continue
        rest = f.read()
        f.close()
        line = re.sub('/usr/local/bin/python',
                      '/usr/bin/env python', line)
        print(filename, ':', repr(line))
        f = open(filename, "w")
        f.write(line)
        f.write(rest)
        f.close()

if __name__ == '__main__':
    main()
