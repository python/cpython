"Test mainmenu, coverage 100%."
# Reported as 88%; mocking turtledemo absence would have no point.

from idlelib import mainmenu
import unittest


class MainMenuTest(unittest.TestCase):

    def test_menudefs(self):
        actual = list(mainmenu.menudefs.keys())
        expect = ['file', 'edit', 'format', 'run', 'shell',
                  'debug', 'options', 'window', 'help']
        self.assertEqual(actual, expect)

    def test_default_keydefs(self):
        self.assertGreaterEqual(len(mainmenu.default_keydefs), 50)


if __name__ == '__main__':
    unittest.main(verbosity=2)
