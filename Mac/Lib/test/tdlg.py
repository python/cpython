# This program requires that a DLOG resource with ID=128 exists.
# You can make one with ResEdit if necessary.

from Res import *
from Dlg import *

ires = 128

def filter(*args): print 'filter:', args

d = GetNewDialog(ires, -1)
while 1:
	n = ModalDialog(filter)
	print 'item:', n
	if n == 1: break
