#! /usr/bin/env python

# This Python program sorts and reformats the table of keywords in ref2.tex

l = []
try:
	while 1:
		l = l + raw_input().split()
except EOFError:
	pass
l.sort()
for x in l[:]:
	while l.count(x) > 1: l.remove(x)
ncols = 5
nrows = (len(l)+ncols-1)/ncols
for i in range(nrows):
	for j in range(i, len(l), nrows):
		print l[j].ljust(10),
	print
