import collections
import contextlib
import dataclasses
import enum
import unittest


class TestPatma(unittest.TestCase):

    def test_patma_000(self):
        match 0:
            case 0:
                x = True
        self.assertEqual(x, True)

    def test_patma_001(self):
        match 0:
            case 0 if False:
                x = False
            case 0 if True:
                x = True
        self.assertEqual(x, True)

    def test_patma_002(self):
        match 0:
            case 0:
                x = True
            case 0:
                x = False
        self.assertEqual(x, True)

    def test_patma_003(self):
        x = False
        match 0:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_004(self):
        x = False
        match 1:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_005(self):
        x = False
        match 2:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_006(self):
        x = False
        match 3:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_007(self):
        x = False
        match 4:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, False)

    def test_patma_008(self):
        x = 0
        class A:
            y = 1
        z = None
        match x:
            case z := A.y:
                pass
        self.assertEqual(x, 0)
        self.assertEqual(A.y, 1)
        self.assertEqual(z, None)

    def test_patma_009(self):
        class A:
            B = 0
        match 0:
            case x if x:
                z = 0
            case y := _ if y == x and y:
                z = 1
            case A.B:
                z = 2
        self.assertEqual(A.B, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 2)

    def test_patma_010(self):
        match ():
            case []:
                x = 0
        self.assertEqual(x, 0)

    def test_patma_011(self):
        match (0, 1, 2):
            case [*x]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_012(self):
        match (0, 1, 2):
            case [0, *x]:
                y = 0
        self.assertEqual(x, [1, 2])
        self.assertEqual(y, 0)

    def test_patma_013(self):
        match (0, 1, 2):
            case [0, 1, *x,]:
                y = 0
        self.assertEqual(x, [2])
        self.assertEqual(y, 0)

    def test_patma_014(self):
        match (0, 1, 2):
            case [0, 1, 2, *x]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_015(self):
        match (0, 1, 2):
            case [*x, 2,]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_016(self):
        match (0, 1, 2):
            case [*x, 1, 2]:
                y = 0
        self.assertEqual(x, [0])
        self.assertEqual(y, 0)

    def test_patma_017(self):
        match (0, 1, 2):
            case [*x, 0, 1, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_018(self):
        match (0, 1, 2):
            case [0, *x, 2]:
                y = 0
        self.assertEqual(x, [1])
        self.assertEqual(y, 0)

    def test_patma_019(self):
        match (0, 1, 2):
            case [0, 1, *x, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_020(self):
        match (0, 1, 2):
            case [0, *x, 1, 2]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_021(self):
        match (0, 1, 2):
            case [*x,]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_022(self):
        x = {}
        match x:
            case {}:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, 0)

    def test_patma_023(self):
        x = {0: 0}
        match x:
            case {}:
                y = 0
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 0)

    def test_patma_024(self):
        x = {}
        y = None
        match x:
            case {0: 0}:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_025(self):
        x = {0: 0}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_026(self):
        x = {0: 1}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

    def test_patma_027(self):
        x = {0: 2}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 2})
        self.assertEqual(y, 0)
        self.assertEqual(z, 2)

    def test_patma_028(self):
        x = {0: 3}
        y = None
        z = None
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 3})
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_029(self):
        x = {}
        y = None
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_030(self):
        x = {False: (True, 2.0, {})}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {False: (True, 2.0, {})})
        self.assertEqual(y, 0)

    def test_patma_031(self):
        x = {False: (True, 2.0, {}), 1: [[]], 2: 0}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {False: (True, 2.0, {}), 1: [[]], 2: 0})
        self.assertEqual(y, 0)

    def test_patma_032(self):
        x = {False: (True, 2.0, {}), 1: [[]], 2: 0}
        match x:
            case {0: [1, 2]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {False: (True, 2.0, {}), 1: [[]], 2: 0})
        self.assertEqual(y, 1)

    def test_patma_033(self):
        x = []
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, [])
        self.assertEqual(y, 2)

    def test_patma_034(self):
        x = {0: 0}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: ([1, 2, {}] | False)} | {1: [[]]} | {0: [1, 2, {}]} | [] | "X" | {}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 1)

    def test_patma_035(self):
        x = {0: 0}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}] | True} | {1: [[]]} | {0: [1, 2, {}]} | [] | "X" | {}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 1)

    def test_patma_036(self):
        x = 0
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_037(self):
        x = 1
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_038(self):
        x = 2
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_039(self):
        x = 3
        y = None
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_040(self):
        x = 0
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_041(self):
        x = 1
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

    def test_patma_042(self):
        x = 2
        y = None
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, None)
        self.assertEqual(z, 2)

    def test_patma_043(self):
        x = 3
        y = None
        z = None
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_044(self):
        x = ()
        match x:
            case []:
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)

    def test_patma_045(self):
        x = ()
        match x:
            case []:
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)

    def test_patma_046(self):
        x = (0,)
        match x:
            case [0]:
                y = 0
        self.assertEqual(x, (0,))
        self.assertEqual(y, 0)

    def test_patma_047(self):
        x = ((),)
        match x:
            case [[]]:
                y = 0
        self.assertEqual(x, ((),))
        self.assertEqual(y, 0)

    def test_patma_048(self):
        x = [0, 1]
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_049(self):
        x = [1, 0]
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [1, 0])
        self.assertEqual(y, 0)

    def test_patma_050(self):
        x = [0, 0]
        y = None
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [0, 0])
        self.assertEqual(y, None)

    def test_patma_051(self):
        w = None
        x = [1, 0]
        match x:
            case [(w := 0)]:
                y = 0
            case [z] | [1, (z := 0 | 1)] | [z]:
                y = 1
        self.assertEqual(w, None)
        self.assertEqual(x, [1, 0])
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_052(self):
        x = [1, 0]
        match x:
            case [0]:
                y = 0
            case [1, 0] if (x := x[:0]):
                y = 1
            case [1, 0]:
                y = 2
        self.assertEqual(x, [])
        self.assertEqual(y, 2)

    def test_patma_053(self):
        x = {0}
        y = None
        match x:
            case [0]:
                y = 0
        self.assertEqual(x, {0})
        self.assertEqual(y, None)

    def test_patma_054(self):
        x = set()
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual(x, set())
        self.assertEqual(y, None)

    def test_patma_055(self):
        x = iter([1, 2, 3])
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual([*x], [1, 2, 3])
        self.assertEqual(y, None)

    def test_patma_056(self):
        x = {}
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_057(self):
        x = {0: False, 1: True}
        y = None
        match x:
            case [0, 1]:
                y = 0
        self.assertEqual(x, {0: False, 1: True})
        self.assertEqual(y, None)

    def test_patma_058(self):
        x = 0
        match x:
            case 0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_059(self):
        x = 0
        match x:
            case False:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_060(self):
        x = 0
        y = None
        match x:
            case 1:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_061(self):
        x = 0
        y = None
        match x:
            case None:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_062(self):
        x = 0
        match x:
            case 0:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_063(self):
        x = 0
        y = None
        match x:
            case 1:
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_064(self):
        x = "x"
        match x:
            case "x":
                y = 0
            case "y":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 0)

    def test_patma_065(self):
        x = "x"
        match x:
            case "y":
                y = 0
            case "x":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 1)

    def test_patma_066(self):
        x = "x"
        match x:
            case "x":
                y = 0
            case "y":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 0)

    def test_patma_067(self):
        x = b"x"
        match x:
            case b"y":
                y = 0
            case b"x":
                y = 1
        self.assertEqual(x, b"x")
        self.assertEqual(y, 1)

    def test_patma_068(self):
        x = 0
        match x:
            case 0 if False:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)

    def test_patma_069(self):
        x = 0
        y = None
        match x:
            case 0 if 0:
                y = 0
            case 0 if 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_070(self):
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_071(self):
        x = 0
        match x:
            case 0 if 1:
                y = 0
            case 0 if 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_072(self):
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        y = 2
        self.assertEqual(x, 0)
        self.assertEqual(y, 2)

    def test_patma_073(self):
        x = 0
        match x:
            case 0 if 0:
                y = 0
            case 0 if 1:
                y = 1
        y = 2
        self.assertEqual(x, 0)
        self.assertEqual(y, 2)

    def test_patma_074(self):
        x = 0
        y = None
        match x:
            case 0 if not (x := 1):
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 1)
        self.assertEqual(y, None)

    def test_patma_075(self):
        x = "x"
        match x:
            case ["x"]:
                y = 0
            case "x":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 1)

    def test_patma_076(self):
        x = b"x"
        match x:
            case [b"x"]:
                y = 0
            case ["x"]:
                y = 1
            case [120]:
                y = 2
            case b"x":
                y = 4
        self.assertEqual(x, b"x")
        self.assertEqual(y, 4)

    def test_patma_077(self):
        x = bytearray(b"x")
        y = None
        match x:
            case [120]:
                y = 0
            case 120:
                y = 1
        self.assertEqual(x, b"x")
        self.assertEqual(y, None)

    def test_patma_078(self):
        x = ""
        match x:
            case []:
                y = 0
            case [""]:
                y = 1
            case "":
                y = 2
        self.assertEqual(x, "")
        self.assertEqual(y, 2)

    def test_patma_079(self):
        x = "xxx"
        match x:
            case ["x", "x", "x"]:
                y = 0
            case ["xxx"]:
                y = 1
            case "xxx":
                y = 2
        self.assertEqual(x, "xxx")
        self.assertEqual(y, 2)

    def test_patma_080(self):
        x = b"xxx"
        match x:
            case [120, 120, 120]:
                y = 0
            case [b"xxx"]:
                y = 1
            case b"xxx":
                y = 2
        self.assertEqual(x, b"xxx")
        self.assertEqual(y, 2)

    def test_patma_081(self):
        x = 0
        match x:
            case 0 if not (x := 1):
                y = 0
            case (z := 0):
                y = 1
        self.assertEqual(x, 1)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_082(self):
        x = 0
        z = None
        match x:
            case (z := 1) if not (x := 1):
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, None)

    def test_patma_083(self):
        x = 0
        match x:
            case (z := 0):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_084(self):
        x = 0
        y = None
        z = None
        match x:
            case (z := 1):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_085(self):
        x = 0
        y = None
        match x:
            case (z := 0) if (w := 0):
                y = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, None)
        self.assertEqual(z, 0)

    def test_patma_086(self):
        x = 0
        match x:
            case (z := (w := 0)):
                y = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_087(self):
        x = 0
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_088(self):
        x = 1
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_089(self):
        x = 2
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_090(self):
        x = 3
        y = None
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_091(self):
        x = 0
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_092(self):
        x = 1
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_093(self):
        x = 2
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_094(self):
        x = 3
        y = None
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_095(self):
        x = 0
        match x:
            case -0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_096(self):
        x = 0
        match x:
            case -0.0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_097(self):
        x = 0
        match x:
            case -0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_098(self):
        x = 0
        match x:
            case -0.0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_099(self):
        x = -1
        match x:
            case -1:
                y = 0
        self.assertEqual(x, -1)
        self.assertEqual(y, 0)

    def test_patma_100(self):
        x = -1.5
        match x:
            case -1.5:
                y = 0
        self.assertEqual(x, -1.5)
        self.assertEqual(y, 0)

    def test_patma_101(self):
        x = -1j
        match x:
            case -1j:
                y = 0
        self.assertEqual(x, -1j)
        self.assertEqual(y, 0)

    def test_patma_102(self):
        x = -1.5j
        match x:
            case -1.5j:
                y = 0
        self.assertEqual(x, -1.5j)
        self.assertEqual(y, 0)

    def test_patma_103(self):
        x = 0
        match x:
            case 0 + 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_104(self):
        x = 0
        match x:
            case 0 - 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_105(self):
        x = 0
        match x:
            case -0 + 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_106(self):
        x = 0
        match x:
            case -0 - 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_107(self):
        x = 0.25 + 1.75j
        match x:
            case 0.25 + 1.75j:
                y = 0
        self.assertEqual(x, 0.25 + 1.75j)
        self.assertEqual(y, 0)

    def test_patma_108(self):
        x = 0.25 - 1.75j
        match x:
            case 0.25 - 1.75j:
                y = 0
        self.assertEqual(x, 0.25 - 1.75j)
        self.assertEqual(y, 0)

    def test_patma_109(self):
        x = -0.25 + 1.75j
        match x:
            case -0.25 + 1.75j:
                y = 0
        self.assertEqual(x, -0.25 + 1.75j)
        self.assertEqual(y, 0)

    def test_patma_110(self):
        x = -0.25 - 1.75j
        match x:
            case -0.25 - 1.75j:
                y = 0
        self.assertEqual(x, -0.25 - 1.75j)
        self.assertEqual(y, 0)

    def test_patma_111(self):
        class A:
            B = 0
        x = 0
        match x:
            case A.B:
                y = 0
        self.assertEqual(A.B, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_112(self):
        class A:
            class B:
                C = 0
        x = 0
        match x:
            case A.B.C:
                y = 0
        self.assertEqual(A.B.C, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_113(self):
        class A:
            class B:
                C = 0
        x = 0
        match x:
            case A.B.C:
                y = 0
        self.assertEqual(A.B.C, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_114(self):
        class A:
            class B:
                class C:
                    D = 0
        x = 0
        match x:
            case A.B.C.D:
                y = 0
        self.assertEqual(A.B.C.D, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_115(self):
        class A:
            class B:
                class C:
                    D = 0
        x = 0
        match x:
            case A.B.C.D:
                y = 0
        self.assertEqual(A.B.C.D, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_116(self):
        match = case = 0
        match match:
            case case:
                x = 0
        self.assertEqual(match, 0)
        self.assertEqual(case, 0)
        self.assertEqual(x, 0)

    def test_patma_117(self):
        match = case = 0
        match case:
            case match:
                x = 0
        self.assertEqual(match, 0)
        self.assertEqual(case, 0)
        self.assertEqual(x, 0)

    def test_patma_118(self):
        x = []
        match x:
            case [*_, _]:
                y = 0
            case []:
                y = 1
        self.assertEqual(x, [])
        self.assertEqual(y, 1)

    def test_patma_119(self):
        x = collections.defaultdict(int)
        match x:
            case {0: 0}:
                y = 0
            case {}:
                y = 1
        self.assertEqual(x, {})
        self.assertEqual(y, 1)

    def test_patma_120(self):
        x = collections.defaultdict(int)
        match x:
            case {0: 0}:
                y = 0
            case {**z}:
                y = 1
        self.assertEqual(x, {})
        self.assertEqual(y, 1)
        self.assertEqual(z, {})

    def test_patma_121(self):
        match ():
            case ():
                x = 0
        self.assertEqual(x, 0)

    def test_patma_122(self):
        match (0, 1, 2):
            case (*x,):
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_123(self):
        match (0, 1, 2):
            case 0, *x:
                y = 0
        self.assertEqual(x, [1, 2])
        self.assertEqual(y, 0)

    def test_patma_124(self):
        match (0, 1, 2):
            case (0, 1, *x,):
                y = 0
        self.assertEqual(x, [2])
        self.assertEqual(y, 0)

    def test_patma_125(self):
        match (0, 1, 2):
            case 0, 1, 2, *x:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_126(self):
        match (0, 1, 2):
            case *x, 2,:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_127(self):
        match (0, 1, 2):
            case (*x, 1, 2):
                y = 0
        self.assertEqual(x, [0])
        self.assertEqual(y, 0)

    def test_patma_128(self):
        match (0, 1, 2):
            case *x, 0, 1, 2,:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_129(self):
        match (0, 1, 2):
            case (0, *x, 2):
                y = 0
        self.assertEqual(x, [1])
        self.assertEqual(y, 0)

    def test_patma_130(self):
        match (0, 1, 2):
            case 0, 1, *x, 2,:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_131(self):
        match (0, 1, 2):
            case (0, *x, 1, 2):
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_132(self):
        match (0, 1, 2):
            case *x,:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_133(self):
        x = collections.defaultdict(int, {0: 1})
        match x:
            case {1: 0}:
                y = 0
            case {0: 0}:
                y = 1
            case {}:
                y = 2
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 2)

    def test_patma_134(self):
        x = collections.defaultdict(int, {0: 1})
        match x:
            case {1: 0}:
                y = 0
            case {0: 0}:
                y = 1
            case {**z}:
                y = 2
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 2)
        self.assertEqual(z, {0: 1})

    def test_patma_135(self):
        x = collections.defaultdict(int, {0: 1})
        match x:
            case {1: 0}:
                y = 0
            case {0: 0}:
                y = 1
            case {0: _, **z}:
                y = 2
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 2)
        self.assertEqual(z, {})

    def test_patma_136(self):
        x = {0: 1}
        match x:
            case {1: 0}:
                y = 0
            case {0: 0}:
                y = 0
            case {}:
                y = 1
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 1)

    def test_patma_137(self):
        x = {0: 1}
        match x:
            case {1: 0}:
                y = 0
            case {0: 0}:
                y = 0
            case {**z}:
                y = 1
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 1)
        self.assertEqual(z, {0: 1})

    def test_patma_138(self):
        x = {0: 1}
        match x:
            case {1: 0}:
                y = 0
            case {0: 0}:
                y = 0
            case {0: _, **z}:
                y = 1
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 1)
        self.assertEqual(z, {})

    def test_patma_139(self):
        x = False
        match x:
            case bool(z):
                y = 0
        self.assertIs(x, False)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_140(self):
        x = True
        match x:
            case bool(z):
                y = 0
        self.assertIs(x, True)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_141(self):
        x = bytearray()
        match x:
            case bytearray(z):
                y = 0
        self.assertEqual(x, bytearray())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_142(self):
        x = b""
        match x:
            case bytes(z):
                y = 0
        self.assertEqual(x, b"")
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_143(self):
        x = {}
        match x:
            case dict(z):
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_144(self):
        x = 0.0
        match x:
            case float(z):
                y = 0
        self.assertEqual(x, 0.0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_145(self):
        x = frozenset()
        match x:
            case frozenset(z):
                y = 0
        self.assertEqual(x, frozenset())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_146(self):
        x = 0
        match x:
            case int(z):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_147(self):
        x = []
        match x:
            case list(z):
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_148(self):
        x = set()
        match x:
            case set(z):
                y = 0
        self.assertEqual(x, set())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_149(self):
        x = ""
        match x:
            case str(z):
                y = 0
        self.assertEqual(x, "")
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_150(self):
        x = ()
        match x:
            case tuple(z):
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_151(self):
        x = 0
        match x,:
            case y,:
                z = 0
        self.assertEqual(x, 0)
        self.assertIs(y, x)
        self.assertIs(z, 0)

    def test_patma_152(self):
        w = 0
        x = 0
        match w, x:
            case y, z:
                v = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertIs(y, w)
        self.assertIs(z, x)
        self.assertEqual(v, 0)

    def test_patma_153(self):
        x = 0
        match w := x,:
            case v := y,:
                z = 0
        self.assertEqual(x, 0)
        self.assertIs(y, x)
        self.assertEqual(z, 0)
        self.assertIs(w, x)
        self.assertIs(v, y)

    def test_patma_154(self):
        x = 0
        y = None
        match x:
            case 0 if x:
                y = 0
        self.assertEqual(x, 0)
        self.assertIs(y, None)

    def test_patma_155(self):
        x = 0
        y = None
        match x:
            case 1:
                y = 0
        self.assertEqual(x, 0)
        self.assertIs(y, None)

    def test_patma_156(self):
        x = 0
        match x:
            case z:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_157(self):
        x = 0
        y = None
        match x:
            case _ if x:
                y = 0
        self.assertEqual(x, 0)
        self.assertIs(y, None)

    def test_patma_158(self):
        x = 0
        match x:
            case 0:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_159(self):
        x = 0
        match x:
            case 0 if not x:
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_160(self):
        x = 0
        z = None
        match x:
            case 0:
                y = 0
            case z if x:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, None)

    def test_patma_161(self):
        x = 0
        match x:
            case 0:
                y = 0
            case _:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_162(self):
        x = 0
        match x:
            case 1 if x:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)

    def test_patma_163(self):
        x = 0
        y = None
        match x:
            case 1:
                y = 0
            case 1 if not x:
                y = 1
        self.assertEqual(x, 0)
        self.assertIs(y, None)

    def test_patma_164(self):
        x = 0
        match x:
            case 1:
                y = 0
            case z:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertIs(z, x)

    def test_patma_165(self):
        x = 0
        match x:
            case 1 if x:
                y = 0
            case _:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)

    def test_patma_166(self):
        x = 0
        match x:
            case z if not z:
                y = 0
            case 0 if x:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_167(self):
        x = 0
        match x:
            case z if not z:
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_168(self):
        x = 0
        match x:
            case z if not x:
                y = 0
            case z:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_169(self):
        x = 0
        match x:
            case z if not z:
                y = 0
            case _ if x:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_170(self):
        x = 0
        match x:
            case _ if not x:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_171(self):
        x = 0
        y = None
        match x:
            case _ if x:
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertIs(y, None)

    def test_patma_172(self):
        x = 0
        z = None
        match x:
            case _ if not x:
                y = 0
            case z if not x:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, None)

    def test_patma_173(self):
        x = 0
        match x:
            case _ if not x:
                y = 0
            case _:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_174(self):
        def http_error(status):
            match status:
                case 400:
                    return "Bad request"
                case 401:
                    return "Unauthorized"
                case 403:
                    return "Forbidden"
                case 404:
                    return "Not found"
                case 418:
                    return "I'm a teapot"
                case _:
                    return "Something else"
        self.assertEqual(http_error(400), "Bad request")
        self.assertEqual(http_error(401), "Unauthorized")
        self.assertEqual(http_error(403), "Forbidden")
        self.assertEqual(http_error(404), "Not found")
        self.assertEqual(http_error(418), "I'm a teapot")
        self.assertEqual(http_error(123), "Something else")
        self.assertEqual(http_error("400"), "Something else")
        self.assertEqual(http_error(401 | 403 | 404), "Something else")  # 407

    def test_patma_175(self):
        def http_error(status):
            match status:
                case 400:
                    return "Bad request"
                case 401 | 403 | 404:
                    return "Not allowed"
                case 418:
                    return "I'm a teapot"
        self.assertEqual(http_error(400), "Bad request")
        self.assertEqual(http_error(401), "Not allowed")
        self.assertEqual(http_error(403), "Not allowed")
        self.assertEqual(http_error(404), "Not allowed")
        self.assertEqual(http_error(418), "I'm a teapot")
        self.assertIs(http_error(123), None)
        self.assertIs(http_error("400"), None)
        self.assertIs(http_error(401 | 403 | 404), None)  # 407

    def test_patma_176(self):
        def whereis(point):
            match point:
                case (0, 0):
                    return "Origin"
                case (0, y):
                    return f"Y={y}"
                case (x, 0):
                    return f"X={x}"
                case (x, y):
                    return f"X={x}, Y={y}"
                case _:
                    raise ValueError("Not a point")
        self.assertEqual(whereis((0, 0)), "Origin")
        self.assertEqual(whereis((0, -1.0)), "Y=-1.0")
        self.assertEqual(whereis(("X", 0)), "X=X")
        self.assertEqual(whereis((None, 1j)), "X=None, Y=1j")
        with self.assertRaises(ValueError):
            whereis(42)

    def test_patma_177(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(point):
            match point:
                case Point(0, 0):
                    return "Origin"
                case Point(0, y):
                    return f"Y={y}"
                case Point(x, 0):
                    return f"X={x}"
                case Point():
                    return "Somewhere else"
                case _:
                    return "Not a point"
        self.assertEqual(whereis(Point(1, 0)), "X=1")
        self.assertEqual(whereis(Point(0, 0)), "Origin")
        self.assertEqual(whereis(10), "Not a point")
        self.assertEqual(whereis(Point(False, False)), "Origin")
        self.assertEqual(whereis(Point(0, -1.0)), "Y=-1.0")
        self.assertEqual(whereis(Point("X", 0)), "X=X")
        self.assertEqual(whereis(Point(None, 1j)), "Somewhere else")
        self.assertEqual(whereis(Point), "Not a point")
        self.assertEqual(whereis(42), "Not a point")

    def test_patma_178(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(point):
            match point:
                case Point(1, var):
                    return var
        self.assertEqual(whereis(Point(1, 0)), 0)
        self.assertIs(whereis(Point(0, 0)), None)

    def test_patma_179(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(point):
            match point:
                case Point(1, y=var):
                    return var
        self.assertEqual(whereis(Point(1, 0)), 0)
        self.assertIs(whereis(Point(0, 0)), None)

    def test_patma_180(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(point):
            match point:
                case Point(x=1, y=var):
                    return var
        self.assertEqual(whereis(Point(1, 0)), 0)
        self.assertIs(whereis(Point(0, 0)), None)

    def test_patma_181(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(point):
            match point:
                case Point(y=var, x=1):
                    return var
        self.assertEqual(whereis(Point(1, 0)), 0)
        self.assertIs(whereis(Point(0, 0)), None)

    def test_patma_182(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(points):
            match points:
                case []:
                    return "No points"
                case [Point(0, 0)]:
                    return "The origin"
                case [Point(x, y)]:
                    return f"Single point {x}, {y}"
                case [Point(0, y1), Point(0, y2)]:
                    return f"Two on the Y axis at {y1}, {y2}"
                case _:
                    return "Something else"
        self.assertEqual(whereis([]), "No points")
        self.assertEqual(whereis([Point(0, 0)]), "The origin")
        self.assertEqual(whereis([Point(0, 1)]), "Single point 0, 1")
        self.assertEqual(whereis([Point(0, 0), Point(0, 0)]), "Two on the Y axis at 0, 0")
        self.assertEqual(whereis([Point(0, 1), Point(0, 1)]), "Two on the Y axis at 1, 1")
        self.assertEqual(whereis([Point(0, 0), Point(1, 0)]), "Something else")
        self.assertEqual(whereis([Point(0, 0), Point(0, 0), Point(0, 0)]), "Something else")
        self.assertEqual(whereis([Point(0, 1), Point(0, 1), Point(0, 1)]), "Something else")

    def test_patma_183(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        def whereis(point):
            match point:
                case Point(x, y) if x == y:
                    return f"Y=X at {x}"
                case Point(x, y):
                    return "Not on the diagonal"
        self.assertEqual(whereis(Point(0, 0)), "Y=X at 0")
        self.assertEqual(whereis(Point(0, False)), "Y=X at 0")
        self.assertEqual(whereis(Point(False, 0)), "Y=X at False")
        self.assertEqual(whereis(Point(-1 - 1j, -1 - 1j)), "Y=X at (-1-1j)")
        self.assertEqual(whereis(Point("X", "X")), "Y=X at X")
        self.assertEqual(whereis(Point("X", "x")), "Not on the diagonal")

    def test_patma_184(self):
        class Seq(collections.abc.Sequence):
            __getitem__ = None
            def __len__(self):
                return 0
        match Seq():
            case []:
                y = 0
        self.assertEqual(y, 0)

    def test_patma_185(self):
        class Seq(collections.abc.Sequence):
            __getitem__ = None
            def __len__(self):
                return 42
        match Seq():
            case [*_]:
                y = 0
        self.assertEqual(y, 0)

    def test_patma_186(self):
        class Seq(collections.abc.Sequence):
            def __getitem__(self, i):
                return i
            def __len__(self):
                return 42
        match Seq():
            case [x, *y, z]:
                w = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, list(range(1, 41)))
        self.assertEqual(z, 41)

    def test_patma_187(self):
        w = range(10)
        match w:
            case [x, y, *rest]:
                z = 0
        self.assertEqual(w, range(10))
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)
        self.assertEqual(rest, list(range(2, 10)))

    def test_patma_188(self):
        w = range(100)
        match w:
            case (x, y, *rest):
                z = 0
        self.assertEqual(w, range(100))
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)
        self.assertEqual(rest, list(range(2, 100)))

    def test_patma_189(self):
        w = range(1000)
        match w:
            case x, y, *rest:
                z = 0
        self.assertEqual(w, range(1000))
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)
        self.assertEqual(rest, list(range(2, 1000)))

    def test_patma_190(self):
        w = range(1 << 10)
        match w:
            case [x, y, *_]:
                z = 0
        self.assertEqual(w, range(1 << 10))
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_191(self):
        w = range(1 << 20)
        match w:
            case (x, y, *_):
                z = 0
        self.assertEqual(w, range(1 << 20))
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_192(self):
        w = range(1 << 30)
        match w:
            case x, y, *_:
                z = 0
        self.assertEqual(w, range(1 << 30))
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_193(self):
        x = {"bandwidth": 0, "latency": 1}
        match x:
            case {"bandwidth": b, "latency": l}:
                y = 0
        self.assertEqual(x, {"bandwidth": 0, "latency": 1})
        self.assertIs(b, x["bandwidth"])
        self.assertIs(l, x["latency"])
        self.assertEqual(y, 0)

    def test_patma_194(self):
        x = {"bandwidth": 0, "latency": 1, "key": "value"}
        match x:
            case {"latency": l, "bandwidth": b}:
                y = 0
        self.assertEqual(x, {"bandwidth": 0, "latency": 1, "key": "value"})
        self.assertIs(l, x["latency"])
        self.assertIs(b, x["bandwidth"])
        self.assertEqual(y, 0)

    def test_patma_195(self):
        x = {"bandwidth": 0, "latency": 1, "key": "value"}
        match x:
            case {"bandwidth": b, "latency": l, **rest}:
                y = 0
        self.assertEqual(x, {"bandwidth": 0, "latency": 1, "key": "value"})
        self.assertIs(b, x["bandwidth"])
        self.assertIs(l, x["latency"])
        self.assertEqual(rest, {"key": "value"})
        self.assertEqual(y, 0)

    def test_patma_196(self):
        x = {"bandwidth": 0, "latency": 1}
        match x:
            case {"latency": l, "bandwidth": b, **rest}:
                y = 0
        self.assertEqual(x, {"bandwidth": 0, "latency": 1})
        self.assertIs(l, x["latency"])
        self.assertIs(b, x["bandwidth"])
        self.assertEqual(rest, {})
        self.assertEqual(y, 0)

    def test_patma_197(self):
        @dataclasses.dataclass
        class Point:
            x: int
            y: int
        w = [Point(-1, 0), Point(1, 2)]
        match w:
            case (Point(x1, y1), p2 := Point(x2, y2)):
                z = 0
        self.assertEqual(w, [Point(-1, 0), Point(1, 2)])
        self.assertIs(x1, w[0].x)
        self.assertIs(y1, w[0].y)
        self.assertIs(p2, w[1])
        self.assertIs(x2, w[1].x)
        self.assertIs(y2, w[1].y)
        self.assertIs(z, 0)

    def test_patma_198(self):
        class Color(enum.Enum):
            RED = 0
            GREEN = 1
            BLUE = 2
        def f(color):
            match color:
                case Color.RED:
                    return "I see red!"
                case Color.GREEN:
                    return "Grass is green"
                case Color.BLUE:
                    return "I'm feeling the blues :("
        self.assertEqual(f(Color.RED), "I see red!")
        self.assertEqual(f(Color.GREEN), "Grass is green")
        self.assertEqual(f(Color.BLUE), "I'm feeling the blues :(")
        self.assertEqual(f(Color), None)
        self.assertEqual(f(0), None)
        self.assertEqual(f(1), None)
        self.assertEqual(f(2), None)
        self.assertEqual(f(3), None)
        self.assertEqual(f(False), None)
        self.assertEqual(f(True), None)
        self.assertEqual(f(2+0j), None)
        self.assertEqual(f(3.0), None)

    def test_patma_199(self):
        class Color(int, enum.Enum):
            RED = 0
            GREEN = 1
            BLUE = 2
        def f(color):
            match color:
                case Color.RED:
                    return "I see red!"
                case Color.GREEN:
                    return "Grass is green"
                case Color.BLUE:
                    return "I'm feeling the blues :("
        self.assertEqual(f(Color.RED), "I see red!")
        self.assertEqual(f(Color.GREEN), "Grass is green")
        self.assertEqual(f(Color.BLUE), "I'm feeling the blues :(")
        self.assertEqual(f(Color), None)
        self.assertEqual(f(0), "I see red!")
        self.assertEqual(f(1), "Grass is green")
        self.assertEqual(f(2), "I'm feeling the blues :(")
        self.assertEqual(f(3), None)
        self.assertEqual(f(False), "I see red!")
        self.assertEqual(f(True), "Grass is green")
        self.assertEqual(f(2+0j), "I'm feeling the blues :(")
        self.assertEqual(f(3.0), None)


class PerfPatma(TestPatma):

    def assertEqual(*_, **__):
        pass

    def assertIs(*_, **__):
        pass

    @contextlib.contextmanager
    def assertRaises(*_, **__):
        yield

    def run_perf(self):
        attrs = vars(type(self)).items()
        tests = [attr for name, attr in attrs if name.startswith("test_")]
        for _ in range(1 << 10):
            for test in tests:
                test(self)

    @staticmethod
    def setUpClass():
        raise unittest.SkipTest("performance testing")


"""
sudo ./python -m pyperf system tune && \
     ./python -m pyperf timeit -s "from test.test_patma import PerfPatma; p = PerfPatma()" "p.run_perf()"; \
sudo ./python -m pyperf system reset
"""
