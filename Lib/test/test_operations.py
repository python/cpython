# Python test set -- part 3, built-in operations.


print '3. Operations'
print 'XXX Mostly not yet implemented'


print '3.1 Dictionary lookups fail if __cmp__() raises an exception'

class BadDictKey:

    def __hash__(self):
        return hash(self.__class__)

    def __cmp__(self, other):
        if isinstance(other, self.__class__):
            print "raising error"
            raise RuntimeError, "gotcha"
        return other

d = {}
x1 = BadDictKey()
x2 = BadDictKey()
d[x1] = 1
for stmt in ['d[x2] = 2',
             'z = d[x2]',
             'x2 in d',
             'd.has_key(x2)',
             'd.get(x2)',
             'd.setdefault(x2, 42)',
             'd.pop(x2)',
             'd.update({x2: 2})']:
    try:
        exec stmt
    except RuntimeError:
        print "%s: caught the RuntimeError outside" % (stmt,)
    else:
        print "%s: No exception passed through!"     # old CPython behavior


# Dict resizing bug, found by Jack Jansen in 2.2 CVS development.
# This version got an assert failure in debug build, infinite loop in
# release build.  Unfortunately, provoking this kind of stuff requires
# a mix of inserts and deletes hitting exactly the right hash codes in
# exactly the right order, and I can't think of a randomized approach
# that would be *likely* to hit a failing case in reasonable time.

d = {}
for i in range(5):
    d[i] = i
for i in range(5):
    del d[i]
for i in range(5, 9):  # i==8 was the problem
    d[i] = i


# Another dict resizing bug (SF bug #1456209).
# This caused Segmentation faults or Illegal instructions.

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

print 'resize bugs not triggered.'
