#! /usr/bin/env python
#  (Force the script to use the latest build.)
#
#  test_parser.py

import parser, traceback

_numFailed = 0

def testChunk(t, fileName):
    global _numFailed
    print '----', fileName,
    try:
        st = parser.suite(t)
        tup = parser.st2tuple(st)
        # this discards the first ST; a huge memory savings when running
        # against a large source file like Tkinter.py.
        st = None
        new = parser.tuple2st(tup)
    except parser.ParserError, err:
        print
        print 'parser module raised exception on input file', fileName + ':'
        traceback.print_exc()
        _numFailed = _numFailed + 1
    else:
        if tup != parser.st2tuple(new):
            print
            print 'parser module failed on input file', fileName
            _numFailed = _numFailed + 1
        else:
            print 'o.k.'

def testFile(fileName):
    t = open(fileName).read()
    testChunk(t, fileName)

def test():
    import sys
    args = sys.argv[1:]
    if not args:
        import glob
        args = glob.glob("*.py")
        args.sort()
    map(testFile, args)
    sys.exit(_numFailed != 0)

if __name__ == '__main__':
    test()
