# Testing sha module (NIST's Secure Hash Algorithm)

import sha

# use the three examples from Federal Information Processing Standards
# Publication 180-1, Secure Hash Standard,  1995 April 17
# http://www.itl.nist.gov/div897/pubs/fip180-1.htm

s = [''] * 3
d = [''] * 3

s[0] = 'abc'
d[0] = 'a9993e364706816aba3e25717850c26c9cd0d89d'

s[1] = 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'
d[1] = '84983e441c3bd26ebaae4aa1f95129e5e54670f1'

s[2] = 'a' * 1000000
d[2] = '34aa973cd4c4daa4f61eeb2bdbad27316534016f'

for i in range(3):
    test = sha.new(s[i]).hexdigest()
    if test == d[i]:
        print "test %d ok" % i
    else:
        print "test %d failed" % i
        print "expected", d[i]
        print "computed", test
