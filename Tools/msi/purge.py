# Purges the Fastly cache for Windows download files
#
# Usage:
#   py -3 purge.py 3.5.1rc1
#

__author__ = 'Steve Dower <steve.dower@python.org>'
__version__ = '1.0.0'

import re
import sys

from urllib.request import *

VERSION_RE = re.compile(r'(\d+\.\d+\.\d+)([A-Za-z_]+\d+)?$')

try:
    m = VERSION_RE.match(sys.argv[1])
    if not m:
        print('Invalid version:', sys.argv[1])
        print('Expected something like "3.5.1rc1"')
        sys.exit(1)
except LookupError:
    print('Missing version argument. Expected something like "3.5.1rc1"')
    sys.exit(1)

URL = "https://www.python.org/ftp/python/{}/".format(m.group(1))
REL = m.group(2) or ''

FILES = [
    "core.msi",
    "core_d.msi",
    "core_pdb.msi",
    "dev.msi",
    "dev_d.msi",
    "doc.msi",
    "exe.msi",
    "exe_d.msi",
    "exe_pdb.msi",
    "launcher.msi",
    "lib.msi",
    "lib_d.msi",
    "lib_pdb.msi",
    "path.msi",
    "pip.msi",
    "tcltk.msi",
    "tcltk_d.msi",
    "tcltk_pdb.msi",
    "test.msi",
    "test_d.msi",
    "test_pdb.msi",
    "tools.msi",
    "ucrt.msi",
    "Windows6.0-KB2999226-x64.msu",
    "Windows6.0-KB2999226-x86.msu",
    "Windows6.1-KB2999226-x64.msu",
    "Windows6.1-KB2999226-x86.msu",
    "Windows8.1-KB2999226-x64.msu",
    "Windows8.1-KB2999226-x86.msu",
    "Windows8-RT-KB2999226-x64.msu",
    "Windows8-RT-KB2999226-x86.msu",
]
PATHS = [
    "python-{}.exe".format(m.group(0)),
    "python-{}-webinstall.exe".format(m.group(0)),
    "python-{}-amd64.exe".format(m.group(0)),
    "python-{}-amd64-webinstall.exe".format(m.group(0)),
    "python-{}-embed-amd64.zip".format(m.group(0)),
    "python-{}-embed-win32.zip".format(m.group(0)),
] + ["win32{}/{}".format(REL, f) for f in FILES] + ["amd64{}/{}".format(REL, f) for f in FILES]
PATHS = PATHS + [p + ".asc" for p in PATHS]

print('Purged:')
for n in PATHS:
    u = URL + n
    with urlopen(Request(u, method='PURGE', headers={'Fastly-Soft-Purge': 1})) as r:
        r.read()
    print('  ', u)
