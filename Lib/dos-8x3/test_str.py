from test_support import verbose
import strop, sys

def test(name, input, output, *args):
    if verbose:
        print 'string.%s%s =? %s... ' % (name, (input,) + args, output),
    f = getattr(strop, name)
    try:
        value = apply(f, (input,) + args)
    except:
         value = sys.exc_type
    if value != output:
        if verbose:
            print 'no'
        print f, `input`, `output`, `value`
    else:
        if verbose:
            print 'yes'

test('atoi', " 1 ", 1)
test('atoi', " 1x", ValueError)
test('atoi', " x1 ", ValueError)
test('atol', "  1  ", 1L)
test('atol', "  1x ", ValueError)
test('atol', "  x1 ", ValueError)
test('atof', "  1  ", 1.0)
test('atof', "  1x ", ValueError)
test('atof', "  x1 ", ValueError)

test('capitalize', ' hello ', ' hello ')
test('capitalize', 'hello ', 'Hello ')
test('find', 'abcdefghiabc', 0, 'abc')
test('find', 'abcdefghiabc', 9, 'abc', 1)
test('find', 'abcdefghiabc', -1, 'def', 4)
test('rfind', 'abcdefghiabc', 9, 'abc')
test('lower', 'HeLLo', 'hello')
test('upper', 'HeLLo', 'HELLO')

transtable = '\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`xyzdefghijklmnopqrstuvwxyz{|}~\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377'

test('maketrans', 'abc', transtable, 'xyz')
test('maketrans', 'abc', ValueError, 'xyzq')

test('split', 'this is the split function',
     ['this', 'is', 'the', 'split', 'function'])
test('split', 'a|b|c|d', ['a', 'b', 'c', 'd'], '|')
test('split', 'a|b|c|d', ['a', 'b', 'c|d'], '|', 2)
test('split', 'a b c d', ['a', 'b c d'], None, 1)
test('split', 'a b c d', ['a', 'b', 'c d'], None, 2)
test('split', 'a b c d', ['a', 'b', 'c', 'd'], None, 3)
test('split', 'a b c d', ['a', 'b', 'c', 'd'], None, 4)
test('split', 'a b c d', ['a', 'b', 'c', 'd'], None, 0)
test('split', 'a  b  c  d', ['a', 'b', 'c  d'], None, 2)

# join now works with any sequence type
class Sequence:
    def __init__(self): self.seq = 'wxyz'
    def __len__(self): return len(self.seq)
    def __getitem__(self, i): return self.seq[i]

test('join', ['a', 'b', 'c', 'd'], 'a b c d')
test('join', ('a', 'b', 'c', 'd'), 'abcd', '')
test('join', Sequence(), 'w x y z')

# try a few long ones
print strop.join(['x' * 100] * 100, ':')
print strop.join(('x' * 100,) * 100, ':')

test('strip', '   hello   ', 'hello')
test('lstrip', '   hello   ', 'hello   ')
test('rstrip', '   hello   ', '   hello')

test('swapcase', 'HeLLo cOmpUteRs', 'hEllO CoMPuTErS')
test('translate', 'xyzabcdef', 'xyzxyz', transtable, 'def')

test('replace', 'one!two!three!', 'one@two!three!', '!', '@', 1)
test('replace', 'one!two!three!', 'one@two@three!', '!', '@', 2)
test('replace', 'one!two!three!', 'one@two@three@', '!', '@', 3)
test('replace', 'one!two!three!', 'one@two@three@', '!', '@', 4)
test('replace', 'one!two!three!', 'one@two@three@', '!', '@', 0)
test('replace', 'one!two!three!', 'one@two@three@', '!', '@')
test('replace', 'one!two!three!', 'one!two!three!', 'x', '@')
test('replace', 'one!two!three!', 'one!two!three!', 'x', '@', 2)

strop.whitespace
strop.lowercase
strop.uppercase
