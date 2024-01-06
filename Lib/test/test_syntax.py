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

>>> del __debug__
Traceback (most recent call last):
SyntaxError: cannot delete __debug__

>>> f() = 1
Traceback (most recent call last):
SyntaxError: cannot assign to function call here. Maybe you meant '==' instead of '='?

>>> yield = 1
Traceback (most recent call last):
SyntaxError: assignment to yield expression not possible

>>> del f()
Traceback (most recent call last):
SyntaxError: cannot delete function call

>>> a + 1 = 2
Traceback (most recent call last):
SyntaxError: cannot assign to expression here. Maybe you meant '==' instead of '='?

>>> (x for x in x) = 1
Traceback (most recent call last):
SyntaxError: cannot assign to generator expression

>>> 1 = 1
Traceback (most recent call last):
SyntaxError: cannot assign to literal here. Maybe you meant '==' instead of '='?

>>> "abc" = 1
Traceback (most recent call last):
SyntaxError: cannot assign to literal here. Maybe you meant '==' instead of '='?

>>> b"" = 1
Traceback (most recent call last):
SyntaxError: cannot assign to literal here. Maybe you meant '==' instead of '='?

>>> ... = 1
Traceback (most recent call last):
SyntaxError: cannot assign to ellipsis here. Maybe you meant '==' instead of '='?

>>> `1` = 1
Traceback (most recent call last):
SyntaxError: invalid syntax

If the left-hand side of an assignment is a list or tuple, an illegal
expression inside that contain should still cause a syntax error.
This test just checks a couple of cases rather than enumerating all of
them.

