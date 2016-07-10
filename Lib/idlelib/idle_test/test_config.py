'''Test idlelib.config.

Much is tested by opening config dialog live or in test_configdialog.
Coverage: 27%
'''
from sys import modules
from test.support import captured_stderr
from tkinter import Tk
import unittest
from idlelib import config

# Tests should not depend on fortuitous user configurations.
# They must not affect actual user .cfg files.
# Replace user parsers with empty parsers that cannot be saved.

idleConf = config.idleConf
usercfg = idleConf.userCfg
testcfg = {}
usermain = testcfg['main'] = config.IdleUserConfParser('')  # filename
userhigh = testcfg['highlight'] = config.IdleUserConfParser('')
userkeys = testcfg['keys'] = config.IdleUserConfParser('')

def setUpModule():
    idleConf.userCfg = testcfg

def tearDownModule():
    idleConf.userCfg = usercfg


class CurrentColorKeysTest(unittest.TestCase):
    """Test correct scenarios for colorkeys and wrap functions.

        The 5 correct patterns are possible results of config dialog.
    """
    colorkeys = idleConf.current_colors_and_keys

    def test_old_default(self):
        # name2 must be blank
        usermain.read_string('''
            [Theme]
            default= 1
            ''')
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Classic')
        usermain['Theme']['name'] = 'IDLE New'
        self.assertEqual(self.colorkeys('Theme'), 'IDLE New')
        usermain['Theme']['name'] = 'non-default'  # error
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Classic')
        usermain.remove_section('Theme')

    def test_new_default(self):
        # name2 overrides name
        usermain.read_string('''
            [Theme]
            default= 1
            name= IDLE New
            name2= IDLE Dark
            ''')
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Dark')
        usermain['Theme']['name2'] = 'non-default'  # error
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Classic')
        usermain.remove_section('Theme')

    def test_user_override(self):
        # name2 does not matter
        usermain.read_string('''
            [Theme]
            default= 0
            name= Custom Dark
            ''')  # error until set userhigh
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Classic')
        userhigh.read_string('[Custom Dark]\na=b')
        self.assertEqual(self.colorkeys('Theme'), 'Custom Dark')
        usermain['Theme']['name2'] = 'IDLE Dark'
        self.assertEqual(self.colorkeys('Theme'), 'Custom Dark')
        usermain.remove_section('Theme')
        userhigh.remove_section('Custom Dark')


class WarningTest(unittest.TestCase):

    def test_warn(self):
        Equal = self.assertEqual
        config._warned = set()
        with captured_stderr() as stderr:
            config._warn('warning', 'key')
        Equal(config._warned, {('warning','key')})
        Equal(stderr.getvalue(), 'warning'+'\n')
        with captured_stderr() as stderr:
            config._warn('warning', 'key')
        Equal(stderr.getvalue(), '')
        with captured_stderr() as stderr:
            config._warn('warn2', 'yek')
        Equal(config._warned, {('warning','key'), ('warn2','yek')})
        Equal(stderr.getvalue(), 'warn2'+'\n')


if __name__ == '__main__':
    unittest.main(verbosity=2)
