"""This module tests SyntaxErrors.

Here's an example of the sort of thing that is tested.

>>> def f(x):
...     global x
Traceback (most recent call last):
SyntaxError: name 'x' is parameter and global

The tests are all raise SyntaxErrors.  They were created by checking
each C call that raises SyntaxError.  There are several modules that
raise these exceptions-- ast.c, compile.c, future.c, pythonrun.c, and
symtable.c.

The parser itself outlaws a lot of invalid syntax.  None of these
errors are tested here at the moment.  We should add some tests; since
there are infinitely many programs with invalid syntax, we would need
to be judicious in selecting some.

The compiler generates a synthetic module name for code executed by
doctest.  Since all the code comes from the same module, a suffix like
[1] is appended to the module name, As a consequence, changing the
order of tests in this module means renumbering all the errors after
it.  (Maybe we should enable the ellipsis option for these tests.)

In ast.c, syntax errors are raised by calling ast_error().

Errors from set_context():

>>> obj.None = 1
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> None = 1
Traceback (most recent call last):
SyntaxError: cannot assign to None

>>> obj.True = 1
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> True = 1
Traceback (most recent call last):
SyntaxError: cannot assign to True

>>> (True := 1)
Traceback (most recent call last):
SyntaxError: cannot use assignment expressions with True

>>> obj.__debug__ = 1
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

>>> __debug__ = 1
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

>>> (__debug__ := 1)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

>>> f() = 1
Traceback (most recent call last):
SyntaxError: cannot assign to function call

# Pegen does not support this yet
# >>> del f()
# Traceback (most recent call last):
# SyntaxError: cannot delete function call

>>> a + 1 = 2
Traceback (most recent call last):
SyntaxError: cannot assign to operator

>>> (x for x in x) = 1
Traceback (most recent call last):
SyntaxError: cannot assign to generator expression

>>> 1 = 1
Traceback (most recent call last):
SyntaxError: cannot assign to literal

>>> "abc" = 1
Traceback (most recent call last):
SyntaxError: cannot assign to literal

>>> b"" = 1
Traceback (most recent call last):
SyntaxError: cannot assign to literal

>>> ... = 1
Traceback (most recent call last):
SyntaxError: cannot assign to Ellipsis

>>> `1` = 1
Traceback (most recent call last):
SyntaxError: invalid syntax

If the left-hand side of an assignment is a list or tuple, an illegal
expression inside that contain should still cause a syntax error.
This test just checks a couple of cases rather than enumerating all of
them.

# All of the following also produce different error messages with pegen
# >>> (a, "b", c) = (1, 2, 3)
# Traceback (most recent call last):
# SyntaxError: cannot assign to literal

# >>> (a, True, c) = (1, 2, 3)
# Traceback (most recent call last):
# SyntaxError: cannot assign to True

>>> (a, __debug__, c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

# >>> (a, *True, c) = (1, 2, 3)
# Traceback (most recent call last):
# SyntaxError: cannot assign to True

>>> (a, *__debug__, c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

# >>> [a, b, c + 1] = [1, 2, 3]
# Traceback (most recent call last):
# SyntaxError: cannot assign to operator

>>> a if 1 else b = 1
Traceback (most recent call last):
SyntaxError: cannot assign to conditional expression

From compiler_complex_args():

>>> def f(None=1):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax


From ast_for_arguments():

>>> def f(x, y=1, z):
...     pass
Traceback (most recent call last):
SyntaxError: non-default argument follows default argument

>>> def f(x, None):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> def f(*None):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> def f(**None):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax


From ast_for_funcdef():

>>> def None(x):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax


From ast_for_call():

>>> def f(it, *varargs, **kwargs):
...     return list(it)
>>> L = range(10)
>>> f(x for x in L)
[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
>>> f(x for x in L, 1)
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized
>>> f(x for x in L, y=1)
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized
>>> f(x for x in L, *[])
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized
>>> f(x for x in L, **{})
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized

# >>> f(L, x for x in L)
# Traceback (most recent call last):
# SyntaxError: Generator expression must be parenthesized

>>> f(x for x in L, y for y in L)
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized
>>> f(x for x in L,)
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized
>>> f((x for x in L), 1)
[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
>>> class C(x for x in L):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> def g(*args, **kwargs):
...     print(args, sorted(kwargs.items()))
>>> g(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
...   20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
...   38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
...   56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
...   74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
...   92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
...   108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
...   122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
...   136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
...   150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
...   164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
...   178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
...   192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
...   206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
...   220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
...   234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
...   248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261,
...   262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275,
...   276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289,
...   290, 291, 292, 293, 294, 295, 296, 297, 298, 299)  # doctest: +ELLIPSIS
(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, ..., 297, 298, 299) []

>>> g(a000=0, a001=1, a002=2, a003=3, a004=4, a005=5, a006=6, a007=7, a008=8,
...   a009=9, a010=10, a011=11, a012=12, a013=13, a014=14, a015=15, a016=16,
...   a017=17, a018=18, a019=19, a020=20, a021=21, a022=22, a023=23, a024=24,
...   a025=25, a026=26, a027=27, a028=28, a029=29, a030=30, a031=31, a032=32,
...   a033=33, a034=34, a035=35, a036=36, a037=37, a038=38, a039=39, a040=40,
...   a041=41, a042=42, a043=43, a044=44, a045=45, a046=46, a047=47, a048=48,
...   a049=49, a050=50, a051=51, a052=52, a053=53, a054=54, a055=55, a056=56,
...   a057=57, a058=58, a059=59, a060=60, a061=61, a062=62, a063=63, a064=64,
...   a065=65, a066=66, a067=67, a068=68, a069=69, a070=70, a071=71, a072=72,
...   a073=73, a074=74, a075=75, a076=76, a077=77, a078=78, a079=79, a080=80,
...   a081=81, a082=82, a083=83, a084=84, a085=85, a086=86, a087=87, a088=88,
...   a089=89, a090=90, a091=91, a092=92, a093=93, a094=94, a095=95, a096=96,
...   a097=97, a098=98, a099=99, a100=100, a101=101, a102=102, a103=103,
...   a104=104, a105=105, a106=106, a107=107, a108=108, a109=109, a110=110,
...   a111=111, a112=112, a113=113, a114=114, a115=115, a116=116, a117=117,
...   a118=118, a119=119, a120=120, a121=121, a122=122, a123=123, a124=124,
...   a125=125, a126=126, a127=127, a128=128, a129=129, a130=130, a131=131,
...   a132=132, a133=133, a134=134, a135=135, a136=136, a137=137, a138=138,
...   a139=139, a140=140, a141=141, a142=142, a143=143, a144=144, a145=145,
...   a146=146, a147=147, a148=148, a149=149, a150=150, a151=151, a152=152,
...   a153=153, a154=154, a155=155, a156=156, a157=157, a158=158, a159=159,
...   a160=160, a161=161, a162=162, a163=163, a164=164, a165=165, a166=166,
...   a167=167, a168=168, a169=169, a170=170, a171=171, a172=172, a173=173,
...   a174=174, a175=175, a176=176, a177=177, a178=178, a179=179, a180=180,
...   a181=181, a182=182, a183=183, a184=184, a185=185, a186=186, a187=187,
...   a188=188, a189=189, a190=190, a191=191, a192=192, a193=193, a194=194,
...   a195=195, a196=196, a197=197, a198=198, a199=199, a200=200, a201=201,
...   a202=202, a203=203, a204=204, a205=205, a206=206, a207=207, a208=208,
...   a209=209, a210=210, a211=211, a212=212, a213=213, a214=214, a215=215,
...   a216=216, a217=217, a218=218, a219=219, a220=220, a221=221, a222=222,
...   a223=223, a224=224, a225=225, a226=226, a227=227, a228=228, a229=229,
...   a230=230, a231=231, a232=232, a233=233, a234=234, a235=235, a236=236,
...   a237=237, a238=238, a239=239, a240=240, a241=241, a242=242, a243=243,
...   a244=244, a245=245, a246=246, a247=247, a248=248, a249=249, a250=250,
...   a251=251, a252=252, a253=253, a254=254, a255=255, a256=256, a257=257,
...   a258=258, a259=259, a260=260, a261=261, a262=262, a263=263, a264=264,
...   a265=265, a266=266, a267=267, a268=268, a269=269, a270=270, a271=271,
...   a272=272, a273=273, a274=274, a275=275, a276=276, a277=277, a278=278,
...   a279=279, a280=280, a281=281, a282=282, a283=283, a284=284, a285=285,
...   a286=286, a287=287, a288=288, a289=289, a290=290, a291=291, a292=292,
...   a293=293, a294=294, a295=295, a296=296, a297=297, a298=298, a299=299)
...  # doctest: +ELLIPSIS
() [('a000', 0), ('a001', 1), ('a002', 2), ..., ('a298', 298), ('a299', 299)]

>>> class C:
...     def meth(self, *args):
...         return args
>>> obj = C()
>>> obj.meth(
...   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
...   20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
...   38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
...   56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
...   74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
...   92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
...   108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
...   122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
...   136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
...   150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
...   164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
...   178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
...   192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
...   206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
...   220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
...   234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
...   248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261,
...   262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275,
...   276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289,
...   290, 291, 292, 293, 294, 295, 296, 297, 298, 299)  # doctest: +ELLIPSIS
(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, ..., 297, 298, 299)

# >>> f(lambda x: x[0] = 3)
# Traceback (most recent call last):
# SyntaxError: expression cannot contain assignment, perhaps you meant "=="?

The grammar accepts any test (basically, any expression) in the
keyword slot of a call site.  Test a few different options.

# >>> f(x()=2)
# Traceback (most recent call last):
# SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
# >>> f(a or b=1)
# Traceback (most recent call last):
# SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
# >>> f(x.y=1)
# Traceback (most recent call last):
# SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
# >>> f((x)=2)
# Traceback (most recent call last):
# SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
# >>> f(True=2)
# Traceback (most recent call last):
# SyntaxError: cannot assign to True
>>> f(__debug__=1)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__
>>> __debug__: int
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__


More set_context():

>>> (x for x in x) += 1
Traceback (most recent call last):
SyntaxError: cannot assign to generator expression
>>> None += 1
Traceback (most recent call last):
SyntaxError: cannot assign to None
>>> __debug__ += 1
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__
>>> f() += 1
Traceback (most recent call last):
SyntaxError: cannot assign to function call


Test continue in finally in weird combinations.

continue in for loop under finally should be ok.

    >>> def test():
    ...     try:
    ...         pass
    ...     finally:
    ...         for abc in range(10):
    ...             continue
    ...     print(abc)
    >>> test()
    9

continue in a finally should be ok.

    >>> def test():
    ...    for abc in range(10):
    ...        try:
    ...            pass
    ...        finally:
    ...            continue
    ...    print(abc)
    >>> test()
    9

    >>> def test():
    ...    for abc in range(10):
    ...        try:
    ...            pass
    ...        finally:
    ...            try:
    ...                continue
    ...            except:
    ...                pass
    ...    print(abc)
    >>> test()
    9

    >>> def test():
    ...    for abc in range(10):
    ...        try:
    ...            pass
    ...        finally:
    ...            try:
    ...                pass
    ...            except:
    ...                continue
    ...    print(abc)
    >>> test()
    9

A continue outside loop should not be allowed.

    >>> def foo():
    ...     try:
    ...         pass
    ...     finally:
    ...         continue
    Traceback (most recent call last):
      ...
    SyntaxError: 'continue' not properly in loop

There is one test for a break that is not in a loop.  The compiler
uses a single data structure to keep track of try-finally and loops,
so we need to be sure that a break is actually inside a loop.  If it
isn't, there should be a syntax error.

   >>> try:
   ...     print(1)
   ...     break
   ...     print(2)
   ... finally:
   ...     print(3)
   Traceback (most recent call last):
     ...
   SyntaxError: 'break' outside loop

This raises a SyntaxError, it used to raise a SystemError.
Context for this change can be found on issue #27514

In 2.5 there was a missing exception and an assert was triggered in a debug
build.  The number of blocks must be greater than CO_MAXBLOCKS.  SF #1565514

   >>> while 1:
   ...  while 2:
   ...   while 3:
   ...    while 4:
   ...     while 5:
   ...      while 6:
   ...       while 8:
   ...        while 9:
   ...         while 10:
   ...          while 11:
   ...           while 12:
   ...            while 13:
   ...             while 14:
   ...              while 15:
   ...               while 16:
   ...                while 17:
   ...                 while 18:
   ...                  while 19:
   ...                   while 20:
   ...                    while 21:
   ...                     while 22:
   ...                      break
   Traceback (most recent call last):
     ...
   SyntaxError: too many statically nested blocks

Misuse of the nonlocal and global statement can lead to a few unique syntax errors.

   >>> def f():
   ...     print(x)
   ...     global x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is used prior to global declaration

   >>> def f():
   ...     x = 1
   ...     global x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is assigned to before global declaration

   >>> def f(x):
   ...     global x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is parameter and global

   >>> def f():
   ...     x = 1
   ...     def g():
   ...         print(x)
   ...         nonlocal x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is used prior to nonlocal declaration

   >>> def f():
   ...     x = 1
   ...     def g():
   ...         x = 2
   ...         nonlocal x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is assigned to before nonlocal declaration

   >>> def f(x):
   ...     nonlocal x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is parameter and nonlocal

   >>> def f():
   ...     global x
   ...     nonlocal x
   Traceback (most recent call last):
     ...
   SyntaxError: name 'x' is nonlocal and global

   >>> def f():
   ...     nonlocal x
   Traceback (most recent call last):
     ...
   SyntaxError: no binding for nonlocal 'x' found

From SF bug #1705365
   >>> nonlocal x
   Traceback (most recent call last):
     ...
   SyntaxError: nonlocal declaration not allowed at module level

From https://bugs.python.org/issue25973
   >>> class A:
   ...     def f(self):
   ...         nonlocal __x
   Traceback (most recent call last):
     ...
   SyntaxError: no binding for nonlocal '_A__x' found


This tests assignment-context; there was a bug in Python 2.5 where compiling
a complex 'if' (one with 'elif') would fail to notice an invalid suite,
leading to spurious errors.

   >>> if 1:
   ...   x() = 1
   ... elif 1:
   ...   pass
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   x() = 1
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call

   >>> if 1:
   ...   x() = 1
   ... elif 1:
   ...   pass
   ... else:
   ...   pass
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   x() = 1
   ... else:
   ...   pass
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   pass
   ... else:
   ...   x() = 1
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call

Make sure that the old "raise X, Y[, Z]" form is gone:
   >>> raise X, Y
   Traceback (most recent call last):
     ...
   SyntaxError: invalid syntax
   >>> raise X, Y, Z
   Traceback (most recent call last):
     ...
   SyntaxError: invalid syntax


>>> f(a=23, a=234)
Traceback (most recent call last):
   ...
SyntaxError: keyword argument repeated: a

>>> {1, 2, 3} = 42
Traceback (most recent call last):
SyntaxError: cannot assign to set display

>>> {1: 2, 3: 4} = 42
Traceback (most recent call last):
SyntaxError: cannot assign to dict display

>>> f'{x}' = 42
Traceback (most recent call last):
SyntaxError: cannot assign to f-string expression

>>> f'{x}-{y}' = 42
Traceback (most recent call last):
SyntaxError: cannot assign to f-string expression

Corner-cases that used to fail to raise the correct error:

    >>> def f(*, x=lambda __debug__:0): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

    >>> def f(*args:(lambda __debug__:0)): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

    >>> def f(**kwargs:(lambda __debug__:0)): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

    # >>> with (lambda *:0): pass
    # Traceback (most recent call last):
    # SyntaxError: named arguments must follow bare *

Corner-cases that used to crash:

    >>> def f(**__debug__): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

    >>> def f(*xx, __debug__): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

"""

