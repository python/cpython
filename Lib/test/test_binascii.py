"""Test the binascii C module."""

from test_support import verify, verbose
import binascii

# Show module doc string
print binascii.__doc__

# Show module exceptions
print binascii.Error
print binascii.Incomplete

# Check presence and display doc strings of all functions
funcs = []
for suffix in "base64", "hqx", "uu":
    prefixes = ["a2b_", "b2a_"]
    if suffix == "hqx":
        prefixes.extend(["crc_", "rlecode_", "rledecode_"])
    for prefix in prefixes:
        name = prefix + suffix
        funcs.append(getattr(binascii, name))
for func in funcs:
    print "%-15s: %s" % (func.__name__, func.__doc__)

# Create binary test data
testdata = "The quick brown fox jumps over the lazy dog.\r\n"
for i in range(256):
    # Be slow so we don't depend on other modules
    testdata = testdata + chr(i)
testdata = testdata + "\r\nHello world.\n"

# Test base64 with valid data
print "base64 test"
MAX_BASE64 = 57
lines = []
for i in range(0, len(testdata), MAX_BASE64):
    b = testdata[i:i+MAX_BASE64]
    a = binascii.b2a_base64(b)
    lines.append(a)
    print a,
res = ""
for line in lines:
    b = binascii.a2b_base64(line)
    res = res + b
verify(res == testdata)

# Test base64 with random invalid characters sprinkled throughout
# (This requires a new version of binascii.)
fillers = ""
valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/"
for i in range(256):
    c = chr(i)
    if c not in valid:
        fillers = fillers + c
def addnoise(line):
    noise = fillers
    ratio = len(line) // len(noise)
    res = ""
    while line and noise:
        if len(line) // len(noise) > ratio:
            c, line = line[0], line[1:]
        else:
            c, noise = noise[0], noise[1:]
        res = res + c
    return res + noise + line
res = ""
for line in map(addnoise, lines):
    b = binascii.a2b_base64(line)
    res = res + b
verify(res == testdata)

# Test base64 with just invalid characters, which should return
# empty strings.
verify(binascii.a2b_base64(fillers) == '')

# Test uu
print "uu test"
MAX_UU = 45
lines = []
for i in range(0, len(testdata), MAX_UU):
    b = testdata[i:i+MAX_UU]
    a = binascii.b2a_uu(b)
    lines.append(a)
    print a,
res = ""
for line in lines:
    b = binascii.a2b_uu(line)
    res = res + b
verify(res == testdata)

# Test crc32()
crc = binascii.crc32("Test the CRC-32 of")
crc = binascii.crc32(" this string.", crc)
if crc != 1571220330:
    print "binascii.crc32() failed."

# The hqx test is in test_binhex.py

# test hexlification
s = '{s\005\000\000\000worldi\002\000\000\000s\005\000\000\000helloi\001\000\000\0000'
t = binascii.b2a_hex(s)
u = binascii.a2b_hex(t)
if s != u:
    print 'binascii hexlification failed'
try:
    binascii.a2b_hex(t[:-1])
except TypeError:
    pass
else:
    print 'expected TypeError not raised'
try:
    binascii.a2b_hex(t[:-1] + 'q')
except TypeError:
    pass
else:
    print 'expected TypeError not raised'

# Verify the treatment of Unicode strings
verify(binascii.hexlify(u'a') == '61', "hexlify failed for Unicode")
