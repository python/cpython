# Conversions to/from quoted-printable transport encoding as per RFC-XXXX
# (Dec 1991 version).

ESCAPE = '='
MAXLINESIZE = 76
HEX = '0123456789ABCDEF'

def needsquoting(c, quotetabs):
	if c == '\t':
		return not quotetabs
	return c == ESCAPE or not(' ' <= c <= '~')

def quote(c):
	if c == ESCAPE:
		return ESCAPE * 2
	else:
		i = ord(c)
		return ESCAPE + HEX[i/16] + HEX[i%16]

def encode(input, output, quotetabs):
	while 1:
		line = input.readline()
		if not line: break
		new = ''
		last = line[-1:]
		if last == '\n': line = line[:-1]
		else: last = ''
		prev = ''
		for c in line:
			if needsquoting(c, quotetabs):
				c = quote(c)
			if len(new) + len(c) >= MAXLINESIZE:
				output.write(new + ESCAPE + '\n')
				new = ''
			new = new + c
			prev = c
		if prev in (' ', '\t'):
			output.write(new + ESCAPE + '\n\n')
		else:
			output.write(new + '\n')

def decode(input, output):
	new = ''
	while 1:
		line = input.readline()
		if not line: break
		i, n = 0, len(line)
		if n > 0 and line[n-1] == '\n':
			partial = 0; n = n-1
			# Strip trailing whitespace
			while n > 0 and line[n-1] in (' ', '\t'):
				n = n-1
		else:
			partial = 1
		while i < n:
			c = line[i]
			if c <> ESCAPE:
				new = new + c; i = i+1
			elif i+1 == n and not partial:
				partial = 1; break
			elif i+1 < n and line[i+1] == ESCAPE:
				new = new + ESCAPE; i = i+2
			elif i+2 < n and ishex(line[i+1]) and ishex(line[i+2]):
				new = new + chr(unhex(line[i+1:i+3])); i = i+3
			else: # Bad escape sequence -- leave it in
				new = new + c; i = i+1
		if not partial:
			output.write(new + '\n')
			new = ''
	if new:
		output.write(new)

def ishex(c):
	return '0' <= c <= '9' or 'a' <= c <= 'f' or 'A' <= c <= 'F'

def unhex(s):
	bits = 0
	for c in s:
		if '0' <= c <= '9':
			i = ord('0')
		elif 'a' <= c <= 'f':
			i = ord('a')-10
		elif 'A' <= c <= 'F':
			i = ord('A')-10
		else:
			break
		bits = bits*16 + (ord(c) - i)
	return bits

def test():
	import sys
	if sys.argv[1:]:
		if sys.argv[1] == '-t': # Quote tabs
			encode(sys.stdin, sys.stdout, 1)
		else:
			decode(sys.stdin, sys.stdout)
	else:
		encode(sys.stdin, sys.stdout, 0)

if __name__ == '__main__':
	main()
