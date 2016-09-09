"""Tests for the com2ann.py script in the Tools/parser directory."""

import unittest
import test.support
import os
import re

from test.test_tools import basepath, toolsdir, skip_if_missing

skip_if_missing()

parser_path = os.path.join(toolsdir, "parser")

with test.support.DirsOnSysPath(parser_path):
    from com2ann import *

class BaseTestCase(unittest.TestCase):

    def check(self, code, expected, n=False, e=False):
        self.assertEqual(com2ann(code,
                         drop_None=n, drop_Ellipsis=e, silent=True),
                         expected)

class SimpleTestCase(BaseTestCase):
    # Tests for basic conversions

    def test_basics(self):
        self.check("z = 5", "z = 5")
        self.check("z: int = 5", "z: int = 5")
        self.check("z = 5 # type: int", "z: int = 5")
        self.check("z = 5 # type: int # comment",
                   "z: int = 5 # comment")

    def test_type_ignore(self):
        self.check("foobar = foobaz() #type: ignore",
                   "foobar = foobaz() #type: ignore")
        self.check("a = 42 #type: ignore #comment",
                   "a = 42 #type: ignore #comment")

    def test_complete_tuple(self):
        self.check("t = 1, 2, 3 # type: Tuple[int, ...]",
                   "t: Tuple[int, ...] = (1, 2, 3)")
        self.check("t = 1, # type: Tuple[int]",
                   "t: Tuple[int] = (1,)")
        self.check("t = (1, 2, 3) # type: Tuple[int, ...]",
                   "t: Tuple[int, ...] = (1, 2, 3)")

    def test_drop_None(self):
        self.check("x = None # type: int",
                   "x: int", True)
        self.check("x = None # type: int # another",
                   "x: int # another", True)
        self.check("x = None # type: int # None",
                   "x: int # None", True)

    def test_drop_Ellipsis(self):
        self.check("x = ... # type: int",
                   "x: int", False, True)
        self.check("x = ... # type: int # another",
                   "x: int # another", False, True)
        self.check("x = ... # type: int # ...",
                   "x: int # ...", False, True)

    def test_newline(self):
        self.check("z = 5 # type: int\r\n", "z: int = 5\r\n")
        self.check("z = 5 # type: int # comment\x85",
                   "z: int = 5 # comment\x85")

    def test_wrong(self):
        self.check("#type : str", "#type : str")
        self.check("x==y #type: bool", "x==y #type: bool")

    def test_pattern(self):
        for line in ["#type: int", "  # type:  str[:] # com"]:
            self.assertTrue(re.search(TYPE_COM, line))
        for line in ["", "#", "# comment", "#type", "type int:"]:
            self.assertFalse(re.search(TYPE_COM, line))

class BigTestCase(BaseTestCase):
    # Tests for really crazy formatting, to be sure
    # that script works reasonably in extreme situations

    def test_crazy(self):
        self.maxDiff = None
        self.check(crazy_code, big_result, False, False)
        self.check(crazy_code, big_result_ne, True, True)

crazy_code = """\
# -*- coding: utf-8 -*- # this should not be spoiled
'''
Docstring here
'''

import testmod
x = 5 #type    : int # this one is OK
ttt \\
    = \\
        1.0, \\
        2.0, \\
        3.0, #type: Tuple[float, float, float]
with foo(x==1) as f: #type: str
    print(f)

for i, j in my_inter(x=1): # type: ignore
    i + j # type: int # what about this

x = y = z = 1 # type: int
x, y, z = [], [], []  # type: (List[int], List[int], List[str])
class C:


    l[f(x
        =1)] = [

         1,
         2,
         ]  # type: List[int]


    (C.x[1]) = \\
        42 == 5# type: bool
lst[...] = \\
    ((\\
...)) # type: int # comment ..

y = ... # type: int # comment ...
z = ...
#type: int


#DONE placement of annotation after target rather than before =

TD.x[1]  \\
    = 0 == 5# type: bool

TD.y[1] =5 == 5# type: bool # one more here
F[G(x == y,

# hm...

    z)]\\
      = None # type: OMG[int] # comment: None
x = None#type:int   #comment : None"""

big_result = """\
# -*- coding: utf-8 -*- # this should not be spoiled
'''
Docstring here
'''

import testmod
x: int = 5 # this one is OK
ttt: Tuple[float, float, float] \\
    = \\
       (1.0, \\
        2.0, \\
        3.0,)
with foo(x==1) as f: #type: str
    print(f)

for i, j in my_inter(x=1): # type: ignore
    i + j # type: int # what about this

x = y = z = 1 # type: int
x, y, z = [], [], []  # type: (List[int], List[int], List[str])
class C:


    l[f(x
        =1)]: List[int] = [

         1,
         2,
         ]


    (C.x[1]): bool = \\
        42 == 5
lst[...]: int = \\
    ((\\
...)) # comment ..

y: int = ... # comment ...
z = ...
#type: int


#DONE placement of annotation after target rather than before =

TD.x[1]: bool  \\
    = 0 == 5

TD.y[1]: bool =5 == 5 # one more here
F[G(x == y,

# hm...

    z)]: OMG[int]\\
      = None # comment: None
x: int = None   #comment : None"""

big_result_ne = """\
# -*- coding: utf-8 -*- # this should not be spoiled
'''
Docstring here
'''

import testmod
x: int = 5 # this one is OK
ttt: Tuple[float, float, float] \\
    = \\
       (1.0, \\
        2.0, \\
        3.0,)
with foo(x==1) as f: #type: str
    print(f)

for i, j in my_inter(x=1): # type: ignore
    i + j # type: int # what about this

x = y = z = 1 # type: int
x, y, z = [], [], []  # type: (List[int], List[int], List[str])
class C:


    l[f(x
        =1)]: List[int] = [

         1,
         2,
         ]


    (C.x[1]): bool = \\
        42 == 5
lst[...]: int \\
    \\
 # comment ..

y: int # comment ...
z = ...
#type: int


#DONE placement of annotation after target rather than before =

TD.x[1]: bool  \\
    = 0 == 5

TD.y[1]: bool =5 == 5 # one more here
F[G(x == y,

# hm...

    z)]: OMG[int]\\
       # comment: None
x: int   #comment : None"""

if __name__ == '__main__':
    unittest.main()
