"""Simple script to replace @DATE@ and friends with real information.

Usage:  rewrite.py boilerplate.tex [VAR=value] ... <template >output
"""

import sys
import time


def get_info(fp):
    s = fp.read()

    d = {}
    start = s.find(r"\date{")
    if start >= 0:
        end = s.find("}", start)
        date = s[start+6:end]
        if date == r"\today":
            date = time.strftime("%B %d, %Y", time.localtime(time.time()))
        d["DATE"] = date
    return d


def main():
    s = sys.stdin.read()
    if "@" in s:
        # yes, we actully need to load the replacement values
        d = get_info(open(sys.argv[1]))
        for arg in sys.argv[2:]:
            name, value = arg.split("=", 1)
            d[name] = value
        start = 0
        while 1:
            start = s.find("@", start)
            if start < 0:
                break
            end = s.find("@", start+1)
            name = s[start+1:end]
            if name:
                value = d.get(name)
                if value is None:
                    start = end + 1
                else:
                    s = s[:start] + value + s[end+1:]
                    start = start + len(value)
            else:
                # "@@" --> "@"
                s = s[:start] + s[end:]
                start = end
    sys.stdout.write(s)


if __name__ == "__main__":
    main()
