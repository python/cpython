import collections
import unittest


class TestMatch(unittest.TestCase):

    def test_patma_000(self) -> None:
        match 0:
            case 0:
                x = True
        self.assertEqual(x, True)

    def test_patma_001(self) -> None:
        match 0:
            case 0 if False:
                x = False
            case 0 if True:
                x = True
        self.assertEqual(x, True)

    def test_patma_002(self) -> None:
        match 0:
            case 0:
                x = True
            case 0:
                x = False
        self.assertEqual(x, True)

    def test_patma_003(self) -> None:
        x = False
        match 0:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_004(self) -> None:
        x = False
        match 1:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_005(self) -> None:
        x = False
        match 2:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_006(self) -> None:
        x = False
        match 3:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_007(self) -> None:
        x = False
        match 4:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, False)

    def test_patma_008(self) -> None:
        x = 0
        y = 1
        z = None
        match x:
            case z := .y:
                pass
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, None)

    def test_patma_009(self) -> None:
        class A:
            B = 0
        match 0:
            case x if x:
                z = 0
            case y := .x if y:
                z = 1
            case A.B:
                z = 2
        self.assertEqual(A.B, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 2)

    def test_patma_010(self) -> None:
        match ():
            case []:
                x = 0
        self.assertEqual(x, 0)

    def test_patma_011(self) -> None:
        match (0, 1, 2):
            case [*x]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_012(self) -> None:
        match (0, 1, 2):
            case [0, *x]:
                y = 0
        self.assertEqual(x, [1, 2])
        self.assertEqual(y, 0)

    def test_patma_013(self) -> None:
        match (0, 1, 2):
            case [0, 1, *x,]:
                y = 0
        self.assertEqual(x, [2])
        self.assertEqual(y, 0)

    def test_patma_014(self) -> None:
        match (0, 1, 2):
            case [0, 1, 2, *x]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_015(self) -> None:
        match (0, 1, 2):
            case [*x, 2,]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_016(self) -> None:
        match (0, 1, 2):
            case [*x, 1, 2]:
                y = 0
        self.assertEqual(x, [0])
        self.assertEqual(y, 0)

    def test_patma_017(self) -> None:
        match (0, 1, 2):
            case [*x, 0, 1, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_018(self) -> None:
        match (0, 1, 2):
            case [0, *x, 2]:
                y = 0
        self.assertEqual(x, [1])
        self.assertEqual(y, 0)

    def test_patma_019(self) -> None:
        match (0, 1, 2):
            case [0, 1, *x, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_020(self) -> None:
        match (0, 1, 2):
            case [0, *x, 1, 2]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_021(self) -> None:
        match (0, 1, 2):
            case [*x,]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_022(self) -> None:
        x = {}
        match x:
            case {}:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, 0)

    def test_patma_023(self) -> None:
        x = {0: 0}
        match x:
            case {}:
                y = 0
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 0)

    def test_patma_024(self) -> None:
        x = {}
        y = None
        match x:
            case {0: 0}:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_025(self) -> None:
        x = {0: 0}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_026(self) -> None:
        x = {0: 1}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

    def test_patma_027(self) -> None:
        x = {0: 2}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 2})
        self.assertEqual(y, 0)
        self.assertEqual(z, 2)

    def test_patma_028(self) -> None:
        x = {0: 3}
        y = None
        z = None
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 3})
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_029(self) -> None:
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

    def test_patma_030(self) -> None:
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

    def test_patma_031(self) -> None:
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

    def test_patma_032(self) -> None:
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

    def test_patma_033(self) -> None:
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

    def test_patma_034(self) -> None:
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

    def test_patma_035(self) -> None:
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

    def test_patma_036(self) -> None:
        x = 0
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_037(self) -> None:
        x = 1
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_038(self) -> None:
        x = 2
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_039(self) -> None:
        x = 3
        y = None
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_040(self) -> None:
        x = 0
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_041(self) -> None:
        x = 1
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

    def test_patma_042(self) -> None:
        x = 2
        y = None
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, None)
        self.assertEqual(z, 2)

    def test_patma_043(self) -> None:
        x = 3
        y = None
        z = None
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_044(self) -> None:
        x = ()
        match x:
            case []:
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)

    def test_patma_045(self) -> None:
        x = ()
        match x:
            case []:
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)

    def test_patma_046(self) -> None:
        x = (0,)
        match x:
            case [0]:
                y = 0
        self.assertEqual(x, (0,))
        self.assertEqual(y, 0)

    def test_patma_047(self) -> None:
        x = ((),)
        match x:
            case [[]]:
                y = 0
        self.assertEqual(x, ((),))
        self.assertEqual(y, 0)

    def test_patma_048(self) -> None:
        x = [0, 1]
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_049(self) -> None:
        x = [1, 0]
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [1, 0])
        self.assertEqual(y, 0)

    def test_patma_050(self) -> None:
        x = [0, 0]
        y = None
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [0, 0])
        self.assertEqual(y, None)

    def test_patma_051(self) -> None:
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

    def test_patma_052(self) -> None:
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

    def test_patma_053(self) -> None:
        x = {0}
        y = None
        match x:
            case [0]:
                y = 0
        self.assertEqual(x, {0})
        self.assertEqual(y, None)

    def test_patma_054(self) -> None:
        x = set()
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual(x, set())
        self.assertEqual(y, None)

    def test_patma_055(self) -> None:
        x = iter([1, 2, 3])
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual([*x], [1, 2, 3])
        self.assertEqual(y, None)

    def test_patma_056(self) -> None:
        x = {}
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_057(self) -> None:
        x = {0: False, 1: True}
        y = None
        match x:
            case [0, 1]:
                y = 0
        self.assertEqual(x, {0: False, 1: True})
        self.assertEqual(y, None)

    def test_patma_058(self) -> None:
        x = 0
        match x:
            case 0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_059(self) -> None:
        x = 0
        match x:
            case False:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_060(self) -> None:
        x = 0
        y = None
        match x:
            case 1:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_061(self) -> None:
        x = 0
        y = None
        match x:
            case None:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_062(self) -> None:
        x = 0
        match x:
            case 0:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_063(self) -> None:
        x = 0
        y = None
        match x:
            case 1:
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_064(self) -> None:
        x = "x"
        match x:
            case "x":
                y = 0
            case "y":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 0)

    def test_patma_065(self) -> None:
        x = "x"
        match x:
            case "y":
                y = 0
            case "x":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 1)

    def test_patma_066(self) -> None:
        x = "x"
        match x:
            case "x":
                y = 0
            case "y":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 0)

    def test_patma_067(self) -> None:
        x = b"x"
        match x:
            case b"y":
                y = 0
            case b"x":
                y = 1
        self.assertEqual(x, b"x")
        self.assertEqual(y, 1)

    def test_patma_068(self) -> None:
        x = 0
        match x:
            case 0 if False:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)

    def test_patma_069(self) -> None:
        x = 0
        y = None
        match x:
            case 0 if 0:
                y = 0
            case 0 if 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_070(self) -> None:
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_071(self) -> None:
        x = 0
        match x:
            case 0 if 1:
                y = 0
            case 0 if 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_072(self) -> None:
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        y = 2
        self.assertEqual(x, 0)
        self.assertEqual(y, 2)

    def test_patma_073(self) -> None:
        x = 0
        match x:
            case 0 if 0:
                y = 0
            case 0 if 1:
                y = 1
        y = 2
        self.assertEqual(x, 0)
        self.assertEqual(y, 2)

    def test_patma_074(self) -> None:
        x = 0
        y = None
        match x:
            case 0 if not (x := 1):
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 1)
        self.assertEqual(y, None)

    def test_patma_075(self) -> None:
        x = "x"
        match x:
            case ["x"]:
                y = 0
            case "x":
                y = 1
        self.assertEqual(x, "x")
        self.assertEqual(y, 1)

    def test_patma_076(self) -> None:
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

    def test_patma_077(self) -> None:
        x = bytearray(b"x")
        y = None
        match x:
            case [120]:
                y = 0
            case 120:
                y = 1
        self.assertEqual(x, b"x")
        self.assertEqual(y, None)

    def test_patma_078(self) -> None:
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

    def test_patma_079(self) -> None:
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

    def test_patma_080(self) -> None:
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

    def test_patma_081(self) -> None:
        x = 0
        match x:
            case 0 if not (x := 1):
                y = 0
            case (z := 0):
                y = 1
        self.assertEqual(x, 1)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_082(self) -> None:
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

    def test_patma_083(self) -> None:
        x = 0
        match x:
            case (z := 0):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_084(self) -> None:
        x = 0
        y = None
        z = None
        match x:
            case (z := 1):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_085(self) -> None:
        x = 0
        y = None
        match x:
            case (z := 0) if (w := 0):
                y = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, None)
        self.assertEqual(z, 0)

    def test_patma_086(self) -> None:
        x = 0
        match x:
            case (z := (w := 0)):
                y = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_087(self) -> None:
        x = 0
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_088(self) -> None:
        x = 1
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_089(self) -> None:
        x = 2
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_090(self) -> None:
        x = 3
        y = None
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_091(self) -> None:
        x = 0
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_092(self) -> None:
        x = 1
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_093(self) -> None:
        x = 2
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_094(self) -> None:
        x = 3
        y = None
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_095(self) -> None:
        x = 0
        match x:
            case -0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_096(self) -> None:
        x = 0
        match x:
            case -0.0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_097(self) -> None:
        x = 0
        match x:
            case -0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_098(self) -> None:
        x = 0
        match x:
            case -0.0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_099(self) -> None:
        x = -1
        match x:
            case -1:
                y = 0
        self.assertEqual(x, -1)
        self.assertEqual(y, 0)

    def test_patma_100(self) -> None:
        x = -1.5
        match x:
            case -1.5:
                y = 0
        self.assertEqual(x, -1.5)
        self.assertEqual(y, 0)

    def test_patma_101(self) -> None:
        x = -1j
        match x:
            case -1j:
                y = 0
        self.assertEqual(x, -1j)
        self.assertEqual(y, 0)

    def test_patma_102(self) -> None:
        x = -1.5j
        match x:
            case -1.5j:
                y = 0
        self.assertEqual(x, -1.5j)
        self.assertEqual(y, 0)

    def test_patma_103(self) -> None:
        x = 0
        match x:
            case 0 + 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_104(self) -> None:
        x = 0
        match x:
            case 0 - 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_105(self) -> None:
        x = 0
        match x:
            case -0 + 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_106(self) -> None:
        x = 0
        match x:
            case -0 - 0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_107(self) -> None:
        x = 0.25 + 1.75j
        match x:
            case 0.25 + 1.75j:
                y = 0
        self.assertEqual(x, 0.25 + 1.75j)
        self.assertEqual(y, 0)

    def test_patma_108(self) -> None:
        x = 0.25 - 1.75j
        match x:
            case 0.25 - 1.75j:
                y = 0
        self.assertEqual(x, 0.25 - 1.75j)
        self.assertEqual(y, 0)

    def test_patma_109(self) -> None:
        x = -0.25 + 1.75j
        match x:
            case -0.25 + 1.75j:
                y = 0
        self.assertEqual(x, -0.25 + 1.75j)
        self.assertEqual(y, 0)

    def test_patma_110(self) -> None:
        x = -0.25 - 1.75j
        match x:
            case -0.25 - 1.75j:
                y = 0
        self.assertEqual(x, -0.25 - 1.75j)
        self.assertEqual(y, 0)

    def test_patma_111(self) -> None:
        class A:
            B = 0
        x = 0
        match x:
            case .A.B:
                y = 0
        self.assertEqual(A.B, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_112(self) -> None:
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

    def test_patma_113(self) -> None:
        class A:
            class B:
                C = 0
        x = 0
        match x:
            case .A.B.C:
                y = 0
        self.assertEqual(A.B.C, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_114(self) -> None:
        class A:
            class B:
                class C:
                    D = 0
        x = 0
        match x:
            case .A.B.C.D:
                y = 0
        self.assertEqual(A.B.C.D, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_115(self) -> None:
        class A:
            class B:
                class C:
                    D = 0
        x = 0
        match x:
            case .A.B.C.D:
                y = 0
        self.assertEqual(A.B.C.D, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_116(self) -> None:
        match = case = 0
        match match:
            case case:
                x = 0
        self.assertEqual(match, 0)
        self.assertEqual(case, 0)
        self.assertEqual(x, 0)

    def test_patma_117(self) -> None:
        match = case = 0
        match case:
            case match:
                x = 0
        self.assertEqual(match, 0)
        self.assertEqual(case, 0)
        self.assertEqual(x, 0)

    def test_patma_118(self) -> None:
        x = []
        match x:
            case [*_, _]:
                y = 0
            case []:
                y = 1
        self.assertEqual(x, [])
        self.assertEqual(y, 1)

    def test_patma_119(self) -> None:
        x = collections.defaultdict(int)
        match x:
            case {0: 0}:
                y = 0
            case {}:
                y = 1
        self.assertEqual(x, {})
        self.assertEqual(y, 1)

    def test_patma_120(self) -> None:
        x = collections.defaultdict(int)
        match x:
            case {0: 0}:
                y = 0
            case {**z}:
                y = 1
        self.assertEqual(x, {})
        self.assertEqual(y, 1)
        self.assertEqual(z, {})

    def test_patma_121(self) -> None:
        match ():
            case ():
                x = 0
        self.assertEqual(x, 0)

    def test_patma_122(self) -> None:
        match (0, 1, 2):
            case (*x,):
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_123(self) -> None:
        match (0, 1, 2):
            case 0, *x:
                y = 0
        self.assertEqual(x, [1, 2])
        self.assertEqual(y, 0)

    def test_patma_124(self) -> None:
        match (0, 1, 2):
            case (0, 1, *x,):
                y = 0
        self.assertEqual(x, [2])
        self.assertEqual(y, 0)

    def test_patma_125(self) -> None:
        match (0, 1, 2):
            case 0, 1, 2, *x:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_126(self) -> None:
        match (0, 1, 2):
            case *x, 2,:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_127(self) -> None:
        match (0, 1, 2):
            case (*x, 1, 2):
                y = 0
        self.assertEqual(x, [0])
        self.assertEqual(y, 0)

    def test_patma_128(self) -> None:
        match (0, 1, 2):
            case *x, 0, 1, 2,:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_129(self) -> None:
        match (0, 1, 2):
            case (0, *x, 2):
                y = 0
        self.assertEqual(x, [1])
        self.assertEqual(y, 0)

    def test_patma_130(self) -> None:
        match (0, 1, 2):
            case 0, 1, *x, 2,:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_131(self) -> None:
        match (0, 1, 2):
            case (0, *x, 1, 2):
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_132(self) -> None:
        match (0, 1, 2):
            case *x,:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_133(self) -> None:
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

    def test_patma_134(self) -> None:
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

    def test_patma_135(self) -> None:
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

    def test_patma_136(self) -> None:
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

    def test_patma_137(self) -> None:
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

    def test_patma_138(self) -> None:
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

    def test_patma_139(self) -> None:
        x = False
        match x:
            case bool(z):
                y = 0
        self.assertIs(x, False)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_140(self) -> None:
        x = True
        match x:
            case bool(z):
                y = 0
        self.assertIs(x, True)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_141(self) -> None:
        x = bytearray()
        match x:
            case bytearray(z):
                y = 0
        self.assertEqual(x, bytearray())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_142(self) -> None:
        x = b""
        match x:
            case bytes(z):
                y = 0
        self.assertEqual(x, b"")
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_143(self) -> None:
        x = {}
        match x:
            case dict(z):
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_144(self) -> None:
        x = 0.0
        match x:
            case float(z):
                y = 0
        self.assertEqual(x, 0.0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_145(self) -> None:
        x = frozenset()
        match x:
            case frozenset(z):
                y = 0
        self.assertEqual(x, frozenset())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_146(self) -> None:
        x = 0
        match x:
            case int(z):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_147(self) -> None:
        x = []
        match x:
            case list(z):
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_148(self) -> None:
        x = set()
        match x:
            case set(z):
                y = 0
        self.assertEqual(x, set())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_149(self) -> None:
        x = ""
        match x:
            case str(z):
                y = 0
        self.assertEqual(x, "")
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def test_patma_150(self) -> None:
        x = ()
        match x:
            case tuple(z):
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)
        self.assertIs(z, x)

    def run_perf(self):
        # ./python -m pyperf timeit -s "from test.test_patma import TestMatch; t = TestMatch()" "t.run_perf()"
        attrs = vars(type(self)).items()
        tests = [attr for name, attr in attrs if name.startswith("test_")]
        assert_equal, assert_is = self.assertEqual, self.assertIs
        try:
            self.assertEqual = self.assertIs = lambda *_: None
            for _ in range(1 << 10):
                for test in tests:
                    test(self)
        finally:
            self.assertEqual, self.assertIs = assert_equal, assert_is
