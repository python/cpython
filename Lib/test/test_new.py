import sys
import new

class Eggs:
    def get_yolks(self):
	return self.yolks

m = new.module('Spam')
m.Eggs = Eggs
sys.modules['Spam'] = m
import Spam

def get_more_yolks(self):
    return self.yolks + 3

C = new.classobj('Spam', (Spam.Eggs,), {'get_more_yolks': get_more_yolks})
c = new.instance(C, {'yolks': 3})

def break_yolks(self):
    self.yolks = self.yolks - 2
im = new.instancemethod(break_yolks, c, C)

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
func = new.function(ccode, g)
func()
if g['c'] <> 3:
    print 'Could not create a proper function object'

# bogus test of new.code()
new.code(3, 3, 3, codestr, (), (), (), "<string>", "<name>")
