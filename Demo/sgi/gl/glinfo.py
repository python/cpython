#! /usr/bin/env python

# Print the values of all values that can be inquired with getgdesc().
# See man getgdesc() for a description.

import gl
import GL

def main():
	names = []
	maxlen = 0
	for name in dir(GL):
		if name[:3] == 'GD_':
			names.append(name)
			maxlen = max(maxlen, len(name))
	for name in names:
		print name + (maxlen - len(name))*' ' + '=',
		print gl.getgdesc(getattr(GL, name))

main()
