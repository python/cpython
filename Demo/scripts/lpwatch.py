#! /usr/bin/env python

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
			if printers[i][:2] == '-P':
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
		text = clearhome
		for name in printers:
			text = text + makestatus(name, thisuser) + '\n'
		print text
		time.sleep(delay)

def makestatus(name, thisuser):
	pipe = posix.popen('lpq -P' + name + ' 2>&1', 'r')
	lines = []
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
		if len(fields) >= 6 and fields[n-1] == 'bytes':
			rank = fields[0]
			user = fields[1]
			job = fields[2]
			files = fields[3:-2]
			bytes = eval(fields[n-2])
			if user == thisuser:
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
				line = string.strip(line)
				if line == 'no entries':
					line = name + ': idle'
				elif line[-22:] == ' is ready and printing':
					line = name
				lines.append(line)
	#
	if totaljobs:
		line = `(totalbytes+1023)/1024` + ' K'
		if totaljobs <> len(users):
			line = line + ' (' + `totaljobs` + ' jobs)'
		if len(users) == 1:
			line = line + ' for ' + users.keys()[0]
		else:
			line = line + ' for ' + `len(users)` + ' users'
			if userseen:
				if aheadjobs == 0:
				  line =  line + ' (' + thisuser + ' first)'
				else:
				  line = line + ' (' + `(aheadbytes+1023)/1024`
				  line = line + ' K before ' + thisuser + ')'
		lines.append(line)
	#
	sts = pipe.close()
	if sts:
		lines.append('lpq exit status ' + `sts`)
	return string.joinfields(lines, ': ')

try:
	main()
except KeyboardInterrupt:
	pass
