#
#   DSA.py : Stupid name.  Should really be called qNEW.py or something.
#            Suggestions for a better name would be welcome.
#
# Maintained by A.M. Kuchling (amk@magnet.com)
# Date: 1997/09/03
# 
# Distribute and use freely; there are no restrictions on further 
# dissemination and usage except those imposed by the laws of your 
# country of residence.
# 

# TODO : 
#   Change the name
#   Add more comments and docstrings
#   Write documentation
#   Add better RNG (?)
     
import types, md5

error = 'DSA module error'

def RandomNumber(N, randfunc):
    "Get an N-bit random number"
    str=randfunc(N/8)
    char=ord(randfunc(1))>>(8-(N%8))
    return Str2Int(chr(char)+str)
    
def Int2Str(n):
    "Convert an integer to a string form"
    s=''
    while n>0:
        s=chr(n & 255)+s
        n=n>>8
    return s

def Str2Int(s):
    "Convert a string to a long integer"
    if type(s)!=types.StringType: return s   # Integers will be left alone
    return reduce(lambda x,y : x*256+ord(y), s, 0L)
    

def getPrime(N, randfunc):
    "Find a prime number measuring N bits"
    number=RandomNumber(N, randfunc) | 1
    while (not isPrime(number)):
        number=number+2
    return number

sieve=[2,3,5,7,11,13,17,19,23,29,31,37,41]
def isPrime(N):
    """Test if a number N is prime, using a simple sieve check, 
    followed by a more elaborate Rabin-Miller test."""
    for i in sieve:
        if (N % i)==0: return 0
    N1=N - 1L ; n=1L
    while (n<N): n=n<<1L # Compute number of bits in N
    for j in sieve:
        a=long(j) ; d=1L ; t=n
        while (t):  # Iterate over the bits in N1
            x=(d*d) % N
            if x==1L and d!=1L and d!=N1: return 0  # Square root of 1 found
            if N1 & t: d=(x*a) % N
            else: d=x
            t=t>>1L
        if d!=1L: return 0
        return 1

class DSAobj:
    def size(self):
	"Return the max. number of bits that can be handled by this key"
        bits, power = 0,1L
	while (power<self.p): bits, power = bits+1, power<<1
	return bits-1
	
    def hasprivate(self):
	"""Return a Boolean denoting whether the object contains private components"""
	if hasattr(self, 'x'): return 1
	else: return 0

    def cansign(self):
	return self.hasprivate()
    def canencrypt(self):
	return 0
	
    def publickey(self):
	new=DSAobj()
	for i in 'pqgy': setattr(new, i, getattr(self, i))
        return new

    def _sign(self, M, K):
	if (self.q<=K):
	    raise error, 'K is greater than q'
        r=pow(self.g, K, self.p) % self.q
        s=(K- (r*M*self.x % self.q)) % self.q
        return (r,s)
    def _verify(self, M, sig):
        r, s = sig
	if r<=0 or r>=self.q or s<=0 or s>=self.q: return 0
        v1=pow(self.g, s, self.p)
	v2=pow(self.y, M*r, self.p)
	v=((v1*v2) % self.p)
	v=v % self.q
        if v==r: return 1
        return 0

    def sign(self, M, K):
	if (not self.hasprivate()):
	    raise error, 'Private key not available in this object'
	if type(M)==types.StringType: M=Str2Int(M)
	if type(K)==types.StringType: K=Str2Int(K)
	return self._sign(M, K)
    def verify(self, M, signature):
	if type(M)==types.StringType: M=Str2Int(M)
	return self._verify(M, signature)
    validate=verify

    def generate(self, L, randfunc, progress_func=None):
	"""Generate a private key with L bits"""
	HASHBITS=128   # Number of bits in the hashing algorithm used 
	               # (128 for MD5; change to 160 for SHA)

	if L<512: raise error, 'Key length <512 bits'
	# Generate string S and prime q
	if progress_func: apply(progress_func, ('p,q\n',))
	while (1):
	    self.q = getPrime(160, randfunc)
            S = Int2Str(self.q)
	    n=(L-1)/HASHBITS
	    C, N, V = 0, 2, {}
#	    b=(self.q >> 5) & 15
            b= (L-1) % HASHBITS
	    powb=pow(long(2), b)
	    powL1=pow(long(2), L-1)
	    while C<4096:
		for k in range(0, n+1):
		    V[k]=Str2Int(md5.new(S+str(N)+str(k)).digest())
		W=V[n] % powb
		for k in range(n-1, -1, -1): 
                    W=(W<< long(HASHBITS) )+V[k]
		X=W+powL1
		p=X-(X%(2*self.q)-1)
		if powL1<=p and isPrime(p): break
		C, N = C+1, N+n+1
	    if C<4096: break
	    if progress_func: apply(progress_func, ('4096 multiples failed\n',) )
	self.p = p
	power=(p-1)/self.q
	if progress_func: apply(progress_func, ('h,g\n',))
	while (1):
	    h=Str2Int(randfunc(L)) % (p-1)
	    g=pow(h, power, p)
	    if 1<h<p-1 and g>1: break
	self.g=g
	if progress_func: apply(progress_func, ('x,y\n',))
	while (1):
	    x=Str2Int(randfunc(20))
	    if 0<x<self.q: break
	self.x, self.y=x, pow(g, x, p)
	return self

object=DSAobj

# XXX this random number generation function sucks, since it isn't
# cryptographically strong!  But it'll do for this first release...

def randfunc(N): 
    import os, string
    if string.lower(os.uname()[0])=='linux':
	# On Linux, use /dev/urandom
	f=open('/dev/urandom', 'r')
	return f.read(N)
    else:
	import time
	s=""
	while len(s)<N:
	    rand=md5.new(str(time.time())).digest()
	    s=s+rand
	return s[0:N]

if __name__=='__main__':
    import sys, string
    BITS=512
    if len(sys.argv)>1:
        BITS=string.atoi(sys.argv[1])
    print ' Generating', BITS, 'bit key'
    key=DSAobj()
    key.generate(BITS, randfunc, sys.stdout.write)
    print ' Key data: (the private key is x)'
    for i in 'xygqp': print '\t', i, ':', hex(getattr(key, i))
    plaintext="Hello"

    if key.cansign():
	print ' Signature test'
	print "Plaintext:", plaintext
	K=getPrime(30, randfunc)
	signature=key.sign(plaintext, K)
	print "Signature:", signature
	result=key.verify(plaintext, signature)
	if not result:
	    print " Sig. verification failed when it should have succeeded"
	else: print 'Signature verified'

	# Test on a mangled plaintext
	result=key.verify(plaintext[:-1], signature)
	if result:
	    print " Sig. verification succeeded when it should have failed"

	# Change a single bit in the plaintext
	badtext=plaintext[:-3]+chr( 1 ^ ord(plaintext[-3]) )+plaintext[-3:]
	result=key.verify(badtext, signature)
	if result:
	    print " Sig. verification succeeded when it should have failed"

	print 'Removing private key data'
	pubonly=key.publickey()
	result=pubonly.verify(plaintext, signature)
	if not result:
	    print " Sig. verification failed when it should have succeeded"
        else: 
            print 'Signature verified'
