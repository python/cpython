# Tests StringIO and cStringIO

import string

def do_test(module):
    s = (string.letters+'\n')*5
    f = module.StringIO(s)
    print f.read(10)
    print f.readline()
    print len(f.readlines(60))

# Don't bother testing cStringIO without
import StringIO, cStringIO
do_test(StringIO)
do_test(cStringIO)
