# Python test set -- part 2, opcodes

from test_support import *


print '2. Opcodes'
print 'XXX Not yet fully implemented'

print '2.1 try inside for loop'
n = 0
for i in range(10):
        n = n+i
        try: 1/0
        except NameError: pass
        except ZeroDivisionError: pass
        except TypeError: pass
        try: pass
        except: pass
        try: pass
        finally: pass
        n = n+i
if n <> 90:
        raise TestFailed, 'try inside for'


print '2.2 raise class exceptions'

class AClass: pass
class BClass(AClass): pass
class CClass: pass
class DClass(AClass):
    def __init__(self, ignore):
        pass

try: raise AClass()
except: pass

try: raise AClass()
except AClass: pass

try: raise BClass()
except AClass: pass

try: raise BClass()
except CClass: raise TestFailed
except: pass

a = AClass()
b = BClass()

try: raise AClass, b
except BClass, v:
        if v != b: raise TestFailed
else: raise TestFailed

try: raise b
except AClass, v:
        if v != b: raise TestFailed

# not enough arguments
try:  raise BClass, a
except TypeError: pass

try:  raise DClass, a
except DClass, v:
    if not isinstance(v, DClass):
        raise TestFailed

print '2.3 comparing function objects'

f = eval('lambda: None')
g = eval('lambda: None')
if f != g: raise TestFailed

f = eval('lambda a: a')
g = eval('lambda a: a')
if f != g: raise TestFailed

f = eval('lambda a=1: a')
g = eval('lambda a=1: a')
if f != g: raise TestFailed

f = eval('lambda: 0')
g = eval('lambda: 1')
if f == g: raise TestFailed

f = eval('lambda: None')
g = eval('lambda a: None')
if f == g: raise TestFailed

f = eval('lambda a: None')
g = eval('lambda b: None')
if f == g: raise TestFailed

f = eval('lambda a: None')
g = eval('lambda a=None: None')
if f == g: raise TestFailed

f = eval('lambda a=0: None')
g = eval('lambda a=1: None')
if f == g: raise TestFailed
