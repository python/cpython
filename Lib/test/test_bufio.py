from test.test_support import verify, TestFailed, TESTFN

# Simple test to ensure that optimizations in fileobject.c deliver
# the expected results.  For best testing, run this under a debug-build
# Python too (to exercise asserts in the C code).

# Repeat string 'pattern' as often as needed to reach total length
# 'length'.  Then call try_one with that string, a string one larger
# than that, and a string one smaller than that.  The main driver
# feeds this all small sizes and various powers of 2, so we exercise
# all likely stdio buffer sizes, and "off by one" errors on both
# sides.
def drive_one(pattern, length):
    q, r = divmod(length, len(pattern))
    teststring = pattern * q + pattern[:r]
    verify(len(teststring) == length)
    try_one(teststring)
    try_one(teststring + "x")
    try_one(teststring[:-1])

# Write s + "\n" + s to file, then open it and ensure that successive
# .readline()s deliver what we wrote.
def try_one(s):
    # Since C doesn't guarantee we can write/read arbitrary bytes in text
    # files, use binary mode.
    f = open(TESTFN, "wb")
    # write once with \n and once without
    f.write(s)
    f.write("\n")
    f.write(s)
    f.close()
    f = open(TESTFN, "rb")
    line = f.readline()
    if line != s + "\n":
        raise TestFailed("Expected %r got %r" % (s + "\n", line))
    line = f.readline()
    if line != s:
        raise TestFailed("Expected %r got %r" % (s, line))
    line = f.readline()
    if line:
        raise TestFailed("Expected EOF but got %r" % line)
    f.close()

# A pattern with prime length, to avoid simple relationships with
# stdio buffer sizes.
primepat = "1234567890\00\01\02\03\04\05\06"

nullpat = "\0" * 1000

try:
    for size in range(1, 257) + [512, 1000, 1024, 2048, 4096, 8192, 10000,
                      16384, 32768, 65536, 1000000]:
        drive_one(primepat, size)
        drive_one(nullpat, size)
finally:
    try:
        import os
        os.unlink(TESTFN)
    except:
        pass
