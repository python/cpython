#! /bin/env python

"""Script to write MIF files from ref.book and ref*.doc."""

import os
import glob
import string

def main():
    files = ['ref.book'] + glob.glob('ref*.doc')
    files.sort()
    print "Files:", string.join(files)
    print "Starting FrameMaker..."
    pipe = os.popen("fmbatch", 'w')
    for i in files:
	cmd = "Open %s\nSaveAs m %s %s.MIF\n" % (i, i, os.path.splitext(i)[0])
	print cmd
	pipe.write(cmd)
    pipe.write("Quit\n")
    sts = pipe.close()
    if sts:
	print "Exit status", hex(sts)
    else:
	print "Starting webmaker..."
	os.system('/depot/sundry/src/webmaker/webmaker-sparc/webmaker -c ref.wml -t "Python 1.5 Reference Manual" ref.MIF')

if __name__ == '__main__':
    main()
