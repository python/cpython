#! /usr/bin/env python

# suff
#
# show different suffixes amongst arguments

import sys

def main():
	files = sys.argv[1:]
	suffixes = {}
	for file in files:
		suff = getsuffix(file)
		if not suffixes.has_key(suff):
			suffixes[suff] = []
		suffixes[suff].append(file)
	keys = suffixes.keys()
	keys.sort()
	for suff in keys:
		print `suff`, len(suffixes[suff])

def getsuffix(file):
	suff = ''
	for i in range(len(file)):
		if file[i] == '.':
			suff = file[i:]
	return suff

main()
