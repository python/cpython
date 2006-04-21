# http://www.python.org/sf/1456209

# A dictresize() attack.  If oldtable == mp->ma_smalltable then pure
# Python code can mangle with mp->ma_smalltable while it is being walked
# over.

class X(object):

    def __hash__(self):
        return 5

    def __eq__(self, other):
        if resizing:
            d.clear()
        return False


d = {}

resizing = False

d[X()] = 1
d[X()] = 2
d[X()] = 3
d[X()] = 4
d[X()] = 5

# now trigger a resize
resizing = True
d[9] = 6

# ^^^ I get Segmentation fault or Illegal instruction here.
