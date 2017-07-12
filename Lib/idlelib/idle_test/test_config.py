'''Test idlelib.config.

Coverage: 46% (100% for IdleConfParser, IdleUserConfParser*, ConfigChanges).
* Except is OSError clause in Save method.
Much of IdleConf is exercised by ConfigDialog and test_configdialog,
but it should be tested here.
'''
import os
import tempfile
from test.support import captured_stderr, findfile
import unittest
from idlelib import config

# Tests should not depend on fortuitous user configurations.
# They must not affect actual user .cfg files.
# Replace user parsers with empty parsers that cannot be saved
# due to getting '' as the filename when created.

idleConf = config.idleConf
usercfg = idleConf.userCfg
testcfg = {}
usermain = testcfg['main'] = config.IdleUserConfParser('')
userhigh = testcfg['highlight'] = config.IdleUserConfParser('')
userkeys = testcfg['keys'] = config.IdleUserConfParser('')

def setUpModule():
    idleConf.userCfg = testcfg

def tearDownModule():
    idleConf.userCfg = usercfg


class IdleConfParserTest(unittest.TestCase):
    """Test that IdleConfParser works"""

    config = """
        [one]
        one = false
        two = true
        three = 10

        [two]
        one = a string
        two = true
        three = false
    """

    def test_get(self):
        parser = config.IdleConfParser('')
        parser.read_string(self.config)
        eq = self.assertEqual

        # Test with type argument.
        self.assertIs(parser.Get('one', 'one', type='bool'), False)
        self.assertIs(parser.Get('one', 'two', type='bool'), True)
        eq(parser.Get('one', 'three', type='int'), 10)
        eq(parser.Get('two', 'one'), 'a string')
        self.assertIs(parser.Get('two', 'two', type='bool'), True)
        self.assertIs(parser.Get('two', 'three', type='bool'), False)

        # Test without type should fallback to string.
        eq(parser.Get('two', 'two'), 'true')
        eq(parser.Get('two', 'three'), 'false')

        # If option not exist, should return None, or default.
        self.assertIsNone(parser.Get('not', 'exist'))
        eq(parser.Get('not', 'exist', default='DEFAULT'), 'DEFAULT')

    def test_get_option_list(self):
        parser = config.IdleConfParser('')
        parser.read_string(self.config)
        get_list = parser.GetOptionList
        self.assertCountEqual(get_list('one'), ['one', 'two', 'three'])
        self.assertCountEqual(get_list('two'), ['one', 'two', 'three'])
        self.assertEqual(get_list('not exist'), [])

    def test_load_nothing(self):
        parser = config.IdleConfParser('')
        parser.Load()
        self.assertEqual(parser.sections(), [])

    def test_load_file(self):
        # Borrow test/cfgparser.1 from test_configparser.
        config_path = findfile('cfgparser.1')
        parser = config.IdleConfParser(config_path)
        parser.Load()

        self.assertEqual(parser.Get('Foo Bar', 'foo'), 'newbar')
        self.assertEqual(parser.GetOptionList('Foo Bar'), ['foo'])


class IdleUserConfParserTest(unittest.TestCase):
    """Test that IdleUserConfParser works"""

    def new_parser(self, path=''):
        return config.IdleUserConfParser(path)

    def test_set_option(self):
        parser = self.new_parser()
        parser.add_section('Foo')
        # Setting new option in existing section should return True.
        self.assertTrue(parser.SetOption('Foo', 'bar', 'true'))
        # Setting existing option with same value should return False.
        self.assertFalse(parser.SetOption('Foo', 'bar', 'true'))
        # Setting exiting option with new value should return True.
        self.assertTrue(parser.SetOption('Foo', 'bar', 'false'))
        self.assertEqual(parser.Get('Foo', 'bar'), 'false')

        # Setting option in new section should create section and return True.
        self.assertTrue(parser.SetOption('Bar', 'bar', 'true'))
        self.assertCountEqual(parser.sections(), ['Bar', 'Foo'])
        self.assertEqual(parser.Get('Bar', 'bar'), 'true')

    def test_remove_option(self):
        parser = self.new_parser()
        parser.AddSection('Foo')
        parser.SetOption('Foo', 'bar', 'true')

        self.assertTrue(parser.RemoveOption('Foo', 'bar'))
        self.assertFalse(parser.RemoveOption('Foo', 'bar'))
        self.assertFalse(parser.RemoveOption('Not', 'Exist'))

    def test_add_section(self):
        parser = self.new_parser()
        self.assertEqual(parser.sections(), [])

        # Should not add duplicate section.
        # Configparser raises DuplicateError, IdleParser not.
        parser.AddSection('Foo')
        parser.AddSection('Foo')
        parser.AddSection('Bar')
        self.assertCountEqual(parser.sections(), ['Bar', 'Foo'])

    def test_remove_empty_sections(self):
        parser = self.new_parser()

        parser.AddSection('Foo')
        parser.AddSection('Bar')
        parser.SetOption('Idle', 'name', 'val')
        self.assertCountEqual(parser.sections(), ['Bar', 'Foo', 'Idle'])
        parser.RemoveEmptySections()
        self.assertEqual(parser.sections(), ['Idle'])

    def test_is_empty(self):
        parser = self.new_parser()

        parser.AddSection('Foo')
        parser.AddSection('Bar')
        self.assertTrue(parser.IsEmpty())
        self.assertEqual(parser.sections(), [])

        parser.SetOption('Foo', 'bar', 'false')
        parser.AddSection('Bar')
        self.assertFalse(parser.IsEmpty())
        self.assertCountEqual(parser.sections(), ['Foo'])

    def test_remove_file(self):
        with tempfile.TemporaryDirectory() as tdir:
            path = os.path.join(tdir, 'test.cfg')
            parser = self.new_parser(path)
            parser.RemoveFile()  # Should not raise exception.

            parser.AddSection('Foo')
            parser.SetOption('Foo', 'bar', 'true')
            parser.Save()
            self.assertTrue(os.path.exists(path))
            parser.RemoveFile()
            self.assertFalse(os.path.exists(path))

    def test_save(self):
        with tempfile.TemporaryDirectory() as tdir:
            path = os.path.join(tdir, 'test.cfg')
            parser = self.new_parser(path)
            parser.AddSection('Foo')
            parser.SetOption('Foo', 'bar', 'true')

            # Should save to path when config is not empty.
            self.assertFalse(os.path.exists(path))
            parser.Save()
            self.assertTrue(os.path.exists(path))

            # Should remove the file from disk when config is empty.
            parser.remove_section('Foo')
            parser.Save()
            self.assertFalse(os.path.exists(path))


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


