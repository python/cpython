# Test TE module, simple version

from Win import *
from TE import *
import Qd

r = (40, 40, 140, 140)
w = NewWindow(r, "TETextBox test", 1, 0, -1, 1, 0x55555555)
##w.DrawGrowIcon()

r = (10, 10, 90, 90)

Qd.SetPort(w)
t = TETextBox("Nobody expects the SPANISH inquisition", r, 1)

import time
time.sleep(10)
