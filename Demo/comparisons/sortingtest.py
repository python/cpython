#! /usr/bin/env python

# 2)  Sorting Test
#
#     Sort an input file that consists of lines like this
#
#         var1=23 other=14 ditto=23 fred=2
#
#     such that each output line is sorted WRT to the number.  Order
#     of output lines does not change.  Resolve collisions using the
#     variable name.   e.g.
#
#         fred=2 other=14 ditto=23 var1=23
#
#     Lines may be up to several kilobytes in length and contain
#     zillions of variables.

# This implementation:
# - Reads stdin, writes stdout
# - Uses any amount of whitespace to separate fields
# - Allows signed numbers
# - Treats illegally formatted fields as field=0
# - Outputs the sorted fields with exactly one space between them
# - Handles blank input lines correctly

import re
import string
import sys

def main():
    prog = re.compile('^(.*)=([-+]?[0-9]+)')
    def makekey(item, prog=prog):
        match = prog.match(item)
        if match:
            var, num = match.group(1, 2)
            return string.atoi(num), var
        else:
            # Bad input -- pretend it's a var with value 0
            return 0, item
    while 1:
        line = sys.stdin.readline()
        if not line:
            break
        items = line.split()
        items = list(map(makekey, items))
        items.sort()
        for num, var in items:
            print("%s=%s" % (var, num), end=' ')
        print()

main()
