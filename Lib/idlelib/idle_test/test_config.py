'''Test idlelib.config.

Much is tested by opening config dialog live or in test_configdialog.
Coverage: 27%
'''
from test.support import captured_stderr
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
    """ Test colorkeys function with user config [Theme] and [Keys] patterns.

        colorkeys = config.IdleConf.current_colors_and_keys
        Test all patterns written by IDLE and some errors
        Item 'default' should really be 'builtin' (versus 'custom).
    """
    colorkeys = idleConf.current_colors_and_keys
    default_theme = 'IDLE Classic'
    default_keys = idleConf.default_keys()

    def test_old_builtin_theme(self):
        # On initial installation, user main is blank.
        self.assertEqual(self.colorkeys('Theme'), self.default_theme)
        # For old default, name2 must be blank.
        usermain.read_string('''
            [Theme]
            default = True
            ''')
        # IDLE omits 'name' for default old builtin theme.
        self.assertEqual(self.colorkeys('Theme'), self.default_theme)
        # IDLE adds 'name' for non-default old builtin theme.
        usermain['Theme']['name'] = 'IDLE New'
        self.assertEqual(self.colorkeys('Theme'), 'IDLE New')
        # Erroneous non-default old builtin reverts to default.
        usermain['Theme']['name'] = 'non-existent'
        self.assertEqual(self.colorkeys('Theme'), self.default_theme)
        usermain.remove_section('Theme')

    def test_new_builtin_theme(self):
        # IDLE writes name2 for new builtins.
        usermain.read_string('''
            [Theme]
            default = True
            name2 = IDLE Dark
            ''')
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Dark')
        # Leftover 'name', not removed, is ignored.
        usermain['Theme']['name'] = 'IDLE New'
        self.assertEqual(self.colorkeys('Theme'), 'IDLE Dark')
        # Erroneous non-default new builtin reverts to default.
        usermain['Theme']['name2'] = 'non-existent'
        self.assertEqual(self.colorkeys('Theme'), self.default_theme)
        usermain.remove_section('Theme')

    def test_user_override_theme(self):
        # Erroneous custom name (no definition) reverts to default.
        usermain.read_string('''
            [Theme]
            default = False
            name = Custom Dark
            ''')
        self.assertEqual(self.colorkeys('Theme'), self.default_theme)
        # Custom name is valid with matching Section name.
        userhigh.read_string('[Custom Dark]\na=b')
        self.assertEqual(self.colorkeys('Theme'), 'Custom Dark')
        # Name2 is ignored.
        usermain['Theme']['name2'] = 'non-existent'
        self.assertEqual(self.colorkeys('Theme'), 'Custom Dark')
        usermain.remove_section('Theme')
        userhigh.remove_section('Custom Dark')

    def test_old_builtin_keys(self):
        # On initial installation, user main is blank.
        self.assertEqual(self.colorkeys('Keys'), self.default_keys)
        # For old default, name2 must be blank, name is always used.
        usermain.read_string('''
            [Keys]
            default = True
            name = IDLE Classic Unix
            ''')
        self.assertEqual(self.colorkeys('Keys'), 'IDLE Classic Unix')
        # Erroneous non-default old builtin reverts to default.
        usermain['Keys']['name'] = 'non-existent'
        self.assertEqual(self.colorkeys('Keys'), self.default_keys)
        usermain.remove_section('Keys')

    def test_new_builtin_keys(self):
        # IDLE writes name2 for new builtins.
        usermain.read_string('''
            [Keys]
            default = True
            name2 = IDLE Modern Unix
            ''')
        self.assertEqual(self.colorkeys('Keys'), 'IDLE Modern Unix')
        # Leftover 'name', not removed, is ignored.
        usermain['Keys']['name'] = 'IDLE Classic Unix'
        self.assertEqual(self.colorkeys('Keys'), 'IDLE Modern Unix')
        # Erroneous non-default new builtin reverts to default.
        usermain['Keys']['name2'] = 'non-existent'
        self.assertEqual(self.colorkeys('Keys'), self.default_keys)
        usermain.remove_section('Keys')

    def test_user_override_keys(self):
        # Erroneous custom name (no definition) reverts to default.
        usermain.read_string('''
            [Keys]
            default = False
            name = Custom Keys
            ''')
        self.assertEqual(self.colorkeys('Keys'), self.default_keys)
        # Custom name is valid with matching Section name.
        userkeys.read_string('[Custom Keys]\na=b')
        self.assertEqual(self.colorkeys('Keys'), 'Custom Keys')
        # Name2 is ignored.
        usermain['Keys']['name2'] = 'non-existent'
        self.assertEqual(self.colorkeys('Keys'), 'Custom Keys')
        usermain.remove_section('Keys')
        userkeys.remove_section('Custom Keys')


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
