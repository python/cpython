#! /usr/bin/env python

# A rather specialized script to make sure that a symbolic link named
# RCS exists pointing to a real RCS directory in a parallel tree
# referenced as RCStree in an ancestor directory.
# (I use this because I like my RCS files to reside on a physically
# different machine).

import os

def main():
	rcstree = 'RCStree'
	rcs = 'RCS'
	if os.path.islink(rcs):
		print `rcs`, 'is a symlink to', `os.readlink(rcs)`
		return
	if os.path.isdir(rcs):
		print `rcs`, 'is an ordinary directory'
		return
	if os.path.exists(rcs):
		print `rcs`, 'is a file?!?!'
		return
	#
	p = os.getcwd()
	up = ''
	down = ''
	# Invariants:
	# (1) join(p, down) is the current directory
	# (2) up is the same directory as p
	# Ergo:
	# (3) join(up, down) is the current directory
	#print 'p =', `p`
	while not os.path.isdir(os.path.join(p, rcstree)):
		head, tail = os.path.split(p)
		#print 'head =', `head`, '; tail =', `tail`
		if not tail:
			print 'Sorry, no ancestor dir contains', `rcstree`
			return
		p = head
		up = os.path.join(os.pardir, up)
		down = os.path.join(tail, down)
		#print 'p =', `p`, '; up =', `up`, '; down =', `down`
	there = os.path.join(up, rcstree)
	there = os.path.join(there, down)
	there = os.path.join(there, rcs)
	if os.path.isdir(there):
		print `there`, 'already exists'
	else:
		print 'making', `there`
		makedirs(there)
	print 'making symlink', `rcs`, '->', `there`
	os.symlink(there, rcs)

def makedirs(p):
	if not os.path.isdir(p):
		head, tail = os.path.split(p)
		makedirs(head)
		os.mkdir(p, 0777)

main()
