# Python test set -- part 3, built-in operations.


print '3. Operations'
print 'XXX Mostly not yet implemented'


print '3.1 Dictionary lookups succeed even if __cmp__() raises an exception'

# SourceForge bug #112558:
# http://sourceforge.net/bugs/?func=detailbug&bug_id=112558&group_id=5470

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
d[x2] = 2
print "No exception passed through."
