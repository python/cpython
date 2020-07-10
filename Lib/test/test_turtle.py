import pickle
import unittest
from test import support
from test.support import import_helper


turtle = import_helper.import_module('turtle')
Vec2D = turtle.Vec2D

test_config = """\
width = 0.75
height = 0.8
canvwidth = 500
canvheight = 200
leftright = 100
topbottom = 100
mode = world
colormode = 255
delay = 100
undobuffersize = 10000
shape = circle
pencolor  = red
fillcolor  = blue
resizemode  = auto
visible  = None
language = english
exampleturtle = turtle
examplescreen = screen
title = Python Turtle Graphics
using_IDLE = ''
"""

test_config_two = """\
# Comments!
# Testing comments!
pencolor  = red
fillcolor  = blue
visible  = False
language = english
# Some more
# comments
using_IDLE = False
"""

invalid_test_config = """
pencolor = red
fillcolor: blue
visible = False
"""


class TurtleConfigTest(unittest.TestCase):

    def get_cfg_file(self, cfg_str):
        self.addCleanup(support.unlink, support.TESTFN)
        with open(support.TESTFN, 'w') as f:
            f.write(cfg_str)
        return support.TESTFN

    def test_config_dict(self):

        cfg_name = self.get_cfg_file(test_config)
        parsed_cfg = turtle.config_dict(cfg_name)

        expected = {
            'width' : 0.75,
            'height' : 0.8,
            'canvwidth' : 500,
            'canvheight': 200,
            'leftright': 100,
            'topbottom': 100,
            'mode': 'world',
            'colormode': 255,
            'delay': 100,
            'undobuffersize': 10000,
            'shape': 'circle',
            'pencolor' : 'red',
            'fillcolor' : 'blue',
            'resizemode' : 'auto',
            'visible' : None,
            'language': 'english',
            'exampleturtle': 'turtle',
            'examplescreen': 'screen',
            'title': 'Python Turtle Graphics',
            'using_IDLE': '',
        }

        self.assertEqual(parsed_cfg, expected)

    def test_partial_config_dict_with_comments(self):

        cfg_name = self.get_cfg_file(test_config_two)
        parsed_cfg = turtle.config_dict(cfg_name)

        expected = {
            'pencolor': 'red',
            'fillcolor': 'blue',
            'visible': False,
            'language': 'english',
            'using_IDLE': False,
        }

        self.assertEqual(parsed_cfg, expected)

    def test_config_dict_invalid(self):

        cfg_name = self.get_cfg_file(invalid_test_config)

        with support.captured_stdout() as stdout:
            parsed_cfg = turtle.config_dict(cfg_name)

        err_msg = stdout.getvalue()

        self.assertIn('Bad line in config-file ', err_msg)
        self.assertIn('fillcolor: blue', err_msg)

        self.assertEqual(parsed_cfg, {
            'pencolor': 'red',
            'visible': False,
        })


class VectorComparisonMixin:

    def assertVectorsAlmostEqual(self, vec1, vec2):
        if len(vec1) != len(vec2):
            self.fail("Tuples are not of equal size")
        for idx, (i, j) in enumerate(zip(vec1, vec2)):
            self.assertAlmostEqual(
                i, j, msg='values at index {} do not match'.format(idx))


