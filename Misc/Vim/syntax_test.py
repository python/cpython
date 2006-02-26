"""Test file for syntax highlighting of editors.

Meant to cover a wide range of different types of statements and expressions.
Not necessarily sensical.

"""
assert True
def foo(): pass
foo()  # Uncoloured
while False: pass
1 and 2
if False: pass
from sys import path
# Comment
# XXX catch your attention
'single-quote', u'unicode'
"double-quote"
"""triple double-quote"""
'''triple single-quote'''
r'raw'
ur'unicode raw'
'escape\n'
'\04'  # octal
'\xFF' # hex
'\u1111' # unicode character
1
1L
1.0
.1
1+2j
[]  # Uncoloured
{}  # Uncoloured
()  # Uncoloured
all
GeneratorExit
trailing_whitespace = path
