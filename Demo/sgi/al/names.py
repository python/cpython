import sys

# My home directory/
GUIDO = '/ufs/guido/'

# Hack sys.path so AL can be found
LIB = GUIDO + 'lib/python'
if LIB not in sys.path: sys.path.insert(0, LIB)

# Python binary to be used on remote machine
PYTHON = GUIDO + 'bin/sgi/python'

# Directory where the programs live
AUDIODIR = GUIDO + 'src/python/demo/sgi/al'
