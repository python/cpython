#! /usr/local/bin/python

import regex
import regsub
import glob
import sys
import os
import stat
import getopt

oldcprt = 'Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,\nAmsterdam, The Netherlands.'
newcprt = 'Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,\nAmsterdam, The Netherlands.'

oldprog = regex.compile(oldcprt)
newprog = regex.compile(newcprt)

def main():
	opts, args = getopt.getopt(sys.argv[1:], 'y:')
	agelimit = 0L
	for opt, arg in opts:
		if opt == '-y':
			agelimit = os.stat(arg)[stat.ST_MTIME]
	if not args:
		args = glob.glob('*.[ch]')
	for file in args:
		try:
			age = os.stat(file)[stat.ST_MTIME]
		except os.error, msg:
			print file, ': stat failed :', msg
			continue
		if age <= agelimit:
			print file, ': too old, skipped'
			continue
		try:
			f = open(file, 'r')
		except IOError, msg:
			print file, ': open failed :', msg
			continue
		head = f.read(1024)
		if oldprog.search(head) < 0:
			if newprog.search(head) < 0:
				print file, ': NO COPYRIGHT FOUND'
			else:
				print file, ': (new copyright already there)'
			f.close()
			continue
		newhead = regsub.sub(oldcprt, newcprt, head)
		data = newhead + f.read()
		f.close()
		try:
			f = open(file + '.new', 'w')
		except IOError, msg:
			print file, ': creat failed :', msg
			continue
		f.write(data)
		f.close()
		try:
			os.rename(file, file + '~')
		except IOError, msg:
			print file, ': rename to backup failed :', msg
			continue
		try:
			os.rename(file + '.new', file)
		except IOError, msg:
			print file, ': rename from .new failed :', msg
			continue
		print file, ': copyright changed.'

main()
