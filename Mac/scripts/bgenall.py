# bgenall - Generate all bgen-generated modules
#
import sys
import os
import string

def bgenone(dirname, shortname):
	os.chdir(dirname)
	m = __import__(shortname+'scan')
	try:
		m.main()
	except:
		return 0
	return 1
	
def main():
	success = []
	failure = []
	sys.path.insert(0, ':')
	srcdir = os.path.join(os.path.join(sys.prefix, 'Mac'), 'Modules')
	contents = os.listdir(srcdir)
	for name in contents:
		moduledir = os.path.join(srcdir, name)
		scanmodule = os.path.join(moduledir, name +'scan.py')
		if os.path.exists(scanmodule):
			if bgenone(moduledir, name):
				success.append(name)
			else:
				failure.append(name)
	print 'Done:', string.join(success, ' ')
	if failure:
		print 'Failed:', string.join(failure, ' ')
		return 0
	return 1
	
if __name__ == '__main__':
	rv = main()
	sys.exit(not rv)