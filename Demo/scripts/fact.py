#! /usr/local/python

# Factorize numbers, slowly.
# This version uses plain integers and is thus limited to 2**31-1.

import sys
from math import sqrt

error = 'fact.error'		# exception

def fact(n):
	if n < 1: raise error	# fact() argument should be >= 1
	if n = 1: return []	# special case
	res = []
	# Treat even factors special, so we can use i = i+2 later
	while n%2 = 0:
		res.append(2)
		n = n/2
	# Try odd numbers up to sqrt(n)
	limit = int(sqrt(float(n+1)))
	i = 3
	while i <= limit:
		if n%i = 0:
			res.append(i)
			n = n/i
			limit = int(sqrt(float(n+1)))
		else:
			i = i+2
	res.append(n)
	return res

def main():
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			n = int(eval(arg))
			print n, fact(n)
	else:
		try:
			while 1:
				n = int(input())
				print n, fact(n)
		except EOFError:
			pass

main()
