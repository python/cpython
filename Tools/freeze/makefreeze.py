import marshal


# Write a file containing frozen code for the modules in the dictionary.

header = """
struct frozen {
	char *name;
	unsigned char *code;
	int size;
} frozen_modules[] = {
"""
trailer = """\
	{0, 0, 0} /* sentinel */
};
"""

def makefreeze(outfp, dict):
	done = []
	mods = dict.keys()
	mods.sort()
	for mod in mods:
		modfn = dict[mod]
		try:
			str = makecode(modfn)
		except IOError, msg:
			sys.stderr.write("%s: %s\n" % (modfn, str(msg)))
			continue
		if str:
			done.append(mod, len(str))
			writecode(outfp, mod, str)
	outfp.write(header)
	for mod, size in done:
		outfp.write('\t{"%s", M_%s, %d},\n' % (mod, mod, size))
	outfp.write(trailer)


# Return code string for a given module -- either a .py or a .pyc
# file.  Return either a string or None (if it's not Python code).
# May raise IOError.

def makecode(filename):
	if filename[-3:] == '.py':
		f = open(filename, 'r')
		try:
			text = f.read()
			code = compile(text, filename, 'exec')
		finally:
			f.close()
		return marshal.dumps(code)
	if filename[-4:] == '.pyc':
		f = open(filename, 'rb')
		try:
			f.seek(8)
			str = f.read()
		finally:
			f.close()
		return str
	# Can't generate code for this extension
	return None


# Write a C initializer for a module containing the frozen python code.
# The array is called M_<mod>.

def writecode(outfp, mod, str):
	outfp.write('static unsigned char M_%s[] = {' % mod)
	for i in range(0, len(str), 16):
		outfp.write('\n\t')
		for c in str[i:i+16]:
			outfp.write('%d,' % ord(c))
	outfp.write('\n};\n')


# Test for the above functions.

def test():
	import os
	import sys
	if not sys.argv[1:]:
		print 'usage: python freezepython.py file.py(c) ...'
		sys.exit(2)
	dict = {}
	for arg in sys.argv[1:]:
		base = os.path.basename(arg)
		mod, ext = os.path.splitext(base)
		dict[mod] = arg
	makefreeze(sys.stdout, dict)

if __name__ == '__main__':
	test()
