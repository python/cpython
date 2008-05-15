from warnings import warnpy3k
warnpy3k("the GLWS module has been removed in Python 3.0", stacklevel=2)
del warnpy3k

NOERROR = 0
NOCONTEXT = -1
NODISPLAY = -2
NOWINDOW = -3
NOGRAPHICS = -4
NOTTOP = -5
NOVISUAL = -6
BUFSIZE = -7
BADWINDOW = -8
ALREADYBOUND = -100
BINDFAILED = -101
SETFAILED = -102
