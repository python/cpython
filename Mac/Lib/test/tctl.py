# play with controls

from Dlg import *
from Ctl import *
from Win import *
from Evt import *
import time
import sys

def main():
	r = (40, 40, 400, 300)
	w = NewWindow(r, "The Spanish Inquisition", 1, 0, -1, 1, 0x55555555)
	w.DrawGrowIcon()
	r = (40, 40, 100, 60)
	c = NewControl(w, r, "SPAM!", 1, 0, 0, 1, 0, 0)
	print 'Ok, try it...'
	sys.exit(1)  # So we can see what happens...


main()
