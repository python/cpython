import os
import sys
import string

MAXFD = 256	# Max number of file descriptors (os.getdtablesize()???)

_active = []

def _cleanup():
    for inst in _active[:]:
	inst.poll()

class Popen3:
    def __init__(self, cmd, capturestderr=0):
	if type(cmd) == type(''):
	    cmd = ['/bin/sh', '-c', cmd]
	p2cread, p2cwrite = os.pipe()
	c2pread, c2pwrite = os.pipe()
	if capturestderr:
	    errout, errin = os.pipe()
	self.pid = os.fork()
	if self.pid == 0:
	    # Child
	    os.close(0)
	    os.close(1)
	    if os.dup(p2cread) <> 0:
		sys.stderr.write('popen2: bad read dup\n')
	    if os.dup(c2pwrite) <> 1:
		sys.stderr.write('popen2: bad write dup\n')
	    if capturestderr:
		os.close(2)
		if os.dup(errin) <> 2: pass
	    for i in range(3, MAXFD):
		try:
		    os.close(i)
		except: pass
	    try:
		os.execvp(cmd[0], cmd)
	    finally:
		os._exit(1)
	    # Shouldn't come here, I guess
	    os._exit(1)
	os.close(p2cread)
	self.tochild = os.fdopen(p2cwrite, 'w')
	os.close(c2pwrite)
	self.fromchild = os.fdopen(c2pread, 'r')
	if capturestderr:
	    os.close(errin)
	    self.childerr = os.fdopen(errout, 'r')
	else:
	    self.childerr = None
	self.sts = -1 # Child not completed yet
	_active.append(self)
    def poll(self):
	if self.sts < 0:
	    try:
		pid, sts = os.waitpid(self.pid, os.WNOHANG)
		if pid == self.pid:
		    self.sts = sts
		    _active.remove(self)
	    except os.error:
		pass
	return self.sts
    def wait(self):
	pid, sts = os.waitpid(self.pid, 0)
	if pid == self.pid:
	    self.sts = sts
	    _active.remove(self)
	return self.sts

def popen2(cmd):
    _cleanup()
    inst = Popen3(cmd, 0)
    return inst.fromchild, inst.tochild

def popen3(cmd):
    _cleanup()
    inst = Popen3(cmd, 1)
    return inst.fromchild, inst.tochild, inst.childerr

def _test():
    teststr = "abc\n"
    print "testing popen2..."
    r, w = popen2('cat')
    w.write(teststr)
    w.close()
    assert r.read() == teststr
    print "testing popen3..."
    r, w, e = popen3(['cat'])
    w.write(teststr)
    w.close()
    assert r.read() == teststr
    assert e.read() == ""
    _cleanup()
    assert not _active
    print "All OK"

if __name__ == '__main__':
    _test()