class ChangesTest(unittest.TestCase):

    empty = {'main':{}, 'highlight':{}, 'keys':{}, 'extensions':{}}

    def load(self):  # Test_add_option verifies that this works.
        changes = self.changes
        changes.add_option('main', 'Msec', 'mitem', 'mval')
        changes.add_option('highlight', 'Hsec', 'hitem', 'hval')
        changes.add_option('keys', 'Ksec', 'kitem', 'kval')
        return changes

    loaded = {'main': {'Msec': {'mitem': 'mval'}},
              'highlight': {'Hsec': {'hitem': 'hval'}},
              'keys': {'Ksec': {'kitem':'kval'}},
              'extensions': {}}

    def setUp(self):
        self.changes = config.ConfigChanges()

    def test_init(self):
        self.assertEqual(self.changes, self.empty)

    def test_add_option(self):
        changes = self.load()
        self.assertEqual(changes, self.loaded)
        changes.add_option('main', 'Msec', 'mitem', 'mval')
        self.assertEqual(changes, self.loaded)

    def test_save_option(self):  # Static function does not touch changes.
        save_option = self.changes.save_option
        self.assertTrue(save_option('main', 'Indent', 'what', '0'))
        self.assertFalse(save_option('main', 'Indent', 'what', '0'))
        self.assertEqual(usermain['Indent']['what'], '0')

        self.assertTrue(save_option('main', 'Indent', 'use-spaces', '0'))
        self.assertEqual(usermain['Indent']['use-spaces'], '0')
        self.assertTrue(save_option('main', 'Indent', 'use-spaces', '1'))
        self.assertFalse(usermain.has_option('Indent', 'use-spaces'))
        usermain.remove_section('Indent')

    def test_save_added(self):
        changes = self.load()
        self.assertTrue(changes.save_all())
        self.assertEqual(usermain['Msec']['mitem'], 'mval')
        self.assertEqual(userhigh['Hsec']['hitem'], 'hval')
        self.assertEqual(userkeys['Ksec']['kitem'], 'kval')
        changes.add_option('main', 'Msec', 'mitem', 'mval')
        self.assertFalse(changes.save_all())
        usermain.remove_section('Msec')
        userhigh.remove_section('Hsec')
        userkeys.remove_section('Ksec')

    def test_save_help(self):
        # Any change to HelpFiles overwrites entire section.
        changes = self.changes
        changes.save_option('main', 'HelpFiles', 'IDLE', 'idledoc')
        changes.add_option('main', 'HelpFiles', 'ELDI', 'codeldi')
        changes.save_all()
        self.assertFalse(usermain.has_option('HelpFiles', 'IDLE'))
        self.assertTrue(usermain.has_option('HelpFiles', 'ELDI'))

    def test_save_default(self):  # Cover 2nd and 3rd false branches.
        changes = self.changes
        changes.add_option('main', 'Indent', 'use-spaces', '1')
        # save_option returns False; cfg_type_changed remains False.

    # TODO: test that save_all calls usercfg Saves.

    def test_delete_section(self):
        changes = self.load()
        changes.delete_section('main', 'fake')  # Test no exception.
        self.assertEqual(changes, self.loaded)  # Test nothing deleted.
        for cfgtype, section in (('main', 'Msec'), ('keys', 'Ksec')):
            testcfg[cfgtype].SetOption(section, 'name', 'value')
            changes.delete_section(cfgtype, section)
            with self.assertRaises(KeyError):
                changes[cfgtype][section]  # Test section gone from changes
                testcfg[cfgtype][section]  # and from mock userCfg.
        # TODO test for save call.

    def test_clear(self):
        changes = self.load()
        changes.clear()
        self.assertEqual(changes, self.empty)


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
