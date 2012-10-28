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
WWWROOT = '/data/ftp.python.org/pub/docs.python.org'

BRANCHES = [
    # checkout, target, isdev
    (BUILDROOT + '/python33', WWWROOT + '/3.3', False),
    (BUILDROOT + '/python34', WWWROOT + '/3.4', True),
    (BUILDROOT + '/python27', WWWROOT + '/2.7', False),
]


def build_one(checkout, target, isdev, quick):
    print 'Doc autobuild started in %s' % checkout
    os.chdir(checkout)
    print 'Running hg pull --update'
    os.system('/usr/local/bin/hg pull --update')
    print 'Running make autobuild'
    maketarget = 'autobuild-' + ('html' if quick else
                                 ('dev' if isdev else 'stable'))
    if os.WEXITSTATUS(os.system('cd Doc; make %s' % maketarget)) == 2:
        print '*' * 80
        return
    print 'Copying HTML files to %s' % target
    os.system('cp -a Doc/build/html/* %s' % target)
    if not quick:
        print 'Copying dist files'
        os.system('mkdir -p %s/archives' % target)
        os.system('cp -a Doc/dist/* %s/archives' % target)
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
        build_one(args[0], args[1], devel, quick)
    else:
        for checkout, dest, devel in BRANCHES:
            build_one(checkout, dest, devel, quick)