class TestVec2D(VectorComparisonMixin, unittest.TestCase):

    def test_constructor(self):
        vec = Vec2D(0.5, 2)
        self.assertEqual(vec[0], 0.5)
        self.assertEqual(vec[1], 2)
        self.assertIsInstance(vec, Vec2D)

        self.assertRaises(TypeError, Vec2D)
        self.assertRaises(TypeError, Vec2D, 0)
        self.assertRaises(TypeError, Vec2D, (0, 1))
        self.assertRaises(TypeError, Vec2D, vec)
        self.assertRaises(TypeError, Vec2D, 0, 1, 2)

    def test_repr(self):
        vec = Vec2D(0.567, 1.234)
        self.assertEqual(repr(vec), '(0.57,1.23)')

    def test_equality(self):
        vec1 = Vec2D(0, 1)
        vec2 = Vec2D(0.0, 1)
        vec3 = Vec2D(42, 1)
        self.assertEqual(vec1, vec2)
        self.assertEqual(vec1, tuple(vec1))
        self.assertEqual(tuple(vec1), vec1)
        self.assertNotEqual(vec1, vec3)
        self.assertNotEqual(vec2, vec3)

    def test_pickling(self):
        vec = Vec2D(0.5, 2)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                pickled = pickle.dumps(vec, protocol=proto)
                unpickled = pickle.loads(pickled)
                self.assertEqual(unpickled, vec)
                self.assertIsInstance(unpickled, Vec2D)

    def _assert_arithmetic_cases(self, test_cases, lambda_operator):
        for test_case in test_cases:
            with self.subTest(case=test_case):

                ((first, second), expected) = test_case

                op1 = Vec2D(*first)
                op2 = Vec2D(*second)

                result = lambda_operator(op1, op2)

                expected = Vec2D(*expected)

                self.assertVectorsAlmostEqual(result, expected)

    def test_vector_addition(self):

        test_cases = [
            (((0, 0), (1, 1)), (1.0, 1.0)),
            (((-1, 0), (2, 2)), (1, 2)),
            (((1.5, 0), (1, 1)), (2.5, 1)),
        ]

        self._assert_arithmetic_cases(test_cases, lambda x, y: x + y)

    def test_vector_subtraction(self):

        test_cases = [
            (((0, 0), (1, 1)), (-1, -1)),
            (((10.625, 0.125), (10, 0)), (0.625, 0.125)),
        ]

        self._assert_arithmetic_cases(test_cases, lambda x, y: x - y)

    def test_vector_multiply(self):

        vec1 = Vec2D(10, 10)
        vec2 = Vec2D(0.5, 3)
        answer = vec1 * vec2
        expected = 35
        self.assertAlmostEqual(answer, expected)

        vec = Vec2D(0.5, 3)
        answer = vec * 10
        expected = Vec2D(5, 30)
        self.assertVectorsAlmostEqual(answer, expected)

    def test_vector_negative(self):
        vec = Vec2D(10, -10)
        expected = (-10, 10)
        self.assertVectorsAlmostEqual(-vec, expected)

    def test_distance(self):
        vec = Vec2D(6, 8)
        expected = 10
        self.assertEqual(abs(vec), expected)

        vec = Vec2D(0, 0)
        expected = 0
        self.assertEqual(abs(vec), expected)

        vec = Vec2D(2.5, 6)
        expected = 6.5
        self.assertEqual(abs(vec), expected)

    def test_rotate(self):

        cases = [
            (((0, 0), 0), (0, 0)),
            (((0, 1), 90), (-1, 0)),
            (((0, 1), -90), (1, 0)),
            (((1, 0), 180), (-1, 0)),
            (((1, 0), 360), (1, 0)),
        ]

        for case in cases:
            with self.subTest(case=case):
                (vec, rot), expected = case
                vec = Vec2D(*vec)
                got = vec.rotate(rot)
                self.assertVectorsAlmostEqual(got, expected)


