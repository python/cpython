# Conversions to/from base64 transport encoding as per RFC-MIME (Dec 1991
# version).

# Parameters set by RFX-XXXX.
MAXLINESIZE = 76 # Excluding the CRLF
INVAR = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
PAD = '='

# Check that I typed that string correctly...
if len(INVAR) <> 64: raise RuntimeError, 'wrong INVAR string!?!?'

# Compute the inverse table, for decode().
inverse = {}
for i in range(64): inverse[INVAR[i]] = i
del i
inverse[PAD] = 0

# Encode a file.
def encode(input, output):
	line = ''
	BUFSIZE = 8192
	leftover = ''
	while 1:
		s = input.read(BUFSIZE)
		if not s: break
		s = leftover + s
		i = 0
		while i+3 <= len(s):
			quad = makequad(s[i:i+3])
			i = i+3
			if len(line) + 4 > MAXLINESIZE:
				output.write(line + '\n')
				line = ''
			line = line + quad
		leftover = s[i:]
	if leftover:
		quad = makeshortquad(leftover)
		if len(line) + 4 > MAXLINESIZE:
			output.write(line + '\n')
			line = ''
		line = line + quad
	if line:
		output.write(line + '\n')

def makequad(s): # Return the quad for a 3 character string
	x = ord(s[0])*0x10000 + ord(s[1])*0x100 + ord(s[2])
	x, c4 = divmod(x, 64)
	x, c3 = divmod(x, 64)
	c1, c2 = divmod(x, 64)
	return INVAR[c1] + INVAR[c2] +INVAR[c3] + INVAR[c4]

def makeshortquad(s): # Return the quad value for a 1 or 2 character string
	n = len(s)
	while len(s) < 3:
		s = s + '\0'
	quad = makequad(s)
	if n == 2:
		quad = quad[:3] + PAD
	elif n == 1:
		quad = quad[:2] + 2*PAD
	return quad

# Decode a file.
def decode(input, output):
	BUFSIZE = 8192
	bits, n, bytes, prev = 0, 0, '', ''
	while 1:
		line = input.readline()
		if not line: break
		for c in line:
			if inverse.has_key(c):
				bits = bits*64 + inverse[c]
				n = n+6
				if n == 24:
					triplet = decodequad(bits)
					if c == PAD:
						if prev == PAD:
							triplet = triplet[:1]
						else:
							triplet = triplet[:2]
					bits, n = 0, 0
					bytes = bytes + triplet
					if len(bytes) > BUFSIZE:
						output.write(bytes[:BUFSIZE])
						bytes = bytes[BUFSIZE:]
				prev = c
	if bytes:
		output.write(bytes)

def decodequad(bits): # Turn 24 bits into 3 characters
	bits, c3 = divmod(bits, 256)
	c1, c2 = divmod(bits, 256)
	return chr(c1) + chr(c2) + chr(c3)

def encodestring(s):
	import StringIO
	f = StringIO.StringIO(s)
	g = StringIO.StringIO()
	encode(f, g)
	return g.getvalue()

def decodestring(s):
	import StringIO
	f = StringIO.StringIO(s)
	g = StringIO.StringIO()
	decode(f, g)
	return g.getvalue()

# Small test program, reads stdin, writes stdout.
# no arg: encode, any arg: decode.
def test():
	import sys
	if sys.argv[1:]:
		decode(sys.stdin, sys.stdout)
	else:
		encode(sys.stdin, sys.stdout)

def test1():
	s0 = "Aladdin:open sesame"
	s1 = encodestring(s0)
	s2 = decodestring(s1)
	print s0, `s1`, s2

if __name__ == '__main__':
	test()
