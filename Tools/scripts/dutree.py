#! /usr/bin/env python
# Format du output in a tree shape

import os, string, sys, errno

def main():
	p = os.popen('du ' + string.join(sys.argv[1:]), 'r')
	total, d = None, {}
	for line in p.readlines():
		i = 0
		while line[i] in '0123456789': i = i+1
		size = eval(line[:i])
		while line[i] in ' \t': i = i+1
		file = line[i:-1]
		comps = string.splitfields(file, '/')
		if comps[0] == '': comps[0] = '/'
		if comps[len(comps)-1] == '': del comps[len(comps)-1]
		total, d = store(size, comps, total, d)
	try:
		display(total, d)
	except IOError, e:
		if e.errno != errno.EPIPE:
			raise

def store(size, comps, total, d):
	if comps == []:
		return size, d
	if not d.has_key(comps[0]):
		d[comps[0]] = None, {}
	t1, d1 = d[comps[0]]
	d[comps[0]] = store(size, comps[1:], t1, d1)
	return total, d

def display(total, d):
	show(total, d, '')

def show(total, d, prefix):
	if not d: return
	list = []
	sum = 0
	for key in d.keys():
		tsub, dsub = d[key]
		list.append((tsub, key))
		if tsub is not None: sum = sum + tsub
## 	if sum < total:
## 		list.append((total - sum, os.curdir))
	list.sort()
	list.reverse()
	width = len(`list[0][0]`)
	for tsub, key in list:
		if tsub is None:
			psub = prefix
		else:
			print prefix + string.rjust(`tsub`, width) + ' ' + key
			psub = prefix + ' '*(width-1) + '|' + ' '*(len(key)+1)
		if d.has_key(key):
			show(tsub, d[key][1], psub)

main()