class TestTNavigator(VectorComparisonMixin, unittest.TestCase):

    def setUp(self):
        self.nav = turtle.TNavigator()

    def test_goto(self):
        self.nav.goto(100, -100)
        self.assertAlmostEqual(self.nav.xcor(), 100)
        self.assertAlmostEqual(self.nav.ycor(), -100)

    def test_pos(self):
        self.assertEqual(self.nav.pos(), self.nav._position)
        self.nav.goto(100, -100)
        self.assertEqual(self.nav.pos(), self.nav._position)

    def test_left(self):
        self.assertEqual(self.nav._orient, (1.0, 0))
        self.nav.left(90)
        self.assertVectorsAlmostEqual(self.nav._orient, (0.0, 1.0))

    def test_right(self):
        self.assertEqual(self.nav._orient, (1.0, 0))
        self.nav.right(90)
        self.assertVectorsAlmostEqual(self.nav._orient, (0, -1.0))

    def test_reset(self):
        self.nav.goto(100, -100)
        self.assertAlmostEqual(self.nav.xcor(), 100)
        self.assertAlmostEqual(self.nav.ycor(), -100)
        self.nav.reset()
        self.assertAlmostEqual(self.nav.xcor(), 0)
        self.assertAlmostEqual(self.nav.ycor(), 0)

    def test_forward(self):
        self.nav.forward(150)
        expected = Vec2D(150, 0)
        self.assertVectorsAlmostEqual(self.nav.position(), expected)

        self.nav.reset()
        self.nav.left(90)
        self.nav.forward(150)
        expected = Vec2D(0, 150)
        self.assertVectorsAlmostEqual(self.nav.position(), expected)

        self.assertRaises(TypeError, self.nav.forward, 'skldjfldsk')

    def test_backwards(self):
        self.nav.back(200)
        expected = Vec2D(-200, 0)
        self.assertVectorsAlmostEqual(self.nav.position(), expected)

        self.nav.reset()
        self.nav.right(90)
        self.nav.back(200)
        expected = Vec2D(0, 200)
        self.assertVectorsAlmostEqual(self.nav.position(), expected)

    def test_distance(self):
        self.nav.forward(100)
        expected = 100
        self.assertAlmostEqual(self.nav.distance(Vec2D(0,0)), expected)

    def test_radians_and_degrees(self):
        self.nav.left(90)
        self.assertAlmostEqual(self.nav.heading(), 90)
        self.nav.radians()
        self.assertAlmostEqual(self.nav.heading(), 1.57079633)
        self.nav.degrees()
        self.assertAlmostEqual(self.nav.heading(), 90)

    def test_towards(self):

        coordinates = [
            # coordinates, expected
            ((100, 0), 0.0),
            ((100, 100), 45.0),
            ((0, 100), 90.0),
            ((-100, 100), 135.0),
            ((-100, 0), 180.0),
            ((-100, -100), 225.0),
            ((0, -100), 270.0),
            ((100, -100), 315.0),
        ]

        for (x, y), expected in coordinates:
            self.assertEqual(self.nav.towards(x, y), expected)
            self.assertEqual(self.nav.towards((x, y)), expected)
            self.assertEqual(self.nav.towards(Vec2D(x, y)), expected)

    def test_heading(self):

        self.nav.left(90)
        self.assertAlmostEqual(self.nav.heading(), 90)
        self.nav.left(45)
        self.assertAlmostEqual(self.nav.heading(), 135)
        self.nav.right(1.6)
        self.assertAlmostEqual(self.nav.heading(), 133.4)
        self.assertRaises(TypeError, self.nav.right, 'sdkfjdsf')
        self.nav.reset()

        rotations = [10, 20, 170, 300]
        result = sum(rotations) % 360
        for num in rotations:
            self.nav.left(num)
        self.assertEqual(self.nav.heading(), result)
        self.nav.reset()

        result = (360-sum(rotations)) % 360
        for num in rotations:
            self.nav.right(num)
        self.assertEqual(self.nav.heading(), result)
        self.nav.reset()

        rotations = [10, 20, -170, 300, -210, 34.3, -50.2, -10, -29.98, 500]
        sum_so_far = 0
        for num in rotations:
            if num < 0:
                self.nav.right(abs(num))
            else:
                self.nav.left(num)
            sum_so_far += num
            self.assertAlmostEqual(self.nav.heading(), sum_so_far % 360)

    def test_setheading(self):
        self.nav.setheading(102.32)
        self.assertAlmostEqual(self.nav.heading(), 102.32)
        self.nav.setheading(-123.23)
        self.assertAlmostEqual(self.nav.heading(), (-123.23) % 360)
        self.nav.setheading(-1000.34)
        self.assertAlmostEqual(self.nav.heading(), (-1000.34) % 360)
        self.nav.setheading(300000)
        self.assertAlmostEqual(self.nav.heading(), 300000%360)

    def test_positions(self):
        self.nav.forward(100)
        self.nav.left(90)
        self.nav.forward(-200)
        self.assertVectorsAlmostEqual(self.nav.pos(), (100.0, -200.0))

    def test_setx_and_sety(self):
        self.nav.setx(-1023.2334)
        self.nav.sety(193323.234)
        self.assertVectorsAlmostEqual(self.nav.pos(), (-1023.2334, 193323.234))

    def test_home(self):
        self.nav.left(30)
        self.nav.forward(-100000)
        self.nav.home()
        self.assertVectorsAlmostEqual(self.nav.pos(), (0,0))
        self.assertAlmostEqual(self.nav.heading(), 0)

    def test_distance_method(self):
        self.assertAlmostEqual(self.nav.distance(30, 40), 50)
        vec = Vec2D(0.22, .001)
        self.assertAlmostEqual(self.nav.distance(vec), 0.22000227271553355)
        another_turtle = turtle.TNavigator()
        another_turtle.left(90)
        another_turtle.forward(10000)
        self.assertAlmostEqual(self.nav.distance(another_turtle), 10000)


class TestTPen(unittest.TestCase):

    def test_pendown_and_penup(self):

        tpen = turtle.TPen()

        self.assertTrue(tpen.isdown())
        tpen.penup()
        self.assertFalse(tpen.isdown())
        tpen.pendown()
        self.assertTrue(tpen.isdown())

    def test_showturtle_hideturtle_and_isvisible(self):

        tpen = turtle.TPen()

        self.assertTrue(tpen.isvisible())
        tpen.hideturtle()
        self.assertFalse(tpen.isvisible())
        tpen.showturtle()
        self.assertTrue(tpen.isvisible())


if __name__ == '__main__':
    unittest.main()
