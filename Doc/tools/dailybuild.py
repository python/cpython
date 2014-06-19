#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Runs the daily build of the Python docs on dinsdale.python.org.
#
# Usages:
#
#   dailybuild.py [-q]
#
# without any arguments builds docs for all branches configured in the global
# BRANCHES value. -q selects "quick build", which means to build only HTML.
#
#   dailybuild.py [-q] [-d] <checkout> <target>
#
# builds one version, where <checkout> is an SVN checkout directory of the
# Python branch to build docs for, and <target> is the directory where the
# result should be placed.  If -d is given, the docs are built even if the
# branch is in development mode (i.e. version contains a, b or c).
#
# This script is not run from the checkout, so if you want to change how the
# daily build is run, you must replace it on dinsdale.  This is necessary, for
# example, after the release of a new minor version.
#
# 03/2010, Georg Brandl

import os
import sys
import getopt


BUILDROOT = '/home/gbrandl/docbuild'
SPHINXBUILD = os.path.join(BUILDROOT, 'sphinx-env/bin/sphinx-build')
WWWROOT = '/data/ftp.python.org/pub/docs.python.org'

BRANCHES = [
    # checkout, target, isdev
    (BUILDROOT + '/python34', WWWROOT + '/3.4', False),
    (BUILDROOT + '/python35', WWWROOT + '/3.5', True),
    (BUILDROOT + '/python27', WWWROOT + '/2.7', False),
]


def _files_changed(old, new):
    with open(old, 'rb') as fp1, open(new, 'rb') as fp2:
        st1 = os.fstat(fp1.fileno())
        st2 = os.fstat(fp2.fileno())
        if st1.st_size != st2.st_size:
            return False
        if st1.st_mtime >= st2.st_mtime:
            return True
        while True:
            one = fp1.read(4096)
            two = fp2.read(4096)
            if one != two:
                return False
            if one == '':
                break
    return True

def build_one(checkout, target, isdev, quick):
    print 'Doc autobuild started in %s' % checkout
    os.chdir(checkout)
    print 'Running hg pull --update'
    os.system('hg pull --update')
    print 'Running make autobuild'
    maketarget = 'autobuild-' + ('html' if quick else
                                 ('dev' if isdev else 'stable'))
    if os.WEXITSTATUS(os.system('cd Doc; make SPHINXBUILD=%s %s' % (SPHINXBUILD, maketarget))) == 2:
        print '*' * 80
        return
    print('Computing changed files')
    changed = []
    for dirpath, dirnames, filenames in os.walk('Doc/build/html/'):
        dir_rel = dirpath[len('Doc/build/html/'):]
        for fn in filenames:
            local_path = os.path.join(dirpath, fn)
            rel_path = os.path.join(dir_rel, fn)
            target_path = os.path.join(target, rel_path)
            if (os.path.exists(target_path) and
                not _files_changed(target_path, local_path)):
                changed.append(rel_path)
    print 'Copying HTML files to %s' % target
    os.system('cp -a Doc/build/html/* %s' % target)
    if not quick:
        print 'Copying dist files'
        os.system('mkdir -p %s/archives' % target)
        os.system('cp -a Doc/dist/* %s/archives' % target)
        changed.append('archives/')
        for fn in os.listdir(os.path.join(target, 'archives')):
            changed.append('archives/' + fn)
    print '%s files changed' % len(changed)
    if changed:
        target_ino = os.stat(target).st_ino
        targets_dir = os.path.dirname(target)
        prefixes = []
        for fn in os.listdir(targets_dir):
            if os.stat(os.path.join(targets_dir, fn)).st_ino == target_ino:
                prefixes.append(fn)
        to_purge = []
        for prefix in prefixes:
            to_purge.extend(prefix + "/" + p for p in changed)
        purge_cmd = 'curl -X PURGE "https://docs.python.org/{%s}"' % ','.join(to_purge)
        print("Running CDN purge")
        os.system(purge_cmd)
    print 'Finished'
    print '=' * 80

def usage():
    print 'Usage:'
    print '  %s' % sys.argv[0]
    print 'or'
    print '  %s [-d] <checkout> <target>' % sys.argv[0]
    sys.exit(1)


if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'dq')
    except getopt.error:
        usage()
    quick = devel = False
    for opt, _ in opts:
        if opt == '-q':
            quick = True
        if opt == '-d':
            devel = True
    if devel and not args:
        usage()
    if args:
        if len(args) != 2:
            usage()
        build_one(os.path.abspath(args[0]), os.path.abspath(args[1]), devel, quick)
    else:
        for checkout, dest, devel in BRANCHES:
            build_one(checkout, dest, devel, quick)
