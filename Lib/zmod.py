# module 'zmod'

# Compute properties of mathematical "fields" formed by taking
# Z/n (the whole numbers modulo some whole number n) and an 
# irreducible polynomial (i.e., a polynomial with only complex zeros),
# e.g., Z/5 and X**2 + 2.
#
# The field is formed by taking all possible linear combinations of
# a set of d base vectors (where d is the degree of the polynomial).
#
# Note that this procedure doesn't yield a field for all combinations
# of n and p: it may well be that some numbers have more than one
# inverse and others have none.  This is what we check.
#
# Remember that a field is a ring where each element has an inverse.
# A ring has commutative addition and multiplication, a zero and a one:
# 0*x = x*0 = 0, 0+x = x+0 = x, 1*x = x*1 = x.  Also, the distributive
# property holds: a*(b+c) = a*b + b*c.
# (XXX I forget if this is an axiom or follows from the rules.)

import poly


# Example N and polynomial

N = 5
P = poly.plus(poly.one(0, 2), poly.one(2, 1)) # 2 + x**2


# Return x modulo y.  Returns >= 0 even if x < 0.

def mod(x, y):
	return divmod(x, y)[1]


# Normalize a polynomial modulo n and modulo p.

def norm(a, n, p):
	a = poly.modulo(a, p)
	a = a[:]
	for i in range(len(a)): a[i] = mod(a[i], n)
	a = poly.normalize(a)
	return a


# Make a list of all n^d elements of the proposed field.

def make_all(mat):
	all = []
	for row in mat:
		for a in row:
			all.append(a)
	return all

def make_elements(n, d):
	if d = 0: return [poly.one(0, 0)]
	sub = make_elements(n, d-1)
	all = []
	for a in sub:
		for i in range(n):
			all.append(poly.plus(a, poly.one(d-1, i)))
	return all

def make_inv(all, n, p):
	x = poly.one(1, 1)
	inv = []
	for a in all:
		inv.append(norm(poly.times(a, x), n, p))
	return inv

def checkfield(n, p):
	all = make_elements(n, len(p)-1)
	inv = make_inv(all, n, p)
	all1 = all[:]
	inv1 = inv[:]
	all1.sort()
	inv1.sort()
	if all1 = inv1: print 'BINGO!'
	else:
		print 'Sorry:', n, p
		print all
		print inv

def rj(s, width):
	if type(s) <> type(''): s = `s`
	n = len(s)
	if n >= width: return s
	return ' '*(width - n) + s

def lj(s, width):
	if type(s) <> type(''): s = `s`
	n = len(s)
	if n >= width: return s
	return s + ' '*(width - n)
