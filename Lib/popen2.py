import os
import sys
import string

MAXFD = 100	# Max number of file descriptors (os.getdtablesize()???)

def popen2(cmd):
	cmd = ['/bin/sh', '-c', cmd]
	p2cread, p2cwrite = os.pipe()
	c2pread, c2pwrite = os.pipe()
	pid = os.fork()
	if pid == 0:
		# Child
		os.close(0)
		os.close(1)
		if os.dup(p2cread) <> 0:
			sys.stderr.write('popen2: bad read dup\n')
		if os.dup(c2pwrite) <> 1:
			sys.stderr.write('popen2: bad write dup\n')
		for i in range(3, MAXFD):
			try:
				os.close(i)
			except:
				pass
		try:
			os.execv(cmd[0], cmd)
		finally:
			os._exit(1)
		# Shouldn't come here, I guess
		os._exit(1)
	os.close(p2cread)
	tochild = os.fdopen(p2cwrite, 'w')
	os.close(c2pwrite)
	fromchild = os.fdopen(c2pread, 'r')
	return fromchild, tochild
