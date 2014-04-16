import unittest
import unittest.mock
import test.support
import os
import os.path
import contextlib
import sys

import ensurepip
import ensurepip._uninstall

# pip currently requires ssl support, so we ensure we handle
# it being missing (http://bugs.python.org/issue19744)
ensurepip_no_ssl = test.support.import_fresh_module("ensurepip",
                                                    blocked=["ssl"])
try:
    import ssl
except ImportError:
    def requires_usable_pip(f):
        deco = unittest.skip(ensurepip._MISSING_SSL_MESSAGE)
        return deco(f)
else:
    def requires_usable_pip(f):
        return f

class TestEnsurePipVersion(unittest.TestCase):

    def test_returns_version(self):
        self.assertEqual(ensurepip._PIP_VERSION, ensurepip.version())

class EnsurepipMixin:

    def setUp(self):
        run_pip_patch = unittest.mock.patch("ensurepip._run_pip")
        self.run_pip = run_pip_patch.start()
        self.addCleanup(run_pip_patch.stop)

        # Avoid side effects on the actual os module
        real_devnull = os.devnull
        os_patch = unittest.mock.patch("ensurepip.os")
        patched_os = os_patch.start()
        self.addCleanup(os_patch.stop)
        patched_os.devnull = real_devnull
        patched_os.path = os.path
        self.os_environ = patched_os.environ = os.environ.copy()


