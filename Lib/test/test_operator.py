import operator
import sys

def test(name, input, output, *args):
    print 'testing:', name
    f = getattr(operator, name)
    params = (input,) + args
    try:
        val = apply(f, params)
    except:
        val = sys.exc_type
    if val <> output:
        print '%s%s = %s: %s expected' % (f.__name__, params, `val`, `output`)

test('abs', -1, 1)
test('add', 3, 7, 4)
test('and_', 0xf, 0xa, 0xa)
test('concat', 'py', 'python', 'thon')

test('countOf', [1, 2, 1, 3, 1, 4], 1, 3)

a = [4, 3, 2, 1]
test('delitem', a, None, 1)
if a <> [4, 2, 1]:
    print 'delitem() failed'

a = range(10)
test('delslice', a, None, 2, 8)
if a <> [0, 1, 8, 9]:
    print 'delslice() failed'

a = range(10)
test('div', 5, 2, 2)
test('getitem', a, 2, 2)
test('getslice', a, [4, 5], 4, 6)
test('indexOf', [4, 3, 2, 1], 1, 3)
test('inv', 4, -5)
test('isCallable', 4, 0)
test('isCallable', operator.isCallable, 1)
test('isMappingType', operator.isMappingType, 0)
test('isMappingType', operator.__dict__, 1)
test('isNumberType', 8.3, 1)
test('isNumberType', dir(), 0)
test('isSequenceType', dir(), 1)
test('isSequenceType', 'yeahbuddy', 1)
test('isSequenceType', 3, 0)
test('lshift', 5, 10, 1)
test('mod', 5, 1, 2)
test('mul', 5, 10, 2)
test('neg', 5, -5)
test('or_', 0xa, 0xf, 0x5)
test('pos', -5, -5)

a = range(3)
test('repeat', a, a+a, 2)
test('rshift', 5, 2, 1)

test('sequenceIncludes', range(4), 1, 2)
test('sequenceIncludes', range(4), 0, 5)

test('setitem', a, None, 0, 2)
if a <> [2, 1, 2]:
    print 'setitem() failed'

a = range(4)
test('setslice', a, None, 1, 3, [2, 1])
if a <> [0, 2, 1, 3]:
    print 'setslice() failed:', a

test('sub', 5, 2, 3)
test('truth', 5, 1)
test('truth', [], 0)
test('xor', 0xb, 0x7, 0xc)


# some negative tests
test('indexOf', [4, 3, 2, 1], ValueError, 9)
