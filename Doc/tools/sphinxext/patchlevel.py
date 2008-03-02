# -*- coding: utf-8 -*-
"""
    patchlevel.py
    ~~~~~~~~~~~~~

    Extract version info from Include/patchlevel.h.
    Adapted from Doc/tools/getversioninfo.

    :copyright: 2007-2008 by Georg Brandl.
    :license: Python license.
"""

import os
import re
import sys

def get_header_version_info(srcdir):
    patchlevel_h = os.path.join(srcdir, '..', 'Include', 'patchlevel.h')

    # This won't pick out all #defines, but it will pick up the ones we
    # care about.
    rx = re.compile(r'\s*#define\s+([a-zA-Z][a-zA-Z_0-9]*)\s+([a-zA-Z_0-9]+)')

    d = {}
    f = open(patchlevel_h)
    try:
        for line in f:
            m = rx.match(line)
            if m is not None:
                name, value = m.group(1, 2)
                d[name] = value
    finally:
        f.close()

    release = version = '%s.%s' % (d['PY_MAJOR_VERSION'], d['PY_MINOR_VERSION'])
    micro = int(d['PY_MICRO_VERSION'])
    if micro != 0:
        release += '.' + str(micro)

    level = d['PY_RELEASE_LEVEL']
    suffixes = {
        'PY_RELEASE_LEVEL_ALPHA': 'a',
        'PY_RELEASE_LEVEL_BETA':  'b',
        'PY_RELEASE_LEVEL_GAMMA': 'c',
        }
    if level != 'PY_RELEASE_LEVEL_FINAL':
        release += suffixes[level] + str(int(d['PY_RELEASE_SERIAL']))
    return version, release


def get_sys_version_info():
    major, minor, micro, level, serial = sys.version_info
    release = version = '%s.%s' % (major, minor)
    if micro:
        release += '.%s' % micro
    if level != 'final':
        release += '%s%s' % (level[0], serial)
    return version, release


def get_version_info():
    try:
        return get_header_version_info('.')
    except (IOError, OSError):
        version, release = get_sys_version_info()
        print >>sys.stderr, 'Can\'t get version info from Include/patchlevel.h, ' \
              'using version of this interpreter (%s).' % release
        return version, release

if __name__ == '__main__':
    print get_header_version_info('.')[1]
