from test_support import verbose

class XReader:
    def __init__(self):
        self.count = 5

    def readlines(self, sizehint = None):
        self.count = self.count - 1
        return map(lambda x: "%d\n" % x, range(self.count))

class Null: pass

import xreadlines


lineno = 0

try:
    xreadlines.xreadlines(Null())[0]
except AttributeError, detail:
    print "AttributeError (expected)"
else:
    print "Did not throw attribute error"

try:
    xreadlines.xreadlines(XReader)[0]
except TypeError, detail:
    print "TypeError (expected)"
else:
    print "Did not throw type error"

try:
    xreadlines.xreadlines(XReader())[1]
except RuntimeError, detail:
    print "RuntimeError (expected):", detail
else:
    print "Did not throw runtime error"

xresult = ['0\n', '1\n', '2\n', '3\n', '0\n', '1\n', '2\n', '0\n', '1\n', '0\n']
for line in xreadlines.xreadlines(XReader()):
    if line != xresult[lineno]:
        print "line %d differs" % lineno
    lineno += 1
