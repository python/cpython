# Test Win module

from Win import *

r = (40, 40, 400, 300)
w = NewWindow(r, "Hello world", 1, 0, -1, 1, 0x55555555)
w.DrawGrowIcon()
import time
time.sleep(10)
