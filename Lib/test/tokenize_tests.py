# Tests for the 'tokenize' module.
# Large bits stolen from test_grammar.py. 

# Comments
"#"
#'
#"
#\
       #
    # abc
'''#
#'''

x = 1  #

# Balancing continuation

a = (3, 4,
  5, 6)
y = [3, 4,
  5]
z = {'a':5,
  'b':6}
x = (len(`y`) + 5*x - a[
   3 ]
   - x + len({
   }
    )
  )

# Backslash means line continuation:
x = 1 \
+ 1

# Backslash does not means continuation in comments :\
x = 0

# Ordinary integers
0xff <> 255
0377 <> 255
2147483647   != 017777777777
-2147483647-1 != 020000000000
037777777777 != -1
0xffffffff != -1

# Long integers
x = 0L
x = 0l
x = 0xffffffffffffffffL
x = 0xffffffffffffffffl
x = 077777777777777777L
x = 077777777777777777l
x = 123456789012345678901234567890L
x = 123456789012345678901234567890l

# Floating-point numbers
x = 3.14
x = 314.
x = 0.314
# XXX x = 000.314
x = .314
x = 3e14
x = 3E14
x = 3e-14
x = 3e+14
x = 3.e14
x = .3e14
x = 3.1e4

# String literals
x = ''; y = "";
x = '\''; y = "'";
x = '"'; y = "\"";
x = "doesn't \"shrink\" does it"
y = 'doesn\'t "shrink" does it'
x = "does \"shrink\" doesn't it"
y = 'does "shrink" doesn\'t it'
x = """
The "quick"
brown fox
jumps over
the 'lazy' dog.
"""
y = '\nThe "quick"\nbrown fox\njumps over\nthe \'lazy\' dog.\n'
y = '''
The "quick"
brown fox
jumps over
the 'lazy' dog.
''';
y = "\n\
The \"quick\"\n\
brown fox\n\
jumps over\n\
the 'lazy' dog.\n\
";
y = '\n\
The \"quick\"\n\
brown fox\n\
jumps over\n\
the \'lazy\' dog.\n\
';
x = r'\\' + R'\\'
x = r'\'' + ''
y = r'''
foo bar \\
baz''' + R'''
foo'''
y = r"""foo
bar \\ baz
""" + R'''spam
'''

# Indentation
if 1:
    x = 2
if 1:
        x = 2
if 1:
    while 0:
     if 0:
           x = 2
     x = 2
if 0:
  if 2:
   while 0:
        if 1:
          x = 2

# Operators

def d22(a, b, c=1, d=2): pass
def d01v(a=1, *rest, **rest): pass

(x, y) <> ({'a':1}, {'b':2})

# comparison
if 1 < 1 > 1 == 1 >= 1 <= 1 <> 1 != 1 in 1 not in 1 is 1 is not 1: pass

# binary
x = 1 & 1
x = 1 ^ 1
x = 1 | 1

# shift
x = 1 << 1 >> 1

# additive
x = 1 - 1 + 1 - 1 + 1

# multiplicative
x = 1 / 1 * 1 % 1

# unary
x = ~1 ^ 1 & 1 | 1 & 1 ^ -1
x = -1*1/1 + 1*1 - ---1*1

# selector
import sys, time
x = sys.modules['time'].time()

