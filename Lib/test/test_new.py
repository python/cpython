from test_support import verbose
import sys
import new

class Eggs:
    def get_yolks(self):
        return self.yolks

print 'new.module()'
m = new.module('Spam')
if verbose:
    print m
m.Eggs = Eggs
sys.modules['Spam'] = m
import Spam

def get_more_yolks(self):
    return self.yolks + 3

print 'new.classobj()'
C = new.classobj('Spam', (Spam.Eggs,), {'get_more_yolks': get_more_yolks})
if verbose:
    print C
print 'new.instance()'
c = new.instance(C, {'yolks': 3})
if verbose:
    print c

def break_yolks(self):
    self.yolks = self.yolks - 2
print 'new.instancemethod()'
im = new.instancemethod(break_yolks, c, C)
if verbose:
    print im

if c.get_yolks() <> 3 and c.get_more_yolks() <> 6:
    print 'Broken call of hand-crafted class instance'
im()
if c.get_yolks() <> 1 and c.get_more_yolks() <> 4:
    print 'Broken call of hand-crafted instance method'

codestr = '''
a = 1
b = 2
c = a + b
'''

ccode = compile(codestr, '<string>', 'exec')
g = {'c': 0, '__builtins__': __builtins__}
# this test could be more robust
print 'new.function()'
func = new.function(ccode, g)
if verbose:
    print func
func()
if g['c'] <> 3:
    print 'Could not create a proper function object'

# bogus test of new.code()
print 'new.code()'
d = new.code(3, 3, 3, 3, codestr, (), (), (), "<string>", "<name>", 1, "")
if verbose:
    print d
