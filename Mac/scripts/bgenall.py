# bgenall - Generate all bgen-generated modules
#
import sys
import os
import string

def bgenone(dirname, shortname):
    os.chdir(dirname)
    print '%s:'%shortname
    # Sigh, we don't want to lose CVS history, so two
    # modules have funny names:
    if shortname == 'carbonevt':
        modulename = 'CarbonEvtscan'
    elif shortname == 'ibcarbon':
        modulename = 'IBCarbonscan'
    else:
        modulename = shortname + 'scan'
    try:
        m = __import__(modulename)
    except:
        print "Error:", shortname, sys.exc_info()[1]
        return 0
    try:
        m.main()
    except:
        print "Error:", shortname, sys.exc_info()[1]
        return 0
    return 1

def main():
    success = []
    failure = []
    sys.path.insert(0, os.curdir)
    if len(sys.argv) > 1:
        srcdir = sys.argv[1]
    else:
        srcdir = os.path.join(os.path.join(sys.prefix, 'Mac'), 'Modules')
    srcdir = os.path.abspath(srcdir)
    contents = os.listdir(srcdir)
    for name in contents:
        moduledir = os.path.join(srcdir, name)
        scanmodule = os.path.join(moduledir, name +'scan.py')
        if os.path.exists(scanmodule):
            if bgenone(moduledir, name):
                success.append(name)
            else:
                failure.append(name)
    print 'Done:', string.join(success, ' ')
    if failure:
        print 'Failed:', string.join(failure, ' ')
        return 0
    return 1

if __name__ == '__main__':
    rv = main()
    sys.exit(not rv)
