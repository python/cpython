# Ridiculously simple test of the winsound module for Windows.

import winsound, time
for i in range(100, 2000, 100):
    winsound.Beep(i, 75)
print "Hopefully you heard some sounds increasing in frequency!"
winsound.MessageBeep()
time.sleep(0.5)
winsound.MessageBeep(winsound.MB_OK)
time.sleep(0.5)
winsound.MessageBeep(winsound.MB_ICONASTERISK)
time.sleep(0.5)
winsound.MessageBeep(winsound.MB_ICONEXCLAMATION)
time.sleep(0.5)
winsound.MessageBeep(winsound.MB_ICONHAND)
time.sleep(0.5)
winsound.MessageBeep(winsound.MB_ICONQUESTION)
time.sleep(0.5)
