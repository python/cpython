# tty.py -- Terminal utilities.
# Author: Steen Lumholt.

from TERMIOS import *
from termios import *

# Indexes for termios list. 
IFLAG = 0
OFLAG = 1
CFLAG = 2
LFLAG = 3
ISPEED = 4
OSPEED = 5
CC = 6

# Put terminal into a raw mode.
def setraw(fd, when=TCSAFLUSH):
	mode = tcgetattr(fd)
	mode[IFLAG] = mode[IFLAG] & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON)
	mode[OFLAG] = mode[OFLAG] & ~(OPOST)
	mode[CFLAG] = mode[CFLAG] & ~(CSIZE | PARENB)
	mode[CFLAG] = mode[CFLAG] | CS8
	mode[LFLAG] = mode[LFLAG] & ~(ECHO | ICANON | IEXTEN | ISIG)
	mode[CC][VMIN] = 1
	mode[CC][VTIME] = 0
	tcsetattr(fd, when, mode)

# Put terminal into a cbreak mode.
def setcbreak(fd, when=TCSAFLUSH):
	mode = tcgetattr(fd)
	mode[LFLAG] = mode[LFLAG] & ~(ECHO | ICANON)
	mode[CC][VMIN] = 1
	mode[CC][VTIME] = 0
	tcsetattr(fd, when, mode)