import re
import sys
import unittest

from test import support

class SyntaxTestCase(unittest.TestCase):

    def _check_error(self, code, errtext,
                     filename="<testcase>", mode="exec", subclass=None, lineno=None, offset=None):
        """Check that compiling code raises SyntaxError with errtext.

        errtest is a regular expression that must be present in the
        test of the exception raised.  If subclass is specified it
        is the expected subclass of SyntaxError (e.g. IndentationError).
        """
        try:
            compile(code, filename, mode)
        except SyntaxError as err:
            if subclass and not isinstance(err, subclass):
                self.fail("SyntaxError is not a %s" % subclass.__name__)
            mo = re.search(errtext, str(err))
            if mo is None:
                self.fail("SyntaxError did not contain '%r'" % (errtext,))
            self.assertEqual(err.filename, filename)
            if lineno is not None:
                self.assertEqual(err.lineno, lineno)
            if offset is not None:
                self.assertEqual(err.offset, offset)
        else:
            self.fail("compile() did not raise SyntaxError")

    def test_assign_call(self):
        self._check_error("f() = 1", "assign")

    @support.skip_if_new_parser("Pegen does not produce a specialized error "
                                "message yet")
    def test_assign_del(self):
        self._check_error("del f()", "delete")

    def test_global_param_err_first(self):
        source = """if 1:
            def error(a):
                global a  # SyntaxError
            def error2():
                b = 1
                global b  # SyntaxError
            """
        self._check_error(source, "parameter and global", lineno=3)

    def test_nonlocal_param_err_first(self):
        source = """if 1:
            def error(a):
                nonlocal a  # SyntaxError
            def error2():
                b = 1
                global b  # SyntaxError
            """
        self._check_error(source, "parameter and nonlocal", lineno=3)

    def test_break_outside_loop(self):
        self._check_error("break", "outside loop")

    def test_yield_outside_function(self):
        self._check_error("if 0: yield",                "outside function")
        self._check_error("if 0: yield\nelse:  x=1",    "outside function")
        self._check_error("if 1: pass\nelse: yield",    "outside function")
        self._check_error("while 0: yield",             "outside function")
        self._check_error("while 0: yield\nelse:  x=1", "outside function")
        self._check_error("class C:\n  if 0: yield",    "outside function")
        self._check_error("class C:\n  if 1: pass\n  else: yield",
                          "outside function")
        self._check_error("class C:\n  while 0: yield", "outside function")
        self._check_error("class C:\n  while 0: yield\n  else:  x = 1",
                          "outside function")

    def test_return_outside_function(self):
        self._check_error("if 0: return",                "outside function")
        self._check_error("if 0: return\nelse:  x=1",    "outside function")
        self._check_error("if 1: pass\nelse: return",    "outside function")
        self._check_error("while 0: return",             "outside function")
        self._check_error("class C:\n  if 0: return",    "outside function")
        self._check_error("class C:\n  while 0: return", "outside function")
        self._check_error("class C:\n  while 0: return\n  else:  x=1",
                          "outside function")
        self._check_error("class C:\n  if 0: return\n  else: x= 1",
                          "outside function")
        self._check_error("class C:\n  if 1: pass\n  else: return",
                          "outside function")

    def test_break_outside_loop(self):
        self._check_error("if 0: break",             "outside loop")
        self._check_error("if 0: break\nelse:  x=1",  "outside loop")
        self._check_error("if 1: pass\nelse: break", "outside loop")
        self._check_error("class C:\n  if 0: break", "outside loop")
        self._check_error("class C:\n  if 1: pass\n  else: break",
                          "outside loop")

    def test_continue_outside_loop(self):
        self._check_error("if 0: continue",             "not properly in loop")
        self._check_error("if 0: continue\nelse:  x=1", "not properly in loop")
        self._check_error("if 1: pass\nelse: continue", "not properly in loop")
        self._check_error("class C:\n  if 0: continue", "not properly in loop")
        self._check_error("class C:\n  if 1: pass\n  else: continue",
                          "not properly in loop")

    def test_unexpected_indent(self):
        self._check_error("foo()\n bar()\n", "unexpected indent",
                          subclass=IndentationError)

    def test_no_indent(self):
        self._check_error("if 1:\nfoo()", "expected an indented block",
                          subclass=IndentationError)

    def test_bad_outdent(self):
        self._check_error("if 1:\n  foo()\n bar()",
                          "unindent does not match .* level",
                          subclass=IndentationError)

    def test_kwargs_last(self):
        self._check_error("int(base=10, '2')",
                          "positional argument follows keyword argument")

    def test_kwargs_last2(self):
        self._check_error("int(**{'base': 10}, '2')",
                          "positional argument follows "
                          "keyword argument unpacking")

    def test_kwargs_last3(self):
        self._check_error("int(**{'base': 10}, *['2'])",
                          "iterable argument unpacking follows "
                          "keyword argument unpacking")

def test_main():
    support.run_unittest(SyntaxTestCase)
    from test import test_syntax
    support.run_doctest(test_syntax, verbosity=True)

if __name__ == "__main__":
    test_main()
