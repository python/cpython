#! /usr/bin/env python

# Print digits of pi forever.
#
# The algorithm, using Python's 'long' integers ("bignums"), works
# with continued fractions, and was conceived by Lambert Meertens.
#
# See also the ABC Programmer's Handbook, by Geurts, Meertens & Pemberton,
# published by Prentice-Hall (UK) Ltd., 1990.

import sys

def main():
	k, a, b, a1, b1 = 2L, 4L, 1L, 12L, 4L
	while 1:
		# Next approximation
		p, q, k = k*k, 2L*k+1L, k+1L
		a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
		# Print common digits
		d, d1 = a/b, a1/b1
		while d == d1:
			output(d)
			a, a1 = 10L*(a%b), 10L*(a1%b1)
			d, d1 = a/b, a1/b1

def output(d):
	# Use write() to avoid spaces between the digits
	# Use int(d) to avoid a trailing L after each digit
	sys.stdout.write(`int(d)`)
	# Flush so the output is seen immediately
	sys.stdout.flush()

main()
