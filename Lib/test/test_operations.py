# Python test set -- part 3, built-in operations.


print '3. Operations'
print 'XXX Mostly not yet implemented'


print '3.1 Dictionary lookups succeed even if __cmp__() raises an exception'

# SourceForge bug #112558:
# http://sourceforge.net/bugs/?func=detailbug&bug_id=112558&group_id=5470

class BadDictKey:
    already_printed_raising_error = 0

    def __hash__(self):
        return hash(self.__class__)

    def __cmp__(self, other):
        if isinstance(other, self.__class__):
            if not BadDictKey.already_printed_raising_error:
                # How many times __cmp__ gets called depends on the hash
                # code and the internals of the dict implementation; we
                # know it will be called at least once, but that's it.
                # already_printed_raising_error makes sure the expected-
                # output file prints the msg at most once.
                BadDictKey.already_printed_raising_error = 1
                print "raising error"
            raise RuntimeError, "gotcha"
        return other

d = {}
x1 = BadDictKey()
x2 = BadDictKey()
d[x1] = 1
d[x2] = 2
print "No exception passed through."

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
