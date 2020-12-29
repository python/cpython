#! /usr/bin/env python3

"""Script to search with Google

Usage:
    python3 google.py [search terms]
"""

import sys
import urllib.parse
import webbrowser


def main(args):
    def quote(arg):
        if ' ' in arg:
            arg = '"%s"' % arg
        return urllib.parse.quote_plus(arg)

    qstring = '%20'.join(quote(arg) for arg in args)
    url = urllib.parse.urljoin('https://www.google.com/search', '?q=' + qstring)
    webbrowser.open(url)

if __name__ == '__main__':
    if len(sys.argv)>1:
        main(sys.argv[1:])
    else:
        print("Enter what you want to search in Google\n"+
              "example : google.py python")
        sys.exit(1)
