# Ridiculously simple test of the winsound module for Windows.

import winsound
for i in range(100, 2000, 100):
    winsound.Beep(i, 75)
print "Hopefully you heard some sounds increasing in frequency!"
