# Test bug in caching of forms

import sys
import os
import flp

filename = 'tcache.fd'
cachename = filename + 's'

def first():
	try:
		os.unlink(cachename)
	except os.error:
		pass
	first = flp.parse_form(filename, 'first')

def second():
	forms = flp.parse_forms(filename)
	k = forms.keys()
	if 'first' in k and 'second' in k:
		print 'OK'
	else:
		print 'BAD!', k

def main():
	if sys.argv[1:]:
		second()
	else:
		first()
		print 'Now run the script again with an argument'

main()