>>> (a, "b", c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to literal

>>> (a, True, c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to True

>>> (a, __debug__, c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

>>> (a, *True, c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to True

>>> (a, *__debug__, c) = (1, 2, 3)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__

>>> [a, b, c + 1] = [1, 2, 3]
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> [a, b[1], c + 1] = [1, 2, 3]
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> [a, b.c.d, c + 1] = [1, 2, 3]
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> a if 1 else b = 1
Traceback (most recent call last):
SyntaxError: cannot assign to conditional expression

>>> a = 42 if True
Traceback (most recent call last):
SyntaxError: expected 'else' after 'if' expression

>>> a = (42 if True)
Traceback (most recent call last):
SyntaxError: expected 'else' after 'if' expression

>>> a = [1, 42 if True, 4]
Traceback (most recent call last):
SyntaxError: expected 'else' after 'if' expression

>>> if True:
...     print("Hello"
...
... if 2:
...    print(123))
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> True = True = 3
Traceback (most recent call last):
SyntaxError: cannot assign to True

>>> x = y = True = z = 3
Traceback (most recent call last):
SyntaxError: cannot assign to True

>>> x = y = yield = 1
Traceback (most recent call last):
SyntaxError: assignment to yield expression not possible

>>> a, b += 1, 2
Traceback (most recent call last):
SyntaxError: 'tuple' is an illegal expression for augmented assignment

>>> (a, b) += 1, 2
Traceback (most recent call last):
SyntaxError: 'tuple' is an illegal expression for augmented assignment

>>> [a, b] += 1, 2
Traceback (most recent call last):
SyntaxError: 'list' is an illegal expression for augmented assignment

Invalid targets in `for` loops and `with` statements should also
produce a specialized error message

>>> for a() in b: pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> for (a, b()) in b: pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> for [a, b()] in b: pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> for (*a, b, c+1) in b: pass
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> for (x, *(y, z.d())) in b: pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> for a, b() in c: pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> for a, b, (c + 1, d()): pass
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> for i < (): pass
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> for a, b
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> with a as b(): pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> with a as (b, c()): pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> with a as [b, c()]: pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> with a as (*b, c, d+1): pass
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> with a as (x, *(y, z.d())): pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> with a as b, c as d(): pass
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> with a as b
Traceback (most recent call last):
SyntaxError: expected ':'

>>> p = p =
Traceback (most recent call last):
SyntaxError: invalid syntax

Comprehensions without 'in' keyword:

>>> [x for x if range(1)]
Traceback (most recent call last):
SyntaxError: 'in' expected after for-loop variables

>>> tuple(x for x if range(1))
Traceback (most recent call last):
SyntaxError: 'in' expected after for-loop variables

>>> [x for x() in a]
Traceback (most recent call last):
SyntaxError: cannot assign to function call

>>> [x for a, b, (c + 1, d()) in y]
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> [x for a, b, (c + 1, d()) if y]
Traceback (most recent call last):
SyntaxError: 'in' expected after for-loop variables

>>> [x for x+1 in y]
Traceback (most recent call last):
SyntaxError: cannot assign to expression

>>> [x for x+1, x() in y]
Traceback (most recent call last):
SyntaxError: cannot assign to expression

Comprehensions creating tuples without parentheses
should produce a specialized error message:

>>> [x,y for x,y in range(100)]
Traceback (most recent call last):
SyntaxError: did you forget parentheses around the comprehension target?

>>> {x,y for x,y in range(100)}
Traceback (most recent call last):
SyntaxError: did you forget parentheses around the comprehension target?

# Missing commas in literals collections should not
# produce special error messages regarding missing
# parentheses, but about missing commas instead

>>> [1, 2 3]
Traceback (most recent call last):
SyntaxError: invalid syntax. Perhaps you forgot a comma?

>>> {1, 2 3}
Traceback (most recent call last):
SyntaxError: invalid syntax. Perhaps you forgot a comma?

>>> {1:2, 2:5 3:12}
Traceback (most recent call last):
SyntaxError: invalid syntax. Perhaps you forgot a comma?

>>> (1, 2 3)
Traceback (most recent call last):
SyntaxError: invalid syntax. Perhaps you forgot a comma?

# Make sure soft keywords constructs don't raise specialized
# errors regarding missing commas or other spezialiced errors

>>> match x:
...     y = 3
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> match x:
...     case y:
...        3 $ 3
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> match x:
...     case $:
...        ...
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> match ...:
...     case {**rest, "key": value}:
...        ...
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> match ...:
...     case {**_}:
...        ...
Traceback (most recent call last):
SyntaxError: invalid syntax

From compiler_complex_args():

>>> def f(None=1):
...     pass
Traceback (most recent call last):
SyntaxError: invalid syntax

From ast_for_arguments():

>>> def f(x, y=1, z):
...     pass
Traceback (most recent call last):
SyntaxError: parameter without a default follows parameter with a default

>>> def f(x, /, y=1, z):
...     pass
Traceback (most recent call last):
SyntaxError: parameter without a default follows parameter with a default

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

>>> def foo(/,a,b=,c):
...    pass
Traceback (most recent call last):
SyntaxError: at least one argument must precede /

>>> def foo(a,/,/,b,c):
...    pass
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> def foo(a,/,a1,/,b,c):
...    pass
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> def foo(a=1,/,/,*b,/,c):
...    pass
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> def foo(a,/,a1=1,/,b,c):
...    pass
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> def foo(a,*b,c,/,d,e):
...    pass
Traceback (most recent call last):
SyntaxError: / must be ahead of *

>>> def foo(a=1,*b,c=3,/,d,e):
...    pass
Traceback (most recent call last):
SyntaxError: / must be ahead of *

>>> def foo(a,*b=3,c):
...    pass
Traceback (most recent call last):
SyntaxError: var-positional argument cannot have default value

>>> def foo(a,*b: int=,c):
...    pass
Traceback (most recent call last):
SyntaxError: var-positional argument cannot have default value

>>> def foo(a,**b=3):
...    pass
Traceback (most recent call last):
SyntaxError: var-keyword argument cannot have default value

>>> def foo(a,**b: int=3):
...    pass
Traceback (most recent call last):
SyntaxError: var-keyword argument cannot have default value

>>> def foo(a,*a, b, **c, d):
...    pass
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> def foo(a,*a, b, **c, d=4):
...    pass
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> def foo(a,*a, b, **c, *d):
...    pass
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> def foo(a,*a, b, **c, **d):
...    pass
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> def foo(a=1,/,**b,/,c):
...    pass
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> def foo(*b,*d):
...    pass
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> def foo(a,*b,c,*d,*e,c):
...    pass
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> def foo(a,b,/,c,*b,c,*d,*e,c):
...    pass
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> def foo(a,b,/,c,*b,c,*d,**e):
...    pass
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> def foo(a=1,/*,b,c):
...    pass
Traceback (most recent call last):
SyntaxError: expected comma between / and *

>>> def foo(a=1,d=,c):
...    pass
Traceback (most recent call last):
SyntaxError: expected default value expression

>>> def foo(a,d=,c):
...    pass
Traceback (most recent call last):
SyntaxError: expected default value expression

>>> def foo(a,d: int=,c):
...    pass
Traceback (most recent call last):
SyntaxError: expected default value expression

>>> lambda /,a,b,c: None
Traceback (most recent call last):
SyntaxError: at least one argument must precede /

>>> lambda a,/,/,b,c: None
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> lambda a,/,a1,/,b,c: None
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> lambda a=1,/,/,*b,/,c: None
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> lambda a,/,a1=1,/,b,c: None
Traceback (most recent call last):
SyntaxError: / may appear only once

>>> lambda a,*b,c,/,d,e: None
Traceback (most recent call last):
SyntaxError: / must be ahead of *

>>> lambda a=1,*b,c=3,/,d,e: None
Traceback (most recent call last):
SyntaxError: / must be ahead of *

>>> lambda a=1,/*,b,c: None
Traceback (most recent call last):
SyntaxError: expected comma between / and *

>>> lambda a,*b=3,c: None
Traceback (most recent call last):
SyntaxError: var-positional argument cannot have default value

>>> lambda a,**b=3: None
Traceback (most recent call last):
SyntaxError: var-keyword argument cannot have default value

>>> lambda a, *a, b, **c, d: None
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> lambda a,*a, b, **c, d=4: None
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> lambda a,*a, b, **c, *d: None
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> lambda a,*a, b, **c, **d: None
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> lambda a=1,/,**b,/,c: None
Traceback (most recent call last):
SyntaxError: arguments cannot follow var-keyword argument

>>> lambda *b,*d: None
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> lambda a,*b,c,*d,*e,c: None
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> lambda a,b,/,c,*b,c,*d,*e,c: None
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> lambda a,b,/,c,*b,c,*d,**e: None
Traceback (most recent call last):
SyntaxError: * argument may appear only once

>>> lambda a=1,d=,c: None
Traceback (most recent call last):
SyntaxError: expected default value expression

>>> lambda a,d=,c: None
Traceback (most recent call last):
SyntaxError: expected default value expression

>>> lambda a,d=3,c: None
Traceback (most recent call last):
SyntaxError: parameter without a default follows parameter with a default

>>> lambda a,/,d=3,c: None
Traceback (most recent call last):
SyntaxError: parameter without a default follows parameter with a default

>>> import ast; ast.parse('''
... def f(
...     *, # type: int
...     a, # type: int
... ):
...     pass
... ''', type_comments=True)
Traceback (most recent call last):
SyntaxError: bare * has associated type comment


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
>>> f(L, x for x in L)
Traceback (most recent call last):
SyntaxError: Generator expression must be parenthesized
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

>>> f(lambda x: x[0] = 3)
Traceback (most recent call last):
SyntaxError: expression cannot contain assignment, perhaps you meant "=="?

# Check that this error doesn't trigger for names:
>>> f(a={x: for x in {}})
Traceback (most recent call last):
SyntaxError: invalid syntax

The grammar accepts any test (basically, any expression) in the
keyword slot of a call site.  Test a few different options.

>>> f(x()=2)
Traceback (most recent call last):
SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
>>> f(a or b=1)
Traceback (most recent call last):
SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
>>> f(x.y=1)
Traceback (most recent call last):
SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
>>> f((x)=2)
Traceback (most recent call last):
SyntaxError: expression cannot contain assignment, perhaps you meant "=="?
>>> f(True=1)
Traceback (most recent call last):
SyntaxError: cannot assign to True
>>> f(False=1)
Traceback (most recent call last):
SyntaxError: cannot assign to False
>>> f(None=1)
Traceback (most recent call last):
SyntaxError: cannot assign to None
>>> f(__debug__=1)
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__
>>> __debug__: int
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__
>>> f(a=)
Traceback (most recent call last):
SyntaxError: expected argument value expression
>>> f(a, b, c=)
Traceback (most recent call last):
SyntaxError: expected argument value expression
>>> f(a, b, c=, d)
Traceback (most recent call last):
SyntaxError: expected argument value expression
>>> f(*args=[0])
Traceback (most recent call last):
SyntaxError: cannot assign to iterable argument unpacking
>>> f(a, b, *args=[0])
Traceback (most recent call last):
SyntaxError: cannot assign to iterable argument unpacking
>>> f(**kwargs={'a': 1})
Traceback (most recent call last):
SyntaxError: cannot assign to keyword argument unpacking
>>> f(a, b, *args, **kwargs={'a': 1})
Traceback (most recent call last):
SyntaxError: cannot assign to keyword argument unpacking


More set_context():

>>> (x for x in x) += 1
Traceback (most recent call last):
SyntaxError: 'generator expression' is an illegal expression for augmented assignment
>>> None += 1
Traceback (most recent call last):
SyntaxError: 'None' is an illegal expression for augmented assignment
>>> __debug__ += 1
Traceback (most recent call last):
SyntaxError: cannot assign to __debug__
>>> f() += 1
Traceback (most recent call last):
SyntaxError: 'function call' is an illegal expression for augmented assignment


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
   SyntaxError: cannot assign to function call here. Maybe you meant '==' instead of '='?

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   x() = 1
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call here. Maybe you meant '==' instead of '='?

   >>> if 1:
   ...   x() = 1
   ... elif 1:
   ...   pass
   ... else:
   ...   pass
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call here. Maybe you meant '==' instead of '='?

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   x() = 1
   ... else:
   ...   pass
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call here. Maybe you meant '==' instead of '='?

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   pass
   ... else:
   ...   x() = 1
   Traceback (most recent call last):
     ...
   SyntaxError: cannot assign to function call here. Maybe you meant '==' instead of '='?

Missing ':' before suites:

   >>> def f()
   ...     pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> def f[T]()
   ...     pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> class A
   ...     pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> class A[T]
   ...     pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> class A[T]()
   ...     pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> class R&D:
   ...     pass
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> if 1
   ...   pass
   ... elif 1:
   ...   pass
   ... else:
   ...   x() = 1
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> if 1:
   ...   pass
   ... elif 1
   ...   pass
   ... else:
   ...   x() = 1
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> if 1:
   ...   pass
   ... elif 1:
   ...   pass
   ... else
   ...   x() = 1
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> for x in range(10)
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> for x in range 10:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> while True
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with blech as something
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with blech
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with blech, block as something
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with blech, block as something, bluch
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with (blech as something)
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with (blech)
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with (blech, block as something)
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with (blech, block as something, bluch)
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> with block ad something:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> try
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> try:
   ...   pass
   ... except
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> match x
   ...   case list():
   ...       pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> match x x:
   ...   case list():
   ...       pass
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> match x:
   ...   case list()
   ...       pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> match x:
   ...   case [y] if y > 0
   ...       pass
   Traceback (most recent call last):
   SyntaxError: expected ':'

   >>> if x = 3:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

   >>> while x = 3:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

   >>> if x.a = 3:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: cannot assign to attribute here. Maybe you meant '==' instead of '='?

   >>> while x.a = 3:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: cannot assign to attribute here. Maybe you meant '==' instead of '='?


Missing parens after function definition

   >>> def f:
   Traceback (most recent call last):
   SyntaxError: expected '('

   >>> async def f:
   Traceback (most recent call last):
   SyntaxError: expected '('

Parenthesized arguments in function definitions

   >>> def f(x, (y, z), w):
   ...    pass
   Traceback (most recent call last):
   SyntaxError: Function parameters cannot be parenthesized

   >>> def f((x, y, z, w)):
   ...    pass
   Traceback (most recent call last):
   SyntaxError: Function parameters cannot be parenthesized

   >>> def f(x, (y, z, w)):
   ...    pass
   Traceback (most recent call last):
   SyntaxError: Function parameters cannot be parenthesized

   >>> def f((x, y, z), w):
   ...    pass
   Traceback (most recent call last):
   SyntaxError: Function parameters cannot be parenthesized

   >>> lambda x, (y, z), w: None
   Traceback (most recent call last):
   SyntaxError: Lambda expression parameters cannot be parenthesized

   >>> lambda (x, y, z, w): None
   Traceback (most recent call last):
   SyntaxError: Lambda expression parameters cannot be parenthesized

   >>> lambda x, (y, z, w): None
   Traceback (most recent call last):
   SyntaxError: Lambda expression parameters cannot be parenthesized

   >>> lambda (x, y, z), w: None
   Traceback (most recent call last):
   SyntaxError: Lambda expression parameters cannot be parenthesized

Custom error messages for try blocks that are not followed by except/finally

   >>> try:
   ...    x = 34
   ...
   Traceback (most recent call last):
   SyntaxError: expected 'except' or 'finally' block

Custom error message for try block mixing except and except*

   >>> try:
   ...    pass
   ... except TypeError:
   ...    pass
   ... except* ValueError:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: cannot have both 'except' and 'except*' on the same 'try'

   >>> try:
   ...    pass
   ... except* TypeError:
   ...    pass
   ... except ValueError:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: cannot have both 'except' and 'except*' on the same 'try'

   >>> try:
   ...    pass
   ... except TypeError:
   ...    pass
   ... except TypeError:
   ...    pass
   ... except* ValueError:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: cannot have both 'except' and 'except*' on the same 'try'

   >>> try:
   ...    pass
   ... except* TypeError:
   ...    pass
   ... except* TypeError:
   ...    pass
   ... except ValueError:
   ...    pass
   Traceback (most recent call last):
   SyntaxError: cannot have both 'except' and 'except*' on the same 'try'

Ensure that early = are not matched by the parser as invalid comparisons
   >>> f(2, 4, x=34); 1 $ 2
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> dict(x=34); x $ y
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> dict(x=34, (x for x in range 10), 1); x $ y
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> dict(x=34, x=1, y=2); x $ y
   Traceback (most recent call last):
   SyntaxError: invalid syntax

Incomplete dictionary literals

   >>> {1:2, 3:4, 5}
   Traceback (most recent call last):
   SyntaxError: ':' expected after dictionary key

   >>> {1:2, 3:4, 5:}
   Traceback (most recent call last):
   SyntaxError: expression expected after dictionary key and ':'

   >>> {1: *12+1, 23: 1}
   Traceback (most recent call last):
   SyntaxError: cannot use a starred expression in a dictionary value

   >>> {1: *12+1}
   Traceback (most recent call last):
   SyntaxError: cannot use a starred expression in a dictionary value

   >>> {1: 23, 1: *12+1}
   Traceback (most recent call last):
   SyntaxError: cannot use a starred expression in a dictionary value

   >>> {1:}
   Traceback (most recent call last):
   SyntaxError: expression expected after dictionary key and ':'

   # Ensure that the error is not raised for syntax errors that happen after sets

   >>> {1} $
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   # Ensure that the error is not raised for invalid expressions

   >>> {1: 2, 3: foo(,), 4: 5}
   Traceback (most recent call last):
   SyntaxError: invalid syntax

   >>> {1: $, 2: 3}
   Traceback (most recent call last):
   SyntaxError: invalid syntax

Specialized indentation errors:

   >>> while condition:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'while' statement on line 1

   >>> for x in range(10):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'for' statement on line 1

   >>> for x in range(10):
   ...     pass
   ... else:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'else' statement on line 3

   >>> async for x in range(10):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'for' statement on line 1

   >>> async for x in range(10):
   ...     pass
   ... else:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'else' statement on line 3

   >>> if something:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'if' statement on line 1

   >>> if something:
   ...     pass
   ... elif something_else:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'elif' statement on line 3

   >>> if something:
   ...     pass
   ... elif something_else:
   ...     pass
   ... else:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'else' statement on line 5

   >>> try:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'try' statement on line 1

   >>> try:
   ...     something()
   ... except:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'except' statement on line 3

   >>> try:
   ...     something()
   ... except A:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'except' statement on line 3

   >>> try:
   ...     something()
   ... except* A:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'except*' statement on line 3

   >>> try:
   ...     something()
   ... except A:
   ...     pass
   ... finally:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'finally' statement on line 5

   >>> try:
   ...     something()
   ... except* A:
   ...     pass
   ... finally:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'finally' statement on line 5

   >>> with A:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'with' statement on line 1

   >>> with A as a, B as b:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'with' statement on line 1

   >>> with (A as a, B as b):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'with' statement on line 1

   >>> async with A:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'with' statement on line 1

   >>> async with A as a, B as b:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'with' statement on line 1

   >>> async with (A as a, B as b):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'with' statement on line 1

   >>> def foo(x, /, y, *, z=2):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after function definition on line 1

   >>> def foo[T](x, /, y, *, z=2):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after function definition on line 1

   >>> class Blech(A):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after class definition on line 1

   >>> class Blech[T](A):
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after class definition on line 1

   >>> match something:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'match' statement on line 1

   >>> match something:
   ...     case []:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'case' statement on line 2

   >>> match something:
   ...     case []:
   ...         ...
   ...     case {}:
   ... pass
   Traceback (most recent call last):
   IndentationError: expected an indented block after 'case' statement on line 4

Make sure that the old "raise X, Y[, Z]" form is gone:
   >>> raise X, Y
   Traceback (most recent call last):
     ...
   SyntaxError: invalid syntax
   >>> raise X, Y, Z
   Traceback (most recent call last):
     ...
   SyntaxError: invalid syntax

Check that an multiple exception types with missing parentheses
raise a custom exception

   >>> try:
   ...   pass
   ... except A, B:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

   >>> try:
   ...   pass
   ... except A, B, C:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

   >>> try:
   ...   pass
   ... except A, B, C as blech:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

   >>> try:
   ...   pass
   ... except A, B, C as blech:
   ...   pass
   ... finally:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized


   >>> try:
   ...   pass
   ... except* A, B:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

   >>> try:
   ...   pass
   ... except* A, B, C:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

   >>> try:
   ...   pass
   ... except* A, B, C as blech:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

   >>> try:
   ...   pass
   ... except* A, B, C as blech:
   ...   pass
   ... finally:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: multiple exception types must be parenthesized

Custom exception for 'except*' without an exception type

   >>> try:
   ...   pass
   ... except* A as a:
   ...   pass
   ... except*:
   ...   pass
   Traceback (most recent call last):
   SyntaxError: expected one or more exception types


>>> f(a=23, a=234)
Traceback (most recent call last):
   ...
SyntaxError: keyword argument repeated: a

>>> {1, 2, 3} = 42
Traceback (most recent call last):
SyntaxError: cannot assign to set display here. Maybe you meant '==' instead of '='?

>>> {1: 2, 3: 4} = 42
Traceback (most recent call last):
SyntaxError: cannot assign to dict literal here. Maybe you meant '==' instead of '='?

>>> f'{x}' = 42
Traceback (most recent call last):
SyntaxError: cannot assign to f-string expression here. Maybe you meant '==' instead of '='?

>>> f'{x}-{y}' = 42
Traceback (most recent call last):
SyntaxError: cannot assign to f-string expression here. Maybe you meant '==' instead of '='?

>>> (x, y, z=3, d, e)
Traceback (most recent call last):
SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

>>> [x, y, z=3, d, e]
Traceback (most recent call last):
SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

>>> [z=3]
Traceback (most recent call last):
SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

>>> {x, y, z=3, d, e}
Traceback (most recent call last):
SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

>>> {z=3}
Traceback (most recent call last):
SyntaxError: invalid syntax. Maybe you meant '==' or ':=' instead of '='?

>>> from t import x,
Traceback (most recent call last):
SyntaxError: trailing comma not allowed without surrounding parentheses

>>> from t import x,y,
Traceback (most recent call last):
SyntaxError: trailing comma not allowed without surrounding parentheses

>>> import a from b
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a.y.z from b.y.z
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a from b as bar
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a.y.z from b.y.z as bar
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a, b,c from b
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a.y.z, b.y.z, c.y.z from b.y.z
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a,b,c from b as bar
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

>>> import a.y.z, b.y.z, c.y.z from b.y.z as bar
Traceback (most recent call last):
SyntaxError: Did you mean to use 'from ... import ...' instead?

# Check that we dont raise the "trailing comma" error if there is more
# input to the left of the valid part that we parsed.

>>> from t import x,y, and 3
Traceback (most recent call last):
SyntaxError: invalid syntax

>>> (): int
Traceback (most recent call last):
SyntaxError: only single target (not tuple) can be annotated
>>> []: int
Traceback (most recent call last):
SyntaxError: only single target (not list) can be annotated
>>> (()): int
Traceback (most recent call last):
SyntaxError: only single target (not tuple) can be annotated
>>> ([]): int
Traceback (most recent call last):
SyntaxError: only single target (not list) can be annotated

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

    >>> with (lambda *:0): pass
    Traceback (most recent call last):
    SyntaxError: named arguments must follow bare *

Corner-cases that used to crash:

    >>> def f(**__debug__): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

    >>> def f(*xx, __debug__): pass
    Traceback (most recent call last):
    SyntaxError: cannot assign to __debug__

    >>> import ä £
    Traceback (most recent call last):
    SyntaxError: invalid character '£' (U+00A3)

  Invalid pattern matching constructs:

    >>> match ...:
    ...   case 42 as _:
    ...     ...
    Traceback (most recent call last):
    SyntaxError: cannot use '_' as a target

    >>> match ...:
    ...   case 42 as 1+2+4:
    ...     ...
    Traceback (most recent call last):
    SyntaxError: invalid pattern target

    >>> match ...:
    ...   case Foo(z=1, y=2, x):
    ...     ...
    Traceback (most recent call last):
    SyntaxError: positional patterns follow keyword patterns

    >>> match ...:
    ...   case Foo(a, z=1, y=2, x):
    ...     ...
    Traceback (most recent call last):
    SyntaxError: positional patterns follow keyword patterns

    >>> match ...:
    ...   case Foo(z=1, x, y=2):
    ...     ...
    Traceback (most recent call last):
    SyntaxError: positional patterns follow keyword patterns

    >>> match ...:
    ...   case C(a=b, c, d=e, f, g=h, i, j=k, ...):
    ...     ...
    Traceback (most recent call last):
    SyntaxError: positional patterns follow keyword patterns

Non-matching 'elif'/'else' statements:

    >>> if a == b:
    ...     ...
    ...     elif a == c:
    Traceback (most recent call last):
    SyntaxError: 'elif' must match an if-statement here

    >>> if x == y:
    ...     ...
    ...     else:
    Traceback (most recent call last):
    SyntaxError: 'else' must match a valid statement here

    >>> elif m == n:
    Traceback (most recent call last):
    SyntaxError: 'elif' must match an if-statement here

    >>> else:
    Traceback (most recent call last):
    SyntaxError: 'else' must match a valid statement here

Uses of the star operator which should fail:

A[:*b]

    >>> A[:*b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[:(*b)]
    Traceback (most recent call last):
        ...
    SyntaxError: cannot use starred expression here
    >>> A[:*b] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> del A[:*b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[*b:]

    >>> A[*b:]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[(*b):]
    Traceback (most recent call last):
        ...
    SyntaxError: cannot use starred expression here
    >>> A[*b:] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> del A[*b:]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[*b:*b]

    >>> A[*b:*b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[(*b:*b)]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[*b:*b] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> del A[*b:*b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[*(1:2)]

    >>> A[*(1:2)]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[*(1:2)] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> del A[*(1:2)]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[*:] and A[:*]

    >>> A[*:]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[:*]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[*]

    >>> A[*]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[**]

    >>> A[**]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

A[**b]

    >>> A[**b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[**b] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> del A[**b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

def f(x: *b)

    >>> def f6(x: *b): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> def f7(x: *b = 1): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

**kwargs: *a

    >>> def f8(**kwargs: *a): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

x: *b

    >>> x: *b
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> x: *b = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

Invalid bytes literals:

   >>> b"Ā"
   Traceback (most recent call last):
      ...
       b"Ā"
        ^^^
   SyntaxError: bytes can only contain ASCII literal characters

   >>> b"абвгде"
   Traceback (most recent call last):
      ...
       b"абвгде"
        ^^^^^^^^
   SyntaxError: bytes can only contain ASCII literal characters

   >>> b"abc ъющый"  # first 3 letters are ascii
   Traceback (most recent call last):
      ...
       b"abc ъющый"
        ^^^^^^^^^^^
   SyntaxError: bytes can only contain ASCII literal characters

Invalid expressions in type scopes:

   >>> type A[T: (x:=3)] = int
   Traceback (most recent call last):
      ...
   SyntaxError: named expression cannot be used within a TypeVar bound

   >>> type A[T: (yield 3)] = int
   Traceback (most recent call last):
      ...
   SyntaxError: yield expression cannot be used within a TypeVar bound

   >>> type A[T: (await 3)] = int
   Traceback (most recent call last):
      ...
   SyntaxError: await expression cannot be used within a TypeVar bound

   >>> type A[T: (yield from [])] = int
   Traceback (most recent call last):
      ...
   SyntaxError: yield expression cannot be used within a TypeVar bound

   >>> type A = (x := 3)
   Traceback (most recent call last):
      ...
   SyntaxError: named expression cannot be used within a type alias

   >>> type A = (yield 3)
   Traceback (most recent call last):
      ...
   SyntaxError: yield expression cannot be used within a type alias

   >>> type A = (await 3)
   Traceback (most recent call last):
      ...
   SyntaxError: await expression cannot be used within a type alias

   >>> type A = (yield from [])
   Traceback (most recent call last):
      ...
   SyntaxError: yield expression cannot be used within a type alias

   >>> class A[T]((x := 3)): ...
   Traceback (most recent call last):
      ...
   SyntaxError: named expression cannot be used within the definition of a generic

   >>> class A[T]((yield 3)): ...
   Traceback (most recent call last):
      ...
   SyntaxError: yield expression cannot be used within the definition of a generic

   >>> class A[T]((await 3)): ...
   Traceback (most recent call last):
      ...
   SyntaxError: await expression cannot be used within the definition of a generic

   >>> class A[T]((yield from [])): ...
   Traceback (most recent call last):
      ...
   SyntaxError: yield expression cannot be used within the definition of a generic

    >>> f(**x, *y)
    Traceback (most recent call last):
    SyntaxError: iterable argument unpacking follows keyword argument unpacking

    >>> f(**x, *)
    Traceback (most recent call last):
    SyntaxError: iterable argument unpacking follows keyword argument unpacking

    >>> f(x, *:)
    Traceback (most recent call last):
    SyntaxError: invalid syntax
"""

import re
import doctest
import textwrap
import unittest

from test import support

class SyntaxTestCase(unittest.TestCase):

    def _check_error(self, code, errtext,
                     filename="<testcase>", mode="exec", subclass=None,
                     lineno=None, offset=None, end_lineno=None, end_offset=None):
        """Check that compiling code raises SyntaxError with errtext.

        errtext is a regular expression that must be present in the
        test of the exception raised. If subclass is specified, it
        is the expected subclass of SyntaxError (e.g. IndentationError).
        """
        try:
            compile(code, filename, mode)
        except SyntaxError as err:
            if subclass and not isinstance(err, subclass):
                self.fail("SyntaxError is not a %s" % subclass.__name__)
            mo = re.search(errtext, str(err))
            if mo is None:
                self.fail("SyntaxError did not contain %r" % (errtext,))
            self.assertEqual(err.filename, filename)
            if lineno is not None:
                self.assertEqual(err.lineno, lineno)
            if offset is not None:
                self.assertEqual(err.offset, offset)
            if end_lineno is not None:
                self.assertEqual(err.end_lineno, end_lineno)
            if end_offset is not None:
                self.assertEqual(err.end_offset, end_offset)

        else:
            self.fail("compile() did not raise SyntaxError")

    def _check_noerror(self, code,
                       errtext="compile() raised unexpected SyntaxError",
                       filename="<testcase>", mode="exec", subclass=None):
        """Check that compiling code does not raise a SyntaxError.

        errtext is the message passed to self.fail if there is
        a SyntaxError. If the subclass parameter is specified,
        it is the subclass of SyntaxError (e.g. IndentationError)
        that the raised error is checked against.
        """
        try:
            compile(code, filename, mode)
        except SyntaxError as err:
            if (not subclass) or isinstance(err, subclass):
                self.fail(errtext)

    def test_expression_with_assignment(self):
        self._check_error(
            "print(end1 + end2 = ' ')",
            'expression cannot contain assignment, perhaps you meant "=="?',
            offset=7
        )

    def test_curly_brace_after_primary_raises_immediately(self):
        self._check_error("f{}", "invalid syntax", mode="single")

    def test_assign_call(self):
        self._check_error("f() = 1", "assign")

    def test_assign_del(self):
        self._check_error("del (,)", "invalid syntax")
        self._check_error("del 1", "cannot delete literal")
        self._check_error("del (1, 2)", "cannot delete literal")
        self._check_error("del None", "cannot delete None")
        self._check_error("del *x", "cannot delete starred")
        self._check_error("del (*x)", "cannot use starred expression")
        self._check_error("del (*x,)", "cannot delete starred")
        self._check_error("del [*x,]", "cannot delete starred")
        self._check_error("del f()", "cannot delete function call")
        self._check_error("del f(a, b)", "cannot delete function call")
        self._check_error("del o.f()", "cannot delete function call")
        self._check_error("del a[0]()", "cannot delete function call")
        self._check_error("del x, f()", "cannot delete function call")
        self._check_error("del f(), x", "cannot delete function call")
        self._check_error("del [a, b, ((c), (d,), e.f())]", "cannot delete function call")
        self._check_error("del (a if True else b)", "cannot delete conditional")
        self._check_error("del +a", "cannot delete expression")
        self._check_error("del a, +b", "cannot delete expression")
        self._check_error("del a + b", "cannot delete expression")
        self._check_error("del (a + b, c)", "cannot delete expression")
        self._check_error("del (c[0], a + b)", "cannot delete expression")
        self._check_error("del a.b.c + 2", "cannot delete expression")
        self._check_error("del a.b.c[0] + 2", "cannot delete expression")
        self._check_error("del (a, b, (c, d.e.f + 2))", "cannot delete expression")
        self._check_error("del [a, b, (c, d.e.f[0] + 2)]", "cannot delete expression")
        self._check_error("del (a := 5)", "cannot delete named expression")
        # We don't have a special message for this, but make sure we don't
        # report "cannot delete name"
        self._check_error("del a += b", "invalid syntax")

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
        msg = "outside loop"
        self._check_error("break", msg, lineno=1)
        self._check_error("if 0: break", msg, lineno=1)
        self._check_error("if 0: break\nelse:  x=1", msg, lineno=1)
        self._check_error("if 1: pass\nelse: break", msg, lineno=2)
        self._check_error("class C:\n  if 0: break", msg, lineno=2)
        self._check_error("class C:\n  if 1: pass\n  else: break",
                          msg, lineno=3)
        self._check_error("with object() as obj:\n break",
                          msg, lineno=2)

    def test_continue_outside_loop(self):
        msg = "not properly in loop"
        self._check_error("if 0: continue", msg, lineno=1)
        self._check_error("if 0: continue\nelse:  x=1", msg, lineno=1)
        self._check_error("if 1: pass\nelse: continue", msg, lineno=2)
        self._check_error("class C:\n  if 0: continue", msg, lineno=2)
        self._check_error("class C:\n  if 1: pass\n  else: continue",
                          msg, lineno=3)
        self._check_error("with object() as obj:\n    continue",
                          msg, lineno=2)

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

    def test_generator_in_function_call(self):
        self._check_error("foo(x,    y for y in range(3) for z in range(2) if z    , p)",
                          "Generator expression must be parenthesized",
                          lineno=1, end_lineno=1, offset=11, end_offset=53)

    def test_except_then_except_star(self):
        self._check_error("try: pass\nexcept ValueError: pass\nexcept* TypeError: pass",
                          r"cannot have both 'except' and 'except\*' on the same 'try'",
                          lineno=3, end_lineno=3, offset=1, end_offset=8)

    def test_except_star_then_except(self):
        self._check_error("try: pass\nexcept* ValueError: pass\nexcept TypeError: pass",
                          r"cannot have both 'except' and 'except\*' on the same 'try'",
                          lineno=3, end_lineno=3, offset=1, end_offset=7)

    def test_empty_line_after_linecont(self):
        # See issue-40847
        s = r"""\
pass
        \

pass
"""
        try:
            compile(s, '<string>', 'exec')
        except SyntaxError:
            self.fail("Empty line after a line continuation character is valid.")

        # See issue-46091
        s1 = r"""\
def fib(n):
    \
'''Print a Fibonacci series up to n.'''
    \
a, b = 0, 1
"""
        s2 = r"""\
def fib(n):
    '''Print a Fibonacci series up to n.'''
    a, b = 0, 1
"""
        try:
            compile(s1, '<string>', 'exec')
            compile(s2, '<string>', 'exec')
        except SyntaxError:
            self.fail("Indented statement over multiple lines is valid")

    def test_continuation_bad_indentation(self):
        # Check that code that breaks indentation across multiple lines raises a syntax error

        code = r"""\
if x:
    y = 1
  \
  foo = 1
        """

        self.assertRaises(IndentationError, exec, code)

    @support.cpython_only
    def test_nested_named_except_blocks(self):
        code = ""
        for i in range(12):
            code += f"{'    '*i}try:\n"
            code += f"{'    '*(i+1)}raise Exception\n"
            code += f"{'    '*i}except Exception as e:\n"
        code += f"{' '*4*12}pass"
        self._check_error(code, "too many statically nested blocks")

    @support.cpython_only
    def test_with_statement_many_context_managers(self):
        # See gh-113297

        def get_code(n):
            code = textwrap.dedent("""
                def bug():
                    with (
                    a
                """)
            for i in range(n):
                code += f"    as a{i}, a\n"
            code += "): yield a"
            return code

        CO_MAXBLOCKS = 20  # static nesting limit of the compiler

        for n in range(CO_MAXBLOCKS):
            with self.subTest(f"within range: {n=}"):
                compile(get_code(n), "<string>", "exec")

        for n in range(CO_MAXBLOCKS, CO_MAXBLOCKS + 5):
            with self.subTest(f"out of range: {n=}"):
                self._check_error(get_code(n), "too many statically nested blocks")

    def test_barry_as_flufl_with_syntax_errors(self):
        # The "barry_as_flufl" rule can produce some "bugs-at-a-distance" if
        # is reading the wrong token in the presence of syntax errors later
        # in the file. See bpo-42214 for more information.
        code = """
def func1():
    if a != b:
        raise ValueError

def func2():
    try
        return 1
    finally:
        pass
"""
        self._check_error(code, "expected ':'")

    def test_invalid_line_continuation_error_position(self):
        self._check_error(r"a = 3 \ 4",
                          "unexpected character after line continuation character",
                          lineno=1, offset=8)
        self._check_error('1,\\#\n2',
                          "unexpected character after line continuation character",
                          lineno=1, offset=4)
        self._check_error('\nfgdfgf\n1,\\#\n2\n',
                          "unexpected character after line continuation character",
                          lineno=3, offset=4)

    def test_invalid_line_continuation_left_recursive(self):
        # Check bpo-42218: SyntaxErrors following left-recursive rules
        # (t_primary_raw in this case) need to be tested explicitly
        self._check_error("A.\u018a\\ ",
                          "unexpected character after line continuation character")
        self._check_error("A.\u03bc\\\n",
                          "unexpected EOF while parsing")

    def test_error_parenthesis(self):
        for paren in "([{":
            self._check_error(paren + "1 + 2", f"\\{paren}' was never closed")

        for paren in "([{":
            self._check_error(f"a = {paren} 1, 2, 3\nb=3", f"\\{paren}' was never closed")

        for paren in ")]}":
            self._check_error(paren + "1 + 2", f"unmatched '\\{paren}'")

        # Some more complex examples:
        code = """\
func(
    a=["unclosed], # Need a quote in this comment: "
    b=2,
)
"""
        self._check_error(code, "parenthesis '\\)' does not match opening parenthesis '\\['")

        self._check_error("match y:\n case e(e=v,v,", " was never closed")

        # Examples with dencodings
        s = b'# coding=latin\n(aaaaaaaaaaaaaaaaa\naaaaaaaaaaa\xb5'
        self._check_error(s, r"'\(' was never closed")

    def test_error_string_literal(self):

        self._check_error("'blech", r"unterminated string literal \(.*\)$")
        self._check_error('"blech', r"unterminated string literal \(.*\)$")
        self._check_error(
            r'"blech\"', r"unterminated string literal \(.*\); perhaps you escaped the end quote"
        )
        self._check_error(
            r'r"blech\"', r"unterminated string literal \(.*\); perhaps you escaped the end quote"
        )
        self._check_error("'''blech", "unterminated triple-quoted string literal")
        self._check_error('"""blech', "unterminated triple-quoted string literal")

    def test_invisible_characters(self):
        self._check_error('print\x17("Hello")', "invalid non-printable character")
        self._check_error(b"with(0,,):\n\x01", "invalid non-printable character")

    def test_match_call_does_not_raise_syntax_error(self):
        code = """
def match(x):
    return 1+1

match(34)
"""
        compile(code, "<string>", "exec")

    def test_case_call_does_not_raise_syntax_error(self):
        code = """
def case(x):
    return 1+1

case(34)
"""
        compile(code, "<string>", "exec")

    def test_multiline_compiler_error_points_to_the_end(self):
        self._check_error(
            "call(\na=1,\na=1\n)",
            "keyword argument repeated",
            lineno=3
        )

    @support.cpython_only
    def test_syntax_error_on_deeply_nested_blocks(self):
        # This raises a SyntaxError, it used to raise a SystemError. Context
        # for this change can be found on issue #27514

        # In 2.5 there was a missing exception and an assert was triggered in a
        # debug build.  The number of blocks must be greater than CO_MAXBLOCKS.
        # SF #1565514

        source = """
while 1:
 while 2:
  while 3:
   while 4:
    while 5:
     while 6:
      while 8:
       while 9:
        while 10:
         while 11:
          while 12:
           while 13:
            while 14:
             while 15:
              while 16:
               while 17:
                while 18:
                 while 19:
                  while 20:
                   while 21:
                    while 22:
                     break
"""
        self._check_error(source, "too many statically nested blocks")

    def test_syntax_error_non_matching_elif_else_statements(self):
        # Check bpo-45759: 'elif' statements that doesn't match an
        # if-statement or 'else' statements that doesn't match any
        # valid else-able statement (e.g. 'while')
        self._check_error(
            "elif m == n:\n    ...",
            "'elif' must match an if-statement here")
        self._check_error(
            "else:\n    ...",
            "'else' must match a valid statement here")
        self._check_noerror("if a == b:\n    ...\nelif a == c:\n    ...")
        self._check_noerror("if x == y:\n    ...\nelse:\n    ...")
        self._check_error(
            "else = 123",
            "invalid syntax")
        self._check_error(
            "elif 55 = 123",
            "cannot assign to literal here")

    @support.cpython_only
    def test_error_on_parser_stack_overflow(self):
        source = "-" * 100000 + "4"
        for mode in ["exec", "eval", "single"]:
            with self.subTest(mode=mode):
                with self.assertRaisesRegex(MemoryError, r"too complex"):
                    compile(source, "<string>", mode)

    @support.cpython_only
    def test_deep_invalid_rule(self):
        # Check that a very deep invalid rule in the PEG
        # parser doesn't have exponential backtracking.
        source = "d{{{{{{{{{{{{{{{{{{{{{{{{{```{{{{{{{ef f():y"
        with self.assertRaises(SyntaxError):
            compile(source, "<string>", "exec")


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
