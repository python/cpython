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
	finally: pass
	try: pass
	except: pass
	try: pass
	finally: pass
	n = n+i
if n <> 90:
	raise TestFailed, 'try inside for'
