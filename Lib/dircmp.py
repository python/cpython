# Module 'dirmp'
#
# Defines a class to build directory diff tools on.

import posix

import path

import dircache
import cmpcache
import statcache
from stat import *

# Directory comparison class.
#
class dircmp():
	#
	def new(dd, (a, b)): # Initialize
		dd.a = a
		dd.b = b
		# Properties that caller may change before callingdd. run():
		dd.hide = ['.', '..'] # Names never to be shown
		dd.ignore = ['RCS', 'tags'] # Names ignored in comparison
		#
		return dd
	#
	def run(dd): # Compare everything except common subdirectories
		dd.a_list = filter(dircache.listdir(dd.a), dd.hide)
		dd.b_list = filter(dircache.listdir(dd.b), dd.hide)
		dd.a_list.sort()
		dd.b_list.sort()
		dd.phase1()
		dd.phase2()
		dd.phase3()
	#
	def phase1(dd): # Compute common names
		dd.a_only = []
		dd.common = []
		for x in dd.a_list:
			if x in dd.b_list:
				dd.common.append(x)
			else:
				dd.a_only.append(x)
		#
		dd.b_only = []
		for x in dd.b_list:
			if x not in dd.common:
				dd.b_only.append(x)
	#
	def phase2(dd): # Distinguish files, directories, funnies
		dd.common_dirs = []
		dd.common_files = []
		dd.common_funny = []
		#
		for x in dd.common:
			a_path = path.cat(dd.a, x)
			b_path = path.cat(dd.b, x)
			#
			ok = 1
			try:
				a_stat = statcache.stat(a_path)
			except posix.error, why:
				# print 'Can\'t stat', a_path, ':', why[1]
				ok = 0
			try:
				b_stat = statcache.stat(b_path)
			except posix.error, why:
				# print 'Can\'t stat', b_path, ':', why[1]
				ok = 0
			#
			if ok:
				a_type = S_IFMT(a_stat[ST_MODE])
				b_type = S_IFMT(b_stat[ST_MODE])
				if a_type <> b_type:
					dd.common_funny.append(x)
				elif S_ISDIR(a_type):
					dd.common_dirs.append(x)
				elif S_ISREG(a_type):
					dd.common_files.append(x)
				else:
					dd.common_funny.append(x)
			else:
				dd.common_funny.append(x)
	#
	def phase3(dd): # Find out differences between common files
		xx = cmpfiles(dd.a, dd.b, dd.common_files)
		dd.same_files, dd.diff_files, dd.funny_files = xx
	#
	def phase4(dd): # Find out differences between common subdirectories
		# A new dircmp object is created for each common subdirectory,
		# these are stored in a dictionary indexed by filename.
		# The hide and ignore properties are inherited from the parent
		dd.subdirs = {}
		for x in dd.common_dirs:
			a_x = path.cat(dd.a, x)
			b_x = path.cat(dd.b, x)
			dd.subdirs[x] = newdd = dircmp().new(a_x, b_x)
			newdd.hide = dd.hide
			newdd.ignore = dd.ignore
			newdd.run()
	#
	def phase4_closure(dd): # Recursively call phase4() on subdirectories
		dd.phase4()
		for x in dd.subdirs.keys():
			dd.subdirs[x].phase4_closure()
	#
	def report(dd): # Print a report on the differences between a and b
		# Assume that phases 1 to 3 have been executed
		# Output format is purposely lousy
		print 'diff', dd.a, dd.b
		if dd.a_only:
			print 'Only in', dd.a, ':', dd.a_only
		if dd.b_only:
			print 'Only in', dd.b, ':', dd.b_only
		if dd.same_files:
			print 'Identical files :', dd.same_files
		if dd.diff_files:
			print 'Differing files :', dd.diff_files
		if dd.funny_files:
			print 'Trouble with common files :', dd.funny_files
		if dd.common_dirs:
			print 'Common subdirectories :', dd.common_dirs
		if dd.common_funny:
			print 'Common funny cases :', dd.common_funny
	#
	def report_closure(dd): # Print reports on dd and on subdirs
		# If phase 4 hasn't been done, no subdir reports are printed
		dd.report()
		try:
			x = dd.subdirs
		except NameError:
			return # No subdirectories computed
		for x in dd.subdirs.keys():
			print
			dd.subdirs[x].report_closure()
	#
	def report_phase4_closure(dd): # Report and do phase 4 recursively
		dd.report()
		dd.phase4()
		for x in dd.subdirs.keys():
			print
			dd.subdirs[x].report_phase4_closure()


# Compare common files in two directories.
# Return:
#	- files that compare equal
#	- files that compare different
#	- funny cases (can't stat etc.)
#
def cmpfiles(a, b, common):
	res = ([], [], [])
	for x in common:
		res[cmp(path.cat(a, x), path.cat(b, x))].append(x)
	return res


# Compare two files.
# Return:
#	0 for equal
#	1 for different
#	2 for funny cases (can't stat, etc.)
#
def cmp(a, b):
	try:
		if cmpcache.cmp(a, b): return 0
		return 1
	except posix.error:
		return 2


# Remove a list item.
# NB: This modifies the list argument.
#
def remove(list, item):
	for i in range(len(list)):
		if list[i] = item:
			del list[i]
			break


# Return a copy with items that occur in skip removed.
#
def filter(list, skip):
	result = []
	for item in list:
		if item not in skip: result.append(item)
	return result


# Demonstration and testing.
#
def demo():
	import sys
	import getopt
	options, args = getopt.getopt(sys.argv[1:], 'r')
	if len(args) <> 2: raise getopt.error, 'need exactly two args'
	dd = dircmp().new(args[0], args[1])
	dd.run()
	if ('-r', '') in options:
		dd.report_phase4_closure()
	else:
		dd.report()

# demo()
