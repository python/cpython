# General floating point formatting functions.

# Functions:
# fix(x, digits_behind)
# sci(x, digits_behind)

# Each takes a number or a string and a number of digits as arguments.

# Parameters:
# x:             number to be formatted; or a string resembling a number
# digits_behind: number of digits behind the decimal point


import regex

# Compiled regular expression to "decode" a number
decoder = regex.compile( \
	'^\([-+]?\)0*\([0-9]*\)\(\(\.[0-9]*\)?\)\(\([eE][-+]?[0-9]+\)?\)$')
# \0 the whole thing
# \1 leading sign or empty
# \2 digits left of decimal point
# \3 fraction (empty or begins with point)
# \5 exponent part (empty or begins with 'e' or 'E')

NotANumber = 'fpformat.NotANumber'

# Return (sign, intpart, fraction, expo) or raise an exception:
# sign is '+' or '-'
# intpart is 0 or more digits beginning with a nonzero
# fraction is 0 or more digits
# expo is an integer
def extract(s):
	if decoder.match(s) < 0: raise NotANumber
	(a1, b1), (a2, b2), (a3, b3), (a4, b4), (a5, b5) = decoder.regs[1:6]
	sign, intpart, fraction, exppart = \
		s[a1:b1], s[a2:b2], s[a3:b3], s[a5:b5]
	if sign == '+': sign = ''
	if fraction: fraction = fraction[1:]
	if exppart: expo = eval(exppart[1:])
	else: expo = 0
	return sign, intpart, fraction, expo

# Remove the exponent by changing intpart and fraction
def unexpo(intpart, fraction, expo):
	if expo > 0: # Move the point left
		f = len(fraction)
		intpart, fraction = intpart + fraction[:expo], fraction[expo:]
		if expo > f:
			intpart = intpart + '0'*(expo-f)
	elif expo < 0: # Move the point right
		i = len(intpart)
		intpart, fraction = intpart[:expo], intpart[expo:] + fraction
		if expo < -i:
			fraction = '0'*(-expo-i) + fraction
	return intpart, fraction

# Round or extend the fraction to size digs
def roundfrac(intpart, fraction, digs):
	f = len(fraction)
	if f <= digs:
		return intpart, fraction + '0'*(digs-f)
	i = len(intpart)
	if i+digs < 0:
		return '0'*-digs, ''
	total = intpart + fraction
	nextdigit = total[i+digs]
	if nextdigit >= '5': # Hard case: increment last digit, may have carry!
		n = i + digs - 1
		while n >= 0:
			if total[n] != '9': break
			n = n-1
		else:
			total = '0' + total
			i = i+1
			n = 0
		total = total[:n] + chr(ord(total[n]) + 1) + '0'*(len(total)-n-1)
		intpart, fraction = total[:i], total[i:]
	if digs >= 0:
		return intpart, fraction[:digs]
	else:
		return intpart[:digs] + '0'*-digs, ''

# Format x as [-]ddd.ddd with 'digs' digits after the point
# and at least one digit before.
# If digs <= 0, the point is suppressed.
def fix(x, digs):
	if type(x) != type(''): x = `x`
	try:
		sign, intpart, fraction, expo = extract(x)
	except NotANumber:
		return x
	intpart, fraction = unexpo(intpart, fraction, expo)
	intpart, fraction = roundfrac(intpart, fraction, digs)
	while intpart and intpart[0] == '0': intpart = intpart[1:]
	if intpart == '': intpart = '0'
	if digs > 0: return sign + intpart + '.' + fraction
	else: return sign + intpart

# Format x as [-]d.dddE[+-]ddd with 'digs' digits after the point
# and exactly one digit before.
# If digs is <= 0, one digit is kept and the point is suppressed.
def sci(x, digs):
	if type(x) != type(''): x = `x`
	sign, intpart, fraction, expo = extract(x)
	if not intpart:
		while fraction and fraction[0] == '0':
			fraction = fraction[1:]
			expo = expo - 1
		if fraction:
			intpart, fraction = fraction[0], fraction[1:]
			expo = expo - 1
		else:
			intpart = '0'
	else:
		expo = expo + len(intpart) - 1
		intpart, fraction = intpart[0], intpart[1:] + fraction
	digs = max(0, digs)
	intpart, fraction = roundfrac(intpart, fraction, digs)
	if len(intpart) > 1:
		intpart, fraction, expo = \
			intpart[0], intpart[1:] + fraction[:-1], \
			expo + len(intpart) - 1
	s = sign + intpart
	if digs > 0: s = s + '.' + fraction
	e = `abs(expo)`
	e = '0'*(3-len(e)) + e
	if expo < 0: e = '-' + e
	else: e = '+' + e
	return s + 'e' + e

# Interactive test run
def test():
	try:
		while 1:
			x, digs = input('Enter (x, digs): ')
			print x, fix(x, digs), sci(x, digs)
	except (EOFError, KeyboardInterrupt):
		pass
