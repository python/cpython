#! /usr/bin/env python

# print md5 checksum for files

bufsize = 8096
fnfilter = None
rmode = 'r'

usage = """
usage: sum5 [-b] [-t] [-l] [-s bufsize] [file ...]
-b        : read files in binary mode
-t        : read files in text mode (default)
-l        : print last pathname component only
-s bufsize: read buffer size (default %d)
file ...  : files to sum; '-' or no files means stdin
""" % bufsize

import sys
import string
import os
import md5
import regsub

StringType = type('')
FileType = type(sys.stdin)

def sum(*files):
	sts = 0
	if files and type(files[-1]) == FileType:
		out, files = files[-1], files[:-1]
	else:
		out = sys.stdout
	if len(files) == 1 and type(files[0]) != StringType:
		files = files[0]
	for f in files:
		if type(f) == StringType:
			if f == '-':
				sts = printsumfp(sys.stdin, '<stdin>', out) or sts
			else:
				sts = printsum(f, out) or sts
		else:
			sts = sum(f, out) or sts
	return sts

def printsum(file, out = sys.stdout):
	try:
		fp = open(file, rmode)
	except IOError, msg:
		sys.stderr.write('%s: Can\'t open: %s\n' % (file, msg))
		return 1
	if fnfilter:
		file = fnfilter(file)
	sts = printsumfp(fp, file, out)
	fp.close()
	return sts

def printsumfp(fp, file, out = sys.stdout):
	m = md5.md5()
	try:
		while 1:
			data = fp.read(bufsize)
			if not data: break
			m.update(data)
	except IOError, msg:
		sys.stderr.write('%s: I/O error: %s\n' % (file, msg))
		return 1
	out.write('%s %s\n' % (hexify(m.digest()), file))
	return 0

def hexify(s):
	res = ''
	for c in s:
		res = res + '%02x' % ord(c)
	return res

def main(args = sys.argv[1:], out = sys.stdout):
	global fnfilter, rmode, bufsize
	import getopt
	try:
		opts, args = getopt.getopt(args, 'blts:')
	except getopt.error, msg:
		sys.stderr.write('%s: %s\n%s' % (sys.argv[0], msg, usage))
		return 2
	for o, a in opts:
		if o == '-l':
			fnfilter = os.path.basename
		if o == '-b':
			rmode = 'rb'
		if o == '-t':
			rmode = 'r'
		if o == '-s':
			bufsize = string.atoi(a)
	if not args: args = ['-']
	return sum(args, out)

if __name__ == '__main__' or __name__ == sys.argv[0]:
	sys.exit(main(sys.argv[1:], sys.stdout))
