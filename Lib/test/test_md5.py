# Testing md5 module

import string
from md5 import md5

def hexstr(s):
	h = string.hexdigits
	r = ''
	for c in s:
		i = ord(c)
		r = r + h[(i >> 4) & 0xF] + h[i & 0xF]
	return r

def md5test(s):
	return 'MD5 ("' + s + '") = ' + hexstr(md5(s).digest())

print 'MD5 test suite:'
print md5test('')
print md5test('a')
print md5test('abc')
print md5test('message digest')
print md5test('abcdefghijklmnopqrstuvwxyz')
print md5test('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789')
print md5test('12345678901234567890123456789012345678901234567890123456789012345678901234567890')

# hexdigest is new with Python 2.0
m = md5('testing the hexdigest method')
h = m.hexdigest()
if hexstr(m.digest()) <> h:
	print 'hexdigest() failed'
