import os
import pickle
import re
import tempfile
import unittest
import unittest.mock
from test import support
from test.support import import_helper
from test.support import os_helper


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


def patch_screen():
    """Patch turtle._Screen for testing without a display.

    We must patch the _Screen class itself instead of the _Screen
    instance because instantiating it requires a display.
    """
    # Create a mock screen that delegates color validation to the real TurtleScreen methods
    mock_screen = unittest.mock.MagicMock()
    mock_screen.__class__ = turtle._Screen
    mock_screen.mode.return_value = "standard"
    mock_screen._colormode = 1.0

    def mock_iscolorstring(color):
        valid_colors = {'red', 'green', 'blue', 'black', 'white', 'yellow',
                        'orange', 'purple', 'pink', 'brown', 'gray', 'grey',
                        'cyan', 'magenta'}

        return color in valid_colors or (isinstance(color, str) and color.startswith('#'))

    mock_screen._iscolorstring = mock_iscolorstring
    mock_screen._colorstr = turtle._Screen._colorstr.__get__(mock_screen)

    return unittest.mock.patch(
        "turtle._Screen.__new__",
        return_value=mock_screen
    )


class TurtleConfigTest(unittest.TestCase):

    def get_cfg_file(self, cfg_str):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, 'w') as f:
            f.write(cfg_str)
        return os_helper.TESTFN

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


class Multiplier:

    def __mul__(self, other):
        return f'M*{other}'

    def __rmul__(self, other):
        return f'{other}*M'


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
        expected = Vec2D(5, 30)
        self.assertVectorsAlmostEqual(vec * 10, expected)
        self.assertVectorsAlmostEqual(10 * vec, expected)
        self.assertVectorsAlmostEqual(vec * 10.0, expected)
        self.assertVectorsAlmostEqual(10.0 * vec, expected)

        M = Multiplier()
        self.assertEqual(vec * M, Vec2D(f"{vec[0]}*M", f"{vec[1]}*M"))
        self.assertEqual(M * vec, f'M*{vec}')

    def test_vector_negative(self):
        vec = Vec2D(10, -10)
        expected = (-10, 10)
        self.assertVectorsAlmostEqual(-vec, expected)

    def test_distance(self):
        self.assertAlmostEqual(abs(Vec2D(6, 8)), 10)
        self.assertEqual(abs(Vec2D(0, 0)), 0)
        self.assertAlmostEqual(abs(Vec2D(2.5, 6)), 6.5)

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

    def test_teleport(self):
        self.nav.teleport(20, -30, fill_gap=True)
        self.assertAlmostEqual(self.nav.xcor(), 20)
        self.assertAlmostEqual(self.nav.ycor(), -30)
        self.nav.teleport(-20, 30, fill_gap=False)
        self.assertAlmostEqual(self.nav.xcor(), -20)
        self.assertAlmostEqual(self.nav.ycor(), 30)

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

    def test_teleport(self):

        tpen = turtle.TPen()

        for fill_gap_value in [True, False]:
            tpen.penup()
            tpen.teleport(100, 100, fill_gap=fill_gap_value)
            self.assertFalse(tpen.isdown())
            tpen.pendown()
            tpen.teleport(-100, -100, fill_gap=fill_gap_value)
            self.assertTrue(tpen.isdown())


class TestTurtleScreen(unittest.TestCase):
    def test_save_raises_if_wrong_extension(self) -> None:
        screen = unittest.mock.Mock()

        msg = "Unknown file extension: '.png', must be one of {'.ps', '.eps'}"
        with (
            tempfile.TemporaryDirectory() as tmpdir,
            self.assertRaisesRegex(ValueError, re.escape(msg))
        ):
            turtle.TurtleScreen.save(screen, os.path.join(tmpdir, "file.png"))

    def test_save_raises_if_parent_not_found(self) -> None:
        screen = unittest.mock.Mock()

        with tempfile.TemporaryDirectory() as tmpdir:
            parent = os.path.join(tmpdir, "unknown_parent")
            msg = f"The directory '{parent}' does not exist. Cannot save to it"

            with self.assertRaisesRegex(FileNotFoundError, re.escape(msg)):
                turtle.TurtleScreen.save(screen, os.path.join(parent, "a.ps"))

    def test_save_raises_if_file_found(self) -> None:
        screen = unittest.mock.Mock()

        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = os.path.join(tmpdir, "some_file.ps")
            with open(file_path, "w") as f:
                f.write("some text")

            msg = (
                f"The file '{file_path}' already exists. To overwrite it use"
                " the 'overwrite=True' argument of the save function."
            )
            with self.assertRaisesRegex(FileExistsError, re.escape(msg)):
                turtle.TurtleScreen.save(screen, file_path)

    def test_save_overwrites_if_specified(self) -> None:
        screen = unittest.mock.Mock()
        screen.cv.postscript.return_value = "postscript"

        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = os.path.join(tmpdir, "some_file.ps")
            with open(file_path, "w") as f:
                f.write("some text")

            turtle.TurtleScreen.save(screen, file_path, overwrite=True)
            with open(file_path) as f:
                self.assertEqual(f.read(), "postscript")

    def test_save(self) -> None:
        screen = unittest.mock.Mock()
        screen.cv.postscript.return_value = "postscript"

        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = os.path.join(tmpdir, "some_file.ps")

            turtle.TurtleScreen.save(screen, file_path)
            with open(file_path) as f:
                self.assertEqual(f.read(), "postscript")

    def test_no_animation_sets_tracer_0(self):
        s = turtle.TurtleScreen(cv=unittest.mock.MagicMock())

        with s.no_animation():
            self.assertEqual(s.tracer(), 0)

    def test_no_animation_resets_tracer_to_old_value(self):
        s = turtle.TurtleScreen(cv=unittest.mock.MagicMock())

        for tracer in [0, 1, 5]:
            s.tracer(tracer)
            with s.no_animation():
                pass
            self.assertEqual(s.tracer(), tracer)

    def test_no_animation_calls_update_at_exit(self):
        s = turtle.TurtleScreen(cv=unittest.mock.MagicMock())
        s.update = unittest.mock.MagicMock()

        with s.no_animation():
            s.update.assert_not_called()
        s.update.assert_called_once()


