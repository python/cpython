"""fixversions - Fix version numbers in .plist files to match current
python version and date"""

import sys
import os
import time
import plistlib

SHORTVERSION = "%d.%d" % (sys.version_info[0], sys.version_info[1])
if sys.version_info[2]:
    SHORTVERSION = SHORTVERSION + ".%d" % sys.version_info[2]
if sys.version_info[3] != 'final':
    SHORTVERSION = SHORTVERSION + "%s%d" % (sys.version_info[3], sys.version_info[4])

COPYRIGHT = "(c) %d Python Software Foundation." % time.gmtime()[0]

LONGVERSION = SHORTVERSION + ", " + COPYRIGHT

def fix(file):
    plist = plistlib.Plist.fromFile(file)
    changed = False
    if plist.has_key("CFBundleGetInfoString") and \
                    plist["CFBundleGetInfoString"] != LONGVERSION:
        plist["CFBundleGetInfoString"] = LONGVERSION
        changed = True
    if plist.has_key("CFBundleLongVersionString") and \
                    plist["CFBundleLongVersionString"] != LONGVERSION:
        plist["CFBundleLongVersionString"] = LONGVERSION
        changed = True
    if plist.has_key("NSHumanReadableCopyright") and \
                    plist["NSHumanReadableCopyright"] != COPYRIGHT:
        plist["NSHumanReadableCopyright"] = COPYRIGHT
        changed = True
    if plist.has_key("CFBundleVersion") and \
                    plist["CFBundleVersion"] != SHORTVERSION:
        plist["CFBundleVersion"] = SHORTVERSION
        changed = True
    if plist.has_key("CFBundleShortVersionString") and \
                    plist["CFBundleShortVersionString"] != SHORTVERSION:
        plist["CFBundleShortVersionString"] = SHORTVERSION
        changed = True
    if changed:
        os.rename(file, file + '~')
        plist.write(file)

def main():
    if len(sys.argv) < 2:
        print "Usage: %s plistfile ..." % sys.argv[0]
        print "or: %s -a      fix standard Python plist files"
        sys.exit(1)
    if sys.argv[1] == "-a":
        files = [
                "../OSXResources/app/Info.plist",
                "../OSXResources/framework/version.plist",
                "../Tools/IDE/PackageManager.plist",
                "../Tools/IDE/PythonIDE.plist",
                "../scripts/BuildApplet.plist"
        ]
        if not os.path.exists(files[0]):
            print "%s -a must be run from Mac/OSX directory"
            sys.exit(1)
    else:
        files = sys.argv[1:]
    for file in files:
        fix(file)
    sys.exit(0)

if __name__ == "__main__":
    main()
