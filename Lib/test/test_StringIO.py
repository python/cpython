# Tests StringIO and cStringIO

def do_test(module):
    s = ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"+'\n')*5
    f = module.StringIO(s)
    print f.read(10)
    print f.readline()
    print len(f.readlines(60))

    f = module.StringIO()
    f.write('abcdef')
    f.seek(3)
    f.write('uvwxyz')
    f.write('!')
    print `f.getvalue()`
    f.close()
    f = module.StringIO()
    f.write(s)
    f.seek(10)
    f.truncate()
    print `f.getvalue()`
    f.seek(0)
    f.truncate(5)
    print `f.getvalue()`
    f.close()
    try:
        f.write("frobnitz")
    except ValueError, e:
        print "Caught expected ValueError writing to closed StringIO:"
        print e
    else:
        print "Failed to catch ValueError writing to closed StringIO."

# Don't bother testing cStringIO without
import StringIO, cStringIO
do_test(StringIO)
do_test(cStringIO)