class TestTurtle(unittest.TestCase):
    def setUp(self):
        with patch_screen():
            self.turtle = turtle.Turtle()

        # Reset the Screen singleton to avoid reference leaks
        self.addCleanup(setattr, turtle.Turtle, '_screen', None)

    def test_begin_end_fill(self):
        self.assertFalse(self.turtle.filling())
        self.turtle.begin_fill()
        self.assertTrue(self.turtle.filling())
        self.turtle.end_fill()
        self.assertFalse(self.turtle.filling())

    def test_fill(self):
        # The context manager behaves like begin_fill and end_fill.
        self.assertFalse(self.turtle.filling())
        with self.turtle.fill():
            self.assertTrue(self.turtle.filling())
        self.assertFalse(self.turtle.filling())

    def test_fill_resets_after_exception(self):
        # The context manager cleans up correctly after exceptions.
        try:
            with self.turtle.fill():
                self.assertTrue(self.turtle.filling())
                raise ValueError
        except ValueError:
            self.assertFalse(self.turtle.filling())

    def test_fill_context_when_filling(self):
        # The context manager works even when the turtle is already filling.
        self.turtle.begin_fill()
        self.assertTrue(self.turtle.filling())
        with self.turtle.fill():
            self.assertTrue(self.turtle.filling())
        self.assertFalse(self.turtle.filling())

    def test_begin_end_poly(self):
        self.assertFalse(self.turtle._creatingPoly)
        self.turtle.begin_poly()
        self.assertTrue(self.turtle._creatingPoly)
        self.turtle.end_poly()
        self.assertFalse(self.turtle._creatingPoly)

    def test_poly(self):
        # The context manager behaves like begin_poly and end_poly.
        self.assertFalse(self.turtle._creatingPoly)
        with self.turtle.poly():
            self.assertTrue(self.turtle._creatingPoly)
        self.assertFalse(self.turtle._creatingPoly)

    def test_poly_resets_after_exception(self):
        # The context manager cleans up correctly after exceptions.
        try:
            with self.turtle.poly():
                self.assertTrue(self.turtle._creatingPoly)
                raise ValueError
        except ValueError:
            self.assertFalse(self.turtle._creatingPoly)

    def test_poly_context_when_creating_poly(self):
        # The context manager works when the turtle is already creating poly.
        self.turtle.begin_poly()
        self.assertTrue(self.turtle._creatingPoly)
        with self.turtle.poly():
            self.assertTrue(self.turtle._creatingPoly)
        self.assertFalse(self.turtle._creatingPoly)

    def test_dot_signature(self):
        self.turtle.dot()
        self.turtle.dot(10)
        self.turtle.dot(size=10)
        self.turtle.dot((0, 0, 0))
        self.turtle.dot(size=(0, 0, 0))
        self.turtle.dot("blue")
        self.turtle.dot("")
        self.turtle.dot(size="blue")
        self.turtle.dot(20, "blue")
        self.turtle.dot(20, "blue")
        self.turtle.dot(20, (0, 0, 0))
        self.turtle.dot(20, 0, 0, 0)
        with self.assertRaises(TypeError):
            self.turtle.dot(color="blue")
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, "_not_a_color_")
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, 0, (0, 0, 0, 0))
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, 0, 0, 0, 0, 0)
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, 0, (-1, 0, 0))
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, 0, -1, 0, 0)
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, 0, (0, 257, 0))
        self.assertRaises(turtle.TurtleGraphicsError, self.turtle.dot, 0, 0, 257, 0)

class TestModuleLevel(unittest.TestCase):
    def test_all_signatures(self):
        import inspect

        known_signatures = {
            'teleport':
                '(x=None, y=None, *, fill_gap: bool = False) -> None',
            'undo': '()',
            'goto': '(x, y=None)',
            'bgcolor': '(*args)',
            'pen': '(pen=None, **pendict)',
        }

        for name in known_signatures:
            with self.subTest(name=name):
                obj = getattr(turtle, name)
                sig = inspect.signature(obj)
                self.assertEqual(str(sig), known_signatures[name])


if __name__ == '__main__':
    unittest.main()