class TestBootstrap(EnsurepipMixin, unittest.TestCase):

    @requires_usable_pip
    def test_basic_bootstrapping(self):
        ensurepip.bootstrap()

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 2)

    @requires_usable_pip
    def test_bootstrapping_with_root(self):
        ensurepip.bootstrap(root="/foo/bar/")

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--root", "/foo/bar/",
                "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    @requires_usable_pip
    def test_bootstrapping_with_user(self):
        ensurepip.bootstrap(user=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--user", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    @requires_usable_pip
    def test_bootstrapping_with_upgrade(self):
        ensurepip.bootstrap(upgrade=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--upgrade", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    @requires_usable_pip
    def test_bootstrapping_with_verbosity_1(self):
        ensurepip.bootstrap(verbosity=1)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "-v", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    @requires_usable_pip
    def test_bootstrapping_with_verbosity_2(self):
        ensurepip.bootstrap(verbosity=2)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "-vv", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    @requires_usable_pip
    def test_bootstrapping_with_verbosity_3(self):
        ensurepip.bootstrap(verbosity=3)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "-vvv", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    @requires_usable_pip
    def test_bootstrapping_with_regular_install(self):
        ensurepip.bootstrap()
        self.assertEqual(self.os_environ["ENSUREPIP_OPTIONS"], "install")

    @requires_usable_pip
    def test_bootstrapping_with_alt_install(self):
        ensurepip.bootstrap(altinstall=True)
        self.assertEqual(self.os_environ["ENSUREPIP_OPTIONS"], "altinstall")

    @requires_usable_pip
    def test_bootstrapping_with_default_pip(self):
        ensurepip.bootstrap(default_pip=True)
        self.assertNotIn("ENSUREPIP_OPTIONS", self.os_environ)

    def test_altinstall_default_pip_conflict(self):
        with self.assertRaises(ValueError):
            ensurepip.bootstrap(altinstall=True, default_pip=True)
        self.assertFalse(self.run_pip.called)

    @requires_usable_pip
    def test_pip_environment_variables_removed(self):
        # ensurepip deliberately ignores all pip environment variables
        # See http://bugs.python.org/issue19734 for details
        self.os_environ["PIP_THIS_SHOULD_GO_AWAY"] = "test fodder"
        ensurepip.bootstrap()
        self.assertNotIn("PIP_THIS_SHOULD_GO_AWAY", self.os_environ)

    @requires_usable_pip
    def test_pip_config_file_disabled(self):
        # ensurepip deliberately ignores the pip config file
        # See http://bugs.python.org/issue20053 for details
        ensurepip.bootstrap()
        self.assertEqual(self.os_environ["PIP_CONFIG_FILE"], os.devnull)

@contextlib.contextmanager
def fake_pip(version=ensurepip._PIP_VERSION):
    if version is None:
        pip = None
    else:
        class FakePip():
            __version__ = version
        pip = FakePip()
    sentinel = object()
    orig_pip = sys.modules.get("pip", sentinel)
    sys.modules["pip"] = pip
    try:
        yield pip
    finally:
        if orig_pip is sentinel:
            del sys.modules["pip"]
        else:
            sys.modules["pip"] = orig_pip

class TestUninstall(EnsurepipMixin, unittest.TestCase):

    def test_uninstall_skipped_when_not_installed(self):
        with fake_pip(None):
            ensurepip._uninstall_helper()
        self.assertFalse(self.run_pip.called)

    def test_uninstall_skipped_with_warning_for_wrong_version(self):
        with fake_pip("not a valid version"):
            with test.support.captured_stderr() as stderr:
                ensurepip._uninstall_helper()
        warning = stderr.getvalue().strip()
        self.assertIn("only uninstall a matching version", warning)
        self.assertFalse(self.run_pip.called)


    @requires_usable_pip
    def test_uninstall(self):
        with fake_pip():
            ensurepip._uninstall_helper()

        self.run_pip.assert_called_once_with(
            ["uninstall", "-y", "pip", "setuptools"]
        )

    @requires_usable_pip
    def test_uninstall_with_verbosity_1(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=1)

        self.run_pip.assert_called_once_with(
            ["uninstall", "-y", "-v", "pip", "setuptools"]
        )

    @requires_usable_pip
    def test_uninstall_with_verbosity_2(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=2)

        self.run_pip.assert_called_once_with(
            ["uninstall", "-y", "-vv", "pip", "setuptools"]
        )

    @requires_usable_pip
    def test_uninstall_with_verbosity_3(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=3)

        self.run_pip.assert_called_once_with(
            ["uninstall", "-y", "-vvv", "pip", "setuptools"]
        )

    @requires_usable_pip
    def test_pip_environment_variables_removed(self):
        # ensurepip deliberately ignores all pip environment variables
        # See http://bugs.python.org/issue19734 for details
        self.os_environ["PIP_THIS_SHOULD_GO_AWAY"] = "test fodder"
        with fake_pip():
            ensurepip._uninstall_helper()
        self.assertNotIn("PIP_THIS_SHOULD_GO_AWAY", self.os_environ)

    @requires_usable_pip
    def test_pip_config_file_disabled(self):
        # ensurepip deliberately ignores the pip config file
        # See http://bugs.python.org/issue20053 for details
        with fake_pip():
            ensurepip._uninstall_helper()
        self.assertEqual(self.os_environ["PIP_CONFIG_FILE"], os.devnull)


class TestMissingSSL(EnsurepipMixin, unittest.TestCase):

    def setUp(self):
        sys.modules["ensurepip"] = ensurepip_no_ssl
        @self.addCleanup
        def restore_module():
            sys.modules["ensurepip"] = ensurepip
        super().setUp()

    def test_bootstrap_requires_ssl(self):
        self.os_environ["PIP_THIS_SHOULD_STAY"] = "test fodder"
        with self.assertRaisesRegex(RuntimeError, "requires SSL/TLS"):
            ensurepip_no_ssl.bootstrap()
        self.assertFalse(self.run_pip.called)
        self.assertIn("PIP_THIS_SHOULD_STAY", self.os_environ)

    def test_uninstall_requires_ssl(self):
        self.os_environ["PIP_THIS_SHOULD_STAY"] = "test fodder"
        with self.assertRaisesRegex(RuntimeError, "requires SSL/TLS"):
            with fake_pip():
                ensurepip_no_ssl._uninstall_helper()
        self.assertFalse(self.run_pip.called)
        self.assertIn("PIP_THIS_SHOULD_STAY", self.os_environ)

    def test_main_exits_early_with_warning(self):
        with test.support.captured_stderr() as stderr:
            ensurepip_no_ssl._main(["--version"])
        warning = stderr.getvalue().strip()
        self.assertTrue(warning.endswith("requires SSL/TLS"), warning)
        self.assertFalse(self.run_pip.called)

# Basic testing of the main functions and their argument parsing

EXPECTED_VERSION_OUTPUT = "pip " + ensurepip._PIP_VERSION

class TestBootstrappingMainFunction(EnsurepipMixin, unittest.TestCase):

    @requires_usable_pip
    def test_bootstrap_version(self):
        with test.support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                ensurepip._main(["--version"])
        result = stdout.getvalue().strip()
        self.assertEqual(result, EXPECTED_VERSION_OUTPUT)
        self.assertFalse(self.run_pip.called)

    @requires_usable_pip
    def test_basic_bootstrapping(self):
        ensurepip._main([])

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 2)

class TestUninstallationMainFunction(EnsurepipMixin, unittest.TestCase):

    def test_uninstall_version(self):
        with test.support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                ensurepip._uninstall._main(["--version"])
        result = stdout.getvalue().strip()
        self.assertEqual(result, EXPECTED_VERSION_OUTPUT)
        self.assertFalse(self.run_pip.called)

    @requires_usable_pip
    def test_basic_uninstall(self):
        with fake_pip():
            ensurepip._uninstall._main([])

        self.run_pip.assert_called_once_with(
            ["uninstall", "-y", "pip", "setuptools"]
        )



if __name__ == "__main__":
    test.support.run_unittest(__name__)
