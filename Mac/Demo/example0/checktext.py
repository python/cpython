"""checktext - Check that a text file has macintosh-style newlines"""

import sys
import EasyDialogs
import string

def main():
    pathname = EasyDialogs.AskFileForOpen(message='File to check end-of-lines in:')
    if not pathname:
        sys.exit(0)
    fp = open(pathname, 'rb')
    try:
        data = fp.read()
    except MemoryError:
        EasyDialogs.Message('Sorry, file is too big.')
        sys.exit(0)
    if len(data) == 0:
        EasyDialogs.Message('File is empty.')
        sys.exit(0)
    number_cr = string.count(data, '\r')
    number_lf = string.count(data, '\n')
    if number_cr == number_lf == 0:
        EasyDialogs.Message('File contains no lines.')
    if number_cr == 0:
        EasyDialogs.Message('File has unix-style line endings')
    elif number_lf == 0:
        EasyDialogs.Message('File has mac-style line endings')
    elif number_cr == number_lf:
        EasyDialogs.Message('File probably has MSDOS-style line endings')
    else:
        EasyDialogs.Message('File has no recognizable line endings (binary file?)')
    sys.exit(0)

if __name__ == '__main__':
    main()
