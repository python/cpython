import strop, sys

def test(name, input, output):
    f = getattr(strop, name)
    try:
	value = f(input)
    except:
	 value = sys.exc_type
	 print sys.exc_value
    if value != output:
	print f, `input`, `output`, `value`

test('atoi', " 1 ", 1)
test('atoi', " 1x", ValueError)
test('atoi', " x1 ", ValueError)
test('atol', "  1  ", 1L)
test('atol', "  1x ", ValueError)
test('atol', "  x1 ", ValueError)
test('atof', "  1  ", 1.0)
test('atof', "  1x ", ValueError)
test('atof', "  x1 ", ValueError)
