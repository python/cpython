import sys
import os
import shutil
import getopt

def buildappbundle(executable, output=None, copyfunc=None, creator=None,
		plist=None, nib=None, resources=None):
	if not output:
		output = os.path.split(executable)[1] + '.app'
	if not copyfunc:
		copyfunc = shutil.copy2
	if not creator:
		creator='????'
	if not resources:
		resources = []
	if nib:
		resources = resources + [nib]
	#
	# Create the main directory structure
	#
	if not os.path.isdir(output):
		os.mkdir(output)
	contents = os.path.join(output, 'Contents')
	if not os.path.isdir(contents):
		os.mkdir(contents)
	macos = os.path.join(contents, 'MacOS')
	if not os.path.isdir(macos):
		os.mkdir(macos)
	#
	# Create the executable
	#
	shortname = os.path.split(executable)[1]
	execname = os.path.join(macos, shortname)
	try:
		os.remove(execname)
	except OSError:
		pass
	copyfunc(executable, execname)
	#
	# Create the PkgInfo file
	#
	pkginfo = os.path.join(contents, 'PkgInfo')
	open(pkginfo, 'wb').write('APPL'+creator)
	if plist:
		# A plist file is specified. Read it.
		plistdata = open(plist).read()
	else:
		#
		# If we have a main NIB we create the extra Cocoa specific info for the plist file
		#
		if not nib:
			nibname = ""
		else:
			nibname, ext = os.path.splitext(os.path.split(nib)[1])
			if ext == '.lproj':
				# Special case: if the main nib is a .lproj we assum a directory
				# and use the first nib from there
				files = os.listdir(nib)
				for f in files:
					if f[-4:] == '.nib':
						nibname = os.path.split(f)[1][:-4]
						break
				else:
					nibname = ""
		if nibname:
			cocoainfo = """
			<key>NSMainNibFile</key>
			<string>%s</string>
			<key>NSPrincipalClass</key>
			<string>NSApplication</string>""" % nibname
		else:
			cocoainfo = ""
		plistdata = \
"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
        <key>CFBundleDevelopmentRegion</key>
        <string>English</string>
        <key>CFBundleExecutable</key>
        <string>%s</string>
        <key>CFBundleInfoDictionaryVersion</key>
        <string>6.0</string>
        <key>CFBundlePackageType</key>
        <string>APPL</string>
        <key>CFBundleSignature</key>
        <string>%s</string>
        <key>CFBundleVersion</key>
        <string>0.1</string>
        %s
</dict>
</plist>
""" % (shortname, creator, cocoainfo)
	#
	# Next, we create the plist file
	#
	infoplist = os.path.join(contents, 'Info.plist')
	open(infoplist, 'w').write(plistdata)
	#
	# Finally, if there are nibs or other resources to copy we do so.
	#
	if resources:
		resdir = os.path.join(contents, 'Resources')
		if not os.path.isdir(resdir):
			os.mkdir(resdir)
		for src in resources:
			dst = os.path.join(resdir, os.path.split(src)[1])
			if os.path.isdir(src):
				shutil.copytree(src, dst)
			else:
				shutil.copy2(src, dst)
				
def usage():
	print "buildappbundle creates an application bundle"
	print "Usage:"
	print "  buildappbundle [options] executable"
	print "Options:"
	print "  --output o    Output file; default executable with .app appended, short -o"
	print "  --link        Symlink the executable (default: copy), short -l"
	print "  --plist file  Plist file (default: generate one), short -p"
	print "  --nib file    Main nib file or lproj folder for Cocoa program, short -n"
	print "  --resource r  Extra resource file to be copied to Resources, short -r"
	print "  --creator c   4-char creator code (default: ????), short -c"
	print "  --help        This message, short -?"
	sys.exit(1)

def main():
	output=None
	copyfunc=None
	creator=None
	plist=None
	nib=None
	resources=[]
	SHORTOPTS = "o:ln:r:p:c:?"
	LONGOPTS=("output=", "link", "nib=", "resource=", "plist=", "creator=", "help")
	try:
		options, args = getopt.getopt(sys.argv[1:], SHORTOPTS, LONGOPTS)
	except getopt.error:
		usage()
	if len(args) != 1:
		usage()
	for opt, arg in options:
		if opt in ('-o', '--output'):
			output = arg
		elif opt in ('-l', '--link'):
			copyfunc = os.symlink
		elif opt in ('-n', '--nib'):
			nib = arg
		elif opt in ('-r', '--resource'):
			resources.append(arg)
		elif opt in ('-c', '--creator'):
			creator = arg
		elif opt in ('-p', '--plist'):
			plist = arg
		elif opt in ('-?', '--help'):
			usage()
	buildappbundle(args[0], output=output, copyfunc=copyfunc, creator=creator,
		plist=plist, resources=resources)
	
if __name__ == '__main__':
	main()
	