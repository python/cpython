#! /usr/local/python

# Print digits of pi forever.
#
# The algorithm, using Python's 'long' integers ("bignums"), works
# with continued fractions, and was conceived by Lambert Meertens.
#
# See also the ABC Programmer's Handbook, by Geurts, Meertens & Pemberton,
# published by Prentice-Hall (UK) Ltd., 1990.

import sys

def main():
	k, a, b, a1, b1 = 2l, 4l, 1l, 12l, 4l
	while 1:
		# Next approximation
		p, q, k = k*k, 2l*k+1l, k+1l
		a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
		# Print common digits
		d, d1 = a/b, a1/b1
		#print a, b, a1, b1
		while d == d1:
			# Use write() to avoid spaces between the digits
			sys.stdout.write(`int(d)`)
			# Flush so the output is seen immediately
			sys.stdout.flush()
			a, a1 = 10l*(a%b), 10l*(a1%b1)
			d, d1 = a/b, a1/b1

main()
