#! /ufs/guido/bin/sgi/python
#! /usr/local/python

# Watch line printer queue(s).
# Intended for BSD 4.3 lpq.

import posix
import sys
import time
import string

DEF_PRINTER = 'psc'
DEF_DELAY = 10

def main():
	delay = DEF_DELAY # XXX Use getopt() later
	try:
		thisuser = posix.environ['LOGNAME']
	except:
		thisuser = posix.environ['USER']
	printers = sys.argv[1:]
	if printers:
		# Strip '-P' from printer names just in case
		# the user specified it...
		for i in range(len(printers)):
			if printers[i][:2] = '-P':
				printers[i] = printers[i][2:]
	else:
		if posix.environ.has_key('PRINTER'):
			printers = [posix.environ['PRINTER']]
		else:
			printers = [DEF_PRINTER]
	#
	clearhome = posix.popen('clear', 'r').read()
	#
	while 1:
		# Pipe output through cat for extra buffering,
		# so the output (which overwrites the previous)
		# appears instantaneous.
		sys.stdout = posix.popen('exec cat', 'w')
		sys.stdout.write(clearhome)
		for name in printers:
			pipe = posix.popen('lpq -P' + name + ' 2>&1', 'r')
			showstatus(name, pipe, thisuser)
			sts = pipe.close()
			if sts:
				print name + ': *** lpq exit status', sts
		sts = sys.stdout.close()
		time.sleep(delay)

def showstatus(name, pipe, thisuser):
	lines = 0
	users = {}
	aheadbytes = 0
	aheadjobs = 0
	userseen = 0
	totalbytes = 0
	totaljobs = 0
	while 1:
		line = pipe.readline()
		if not line: break
		fields = string.split(line)
		n = len(fields)
		if len(fields) >= 6 and fields[n-1] = 'bytes':
			rank = fields[0]
			user = fields[1]
			job = fields[2]
			files = fields[3:-2]
			bytes = eval(fields[n-2])
			if user = thisuser:
				userseen = 1
			elif not userseen:
				aheadbytes = aheadbytes + bytes
				aheadjobs = aheadjobs + 1
			totalbytes = totalbytes + bytes
			totaljobs = totaljobs + 1
			if users.has_key(user):
				ujobs, ubytes = users[user]
			else:
				ujobs, ubytes = 0, 0
			ujobs = ujobs + 1
			ubytes = ubytes + bytes
			users[user] = ujobs, ubytes
		else:
			if fields and fields[0] <> 'Rank':
				if line[-1:] = '\n':
					line = line[:-1]
				if not lines:
					print name + ':',
				else:
					print
				print line,
				lines = lines + 1
	if totaljobs:
		if lines > 1:
			print
			lines = lines+1
		print (totalbytes+1023)/1024, 'K',
		if totaljobs <> len(users):
			print '(' + `totaljobs` + ' jobs)',
		if len(users) = 1:
			print 'for', users.keys()[0],
		else:
			print 'for', len(users), 'users',
			if userseen:
				if aheadjobs = 0:
					print '(' + thisuser + ' first)',
				else:
					print '(' + `(aheadbytes+1023)/1024`,
					print 'K before', thisuser + ')'
	if lines:
		print

try:
	main()
except KeyboardInterrupt:
	pass
