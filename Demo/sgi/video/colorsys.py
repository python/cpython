# Conversions between RGB and YIQ color spaces.
#
# R, G, B are amounts of Red, Green and Blue;
# Y, I, Q are intensity (Y) and two difference signals (I, Q).


# Convert a (R, G, B) triple to (Y, I, Q).
# Scale is arbitrary (output uses the same scale as input).

def rgb_to_yiq(r, g, b):
	y = 0.30*r + 0.59*g + 0.11*b
	i = 0.60*r - 0.28*g - 0.32*b
	q = 0.21*r - 0.52*g + 0.31*b
	return y, i, q


# Convert a (Y, I, Q) triple to (R, G, B).
# Input and output values are in the interval [0.0 ... 1.0].

def yiq_to_rgb(y, i, q):
	r = y + 0.948262*i + 0.624013*q
	g = y - 0.276066*i - 0.639810*q
	b = y - 1.105450*i + 1.729860*q
	if r < 0.0: r = 0.0
	if g < 0.0: g = 0.0
	if b < 0.0: b = 0.0
	if r > 1.0: r = 1.0
	if g > 1.0: g = 1.0
	if b > 1.0: b = 1.0
	return r, g, b
