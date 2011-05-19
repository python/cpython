import os
import re
import sys
import shutil
import logging
import unittest as ut1
import packaging.database

from os.path import join
from operator import getitem, setitem, delitem
from packaging.command.build import build
from packaging.tests import unittest
from packaging.tests.support import (TempdirManager, EnvironRestorer,
                                     LoggingCatcher)
from packaging.command.test import test
from packaging.command import set_command
from packaging.dist import Distribution


EXPECTED_OUTPUT_RE = r'''FAIL: test_blah \(myowntestmodule.SomeTest\)
----------------------------------------------------------------------
Traceback \(most recent call last\):
  File ".+/myowntestmodule.py", line \d+, in test_blah
    self.fail\("horribly"\)
AssertionError: horribly
'''

here = os.path.dirname(os.path.abspath(__file__))


class MockBuildCmd(build):
    build_lib = "mock build lib"
    command_name = 'build'
    plat_name = 'whatever'

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        self._record.append("build has run")


class TestTest(TempdirManager,
               EnvironRestorer,
               LoggingCatcher,
               unittest.TestCase):

    restore_environ = ['PYTHONPATH']

    def setUp(self):
        super(TestTest, self).setUp()
        self.addCleanup(packaging.database.clear_cache)
        new_pythonpath = os.path.dirname(os.path.dirname(here))
        pythonpath = os.environ.get('PYTHONPATH')
        if pythonpath is not None:
            new_pythonpath = os.pathsep.join((new_pythonpath, pythonpath))
        os.environ['PYTHONPATH'] = new_pythonpath

    def assert_re_match(self, pattern, string):
        def quote(s):
            lines = ['## ' + line for line in s.split('\n')]
            sep = ["#" * 60]
            return [''] + sep + lines + sep
        msg = quote(pattern) + ["didn't match"] + quote(string)
        msg = "\n".join(msg)
        if not re.search(pattern, string):
            self.fail(msg)

    def prepare_dist(self, dist_name):
        pkg_dir = join(os.path.dirname(__file__), "dists", dist_name)
        temp_pkg_dir = join(self.mkdtemp(), dist_name)
        shutil.copytree(pkg_dir, temp_pkg_dir)
        return temp_pkg_dir

    def safely_replace(self, obj, attr,
                       new_val=None, delete=False, dictionary=False):
        """Replace a object's attribute returning to its original state at the
        end of the test run. Creates the attribute if not present before
        (deleting afterwards). When delete=True, makes sure the value is del'd
        for the test run. If dictionary is set to True, operates of its items
        rather than attributes."""
        if dictionary:
            _setattr, _getattr, _delattr = setitem, getitem, delitem

            def _hasattr(_dict, value):
                return value in _dict
        else:
            _setattr, _getattr, _delattr, _hasattr = (setattr, getattr,
                                                      delattr, hasattr)

        orig_has_attr = _hasattr(obj, attr)
        if orig_has_attr:
            orig_val = _getattr(obj, attr)

        if delete is False:
            _setattr(obj, attr, new_val)
        elif orig_has_attr:
            _delattr(obj, attr)

        def do_cleanup():
            if orig_has_attr:
                _setattr(obj, attr, orig_val)
            elif _hasattr(obj, attr):
                _delattr(obj, attr)

        self.addCleanup(do_cleanup)

    def test_runs_unittest(self):
        module_name, a_module = self.prepare_a_module()
        record = []
        a_module.recorder = lambda *args: record.append("suite")

        class MockTextTestRunner:
            def __init__(*_, **__):
                pass

            def run(_self, suite):
                record.append("run")

        self.safely_replace(ut1, "TextTestRunner", MockTextTestRunner)

        dist = Distribution()
        cmd = test(dist)
        cmd.suite = "%s.recorder" % module_name
        cmd.run()
        self.assertEqual(record, ["suite", "run"])

    def test_builds_before_running_tests(self):
        self.addCleanup(set_command, 'packaging.command.build.build')
        set_command('packaging.tests.test_command_test.MockBuildCmd')

        dist = Distribution()
        dist.get_command_obj('build')._record = record = []
        cmd = test(dist)
        cmd.runner = self.prepare_named_function(lambda: None)
        cmd.ensure_finalized()
        cmd.run()
        self.assertEqual(['build has run'], record)

    def _test_works_with_2to3(self):
        pass

    def test_checks_requires(self):
        dist = Distribution()
        cmd = test(dist)
        phony_project = 'ohno_ohno-impossible_1234-name_stop-that!'
        cmd.tests_require = [phony_project]
        cmd.ensure_finalized()
        logs = self.get_logs(logging.WARNING)
        self.assertEqual(1, len(logs))
        self.assertIn(phony_project, logs[0])

    def prepare_a_module(self):
        tmp_dir = self.mkdtemp()
        sys.path.append(tmp_dir)
        self.addCleanup(sys.path.remove, tmp_dir)

        self.write_file((tmp_dir, 'packaging_tests_a.py'), '')
        import packaging_tests_a as a_module
        return "packaging_tests_a", a_module

    def prepare_named_function(self, func):
        module_name, a_module = self.prepare_a_module()
        a_module.recorder = func
        return "%s.recorder" % module_name

    def test_custom_runner(self):
        dist = Distribution()
        cmd = test(dist)
        record = []
        cmd.runner = self.prepare_named_function(
            lambda: record.append("runner called"))
        cmd.ensure_finalized()
        cmd.run()
        self.assertEqual(["runner called"], record)

    def prepare_mock_ut2(self):
        class MockUTClass:
            def __init__(*_, **__):
                pass

            def discover(self):
                pass

            def run(self, _):
                pass

        class MockUTModule:
            TestLoader = MockUTClass
            TextTestRunner = MockUTClass

        mock_ut2 = MockUTModule()
        self.safely_replace(sys.modules, "unittest2",
                            mock_ut2, dictionary=True)
        return mock_ut2

    def test_gets_unittest_discovery(self):
        mock_ut2 = self.prepare_mock_ut2()
        dist = Distribution()
        cmd = test(dist)
        self.safely_replace(ut1.TestLoader, "discover", lambda: None)
        self.assertEqual(cmd.get_ut_with_discovery(), ut1)

        del ut1.TestLoader.discover
        self.assertEqual(cmd.get_ut_with_discovery(), mock_ut2)

    def test_calls_discover(self):
        self.safely_replace(ut1.TestLoader, "discover", delete=True)
        mock_ut2 = self.prepare_mock_ut2()
        record = []
        mock_ut2.TestLoader.discover = lambda self, path: record.append(path)
        dist = Distribution()
        cmd = test(dist)
        cmd.run()
        self.assertEqual([os.curdir], record)


def test_suite():
    return unittest.makeSuite(TestTest)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
