# fixgusidir - Modify filenames in the CWGUSI source tree, so
# that it can be put under CVS. Needed because there are files with slashes
# in their name, which CVS does not approve of.
#
# Usage:
# - On importing gusi in cvs:
#   - Run the script after unpacking the gusi distribution. This creates
#     _s_ files for all / files.
#   - Remove all / files with "find file" or some such.
#   - import the tree
# - On checking out gusi:
#   - After checkout, run the script to create / files for all _s_ files.
# - After modifying stuff, or later checkouts:
#   - Run the script. Conflicts between / and _s_ files will be reported.
#   - Fix the problems by removing the outdated / or _s_ file.
#   - Run the script again. Possibly do a cvs checkin.
import macfs
import os
import sys
import re

# Substitution for slashes in filenames
SUBST = '_s_'
# Program to find those substitutions
SUBSTPROG = re.compile(SUBST)

def main():
	if len(sys.argv) > 1:
		gusidir = sys.argv[1]
	else:
		fss, ok = macfs.GetDirectory("CWGUSI source folder?")
		if not ok: sys.exit(0)
		gusidir = fss.as_pathname()
	fixgusifolder(gusidir)
	sys.exit(1)
			
def fixgusifolder(gusidir):
	"""Synchronize files with / in their name with their _s_ counterparts"""
	os.path.walk(gusidir, gusiwalk, None)
	
def gusiwalk(dummy, top, names):
	"""Helper for fixgusifolder: convert a single directory full of files"""
	# First remember names with slashes and with our slash-substitution
	macnames = []
	codenames = []
	for name in names:
		if '/' in name:
			macnames.append(name)
		if SUBSTPROG.search(name):
			codenames.append(name)
	# Next, check whether we need to copy any slash-files to subst-files
	for name in macnames:
		if os.path.isdir(name):
			print '** Folder with slash in name cannot be handled!'
			sys.exit(1)
		othername = mac2codename(name)
		if len(othername) > 31:
			print '** Converted filename too long:', othername
			sys.exit(1)
		if othername in codenames:
			codenames.remove(othername)
		sync(os.path.join(top, name), os.path.join(top, othername))
	# Now codenames contains only files that have no / equivalent
	for name in codenames:
		othername = code2macname(name)
		sync(os.path.join(top, name), os.path.join(top, othername))
		
def mac2codename(name):
	return re.sub('/', SUBST, name)
	
def code2macname(name):
	return re.sub(SUBST, '/', name)
	
def sync(old, new):
	if os.path.exists(new):
		# Both exist. Check that they are indentical
		d1 = open(old, 'rb').read()
		d2 = open(new, 'rb').read()
		if d1 == d2:
			print '-- OK         ', old
			return
		print '** OUT-OF-SYNC', old
	fp = open(new, 'wb')
	fp.write(open(old, 'rb').read())
	print '-- COPIED     ', old
	
if __name__ == '__main__':
	main()
	
