#! /bin/env python

"""Script to write MIF files from ref.book and ref*.doc."""

import os
import glob

def main():
    pipe = os.popen("fmbatch", 'w')
    for i in ['ref.book'] + glob.glob('ref*.doc'):
	cmd = "Open %s\nSaveAs m %s %s.MIF\n" % (i, i, os.path.splitext(i)[0])
	print cmd
	pipe.write(cmd)
    pipe.write("Quit\n")

if __name__ == '__main__':
    main()
