#! /usr/bin/env python3

# linktree
#
# Make a copy of a directory tree with symbolic links to all files in the
# original tree.
# All symbolic links go to a special symbolic link at the top, so you
# can easily fix things if the original source tree moves.
# See also "mkreal".
#
# usage: mklinks oldtree newtree

import sys, os

LINK = '.LINK' # Name of special symlink at the top.

debug = 0

def main():
    if not 3 <= len(sys.argv) <= 4:
        print('usage:', sys.argv[0], 'oldtree newtree [linkto]')
        return 2
    oldtree, newtree = sys.argv[1], sys.argv[2]
    if len(sys.argv) > 3:
        link = sys.argv[3]
        link_may_fail = 1
    else:
        link = LINK
        link_may_fail = 0
    if not os.path.isdir(oldtree):
        print(oldtree + ': not a directory')
        return 1
    try:
        os.mkdir(newtree, 0o777)
    except OSError as msg:
        print(newtree + ': cannot mkdir:', msg)
        return 1
    linkname = os.path.join(newtree, link)
    try:
        os.symlink(os.path.join(os.pardir, oldtree), linkname)
    except OSError as msg:
        if not link_may_fail:
            print(linkname + ': cannot symlink:', msg)
            return 1
        else:
            print(linkname + ': warning: cannot symlink:', msg)
    linknames(oldtree, newtree, link)
    return 0

def linknames(old, new, link):
    if debug: print('linknames', (old, new, link))
    try:
        names = os.listdir(old)
    except OSError as msg:
        print(old + ': warning: cannot listdir:', msg)
        return
    for name in names:
        if name not in (os.curdir, os.pardir):
            oldname = os.path.join(old, name)
            linkname = os.path.join(link, name)
            newname = os.path.join(new, name)
            if debug > 1: print(oldname, newname, linkname)
            if os.path.isdir(oldname) and \
               not os.path.islink(oldname):
                try:
                    os.mkdir(newname, 0o777)
                    ok = 1
                except:
                    print(newname + \
                          ': warning: cannot mkdir:', msg)
                    ok = 0
                if ok:
                    linkname = os.path.join(os.pardir,
                                            linkname)
                    linknames(oldname, newname, linkname)
            else:
                os.symlink(linkname, newname)

if __name__ == '__main__':
    sys.exit(main())
