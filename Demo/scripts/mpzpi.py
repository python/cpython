#! /usr/bin/env python
# Print digits of pi forever.
#
# The algorithm, using Python's 'long' integers ("bignums"), works
# with continued fractions, and was conceived by Lambert Meertens.
#
# See also the ABC Programmer's Handbook, by Geurts, Meertens & Pemberton,
# published by Prentice-Hall (UK) Ltd., 1990.

import sys
from mpz import mpz

def main():
	mpzone, mpztwo, mpzten = mpz(1), mpz(2), mpz(10)
	k, a, b, a1, b1 = mpz(2), mpz(4), mpz(1), mpz(12), mpz(4)
	while 1:
		# Next approximation
		p, q, k = k*k, mpztwo*k+mpzone, k+mpzone
		a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
		# Print common digits
		d, d1 = a/b, a1/b1
		while d == d1:
			output(d)
			a, a1 = mpzten*(a%b), mpzten*(a1%b1)
			d, d1 = a/b, a1/b1

def output(d):
	# Use write() to avoid spaces between the digits
	# Use int(d) to avoid a trailing L after each digit
	sys.stdout.write(`int(d)`)
	# Flush so the output is seen immediately
	sys.stdout.flush()

main()
