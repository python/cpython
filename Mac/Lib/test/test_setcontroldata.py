import W
from Controls import *

w = W.Window((200,200), "Test")

w.c = W.Button((5,5,100,50), "Test")

w.open()

print repr(w.c._control.GetControlData(0, 'dflt'))

str = '\001'
w.c._control.SetControlData(0, 'dflt', str)