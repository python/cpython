# Module 'rect'.
#
# Operations on rectangles.
# There is some normalization: all results return the object 'empty'
# if their result would contain no points.


# Exception.
#
error = 'rect.error'


# The empty rectangle.
#
empty = (0, 0), (0, 0)


# Check if a rectangle is empty.
#
def is_empty((left, top), (right, bottom)):
	return left >= right or top >= bottom


# Compute the intersection or two or more rectangles.
# This works with a list or tuple argument.
#
def intersect(list):
	if not list: raise error, 'intersect called with empty list'
	if is_empty(list[0]): return empty
	(left, top), (right, bottom) = list[0]
	for rect in list[1:]:
		if is_empty(rect):
			return empty
		(l, t), (r, b) = rect
		if left < l: left = l
		if top < t: top = t
		if right > r: right = r
		if bottom > b: bottom = b
		if is_empty((left, top), (right, bottom)):
			return empty
	return (left, top), (right, bottom)


# Compute the smallest rectangle containing all given rectangles.
# This works with a list or tuple argument.
#
def union(list):
	(left, top), (right, bottom) = empty
	for (l, t), (r, b) in list[1:]:
		if not is_empty((l, t), (r, b)):
			if l < left: left = l
			if t < top: top = t
			if r > right: right = r
			if b > bottom: bottom = b
	res = (left, top), (right, bottom)
	if is_empty(res):
		return empty
	return res


# Check if a point is in a rectangle.
#
def pointinrect((h, v), ((left, top), (right, bottom))):
	return left <= h < right and top <= v < bottom


# Return a rectangle that is dh, dv inside another
#
def inset(((left, top), (right, bottom)), (dh, dv)):
	left = left + dh
	top = top + dv
	right = right - dh
	bottom = bottom - dv
	r = (left, top), (right, bottom)
	if is_empty(r):
		return empty
	else:
		return r


# Conversions between rectangles and 'geometry tuples',
# given as origin (h, v) and dimensions (width, height).
#
def rect2geom((left, top), (right, bottom)):
	return (left, top), (right-left, bottom-top)

def geom2rect((h, v), (width, height)):
	return (h, v), (h+width, v+height)
