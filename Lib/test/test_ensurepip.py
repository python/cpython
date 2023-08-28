import contextlib
import os
import os.path
import sys
import tempfile
import test.support
import unittest
import unittest.mock
import urllib.request
from hashlib import sha256
from io import BytesIO
from pathlib import Path
from random import randbytes, randint
from runpy import run_module

import ensurepip
import ensurepip._bundler
import ensurepip._structs
import ensurepip._wheelhouses
import ensurepip._uninstall


class TestPackages(unittest.TestCase):
    def setUp(self):
        ensurepip._structs._get_wheel_pkg_dir_from_sysconfig.cache_clear()
        ensurepip._wheelhouses.discover_ondisk_packages.cache_clear()

    def tearDown(self):
        ensurepip._structs._get_wheel_pkg_dir_from_sysconfig.cache_clear()
        ensurepip._wheelhouses.discover_ondisk_packages.cache_clear()

    def touch(self, directory, filename):
        fullname = os.path.join(directory, filename)
        open(fullname, "wb").close()

    def test_version(self):
        # Test version()
        with tempfile.TemporaryDirectory() as tmpdir:
            self.touch(tmpdir, "pip-1.2.3b1-py2.py3-none-any.whl")
            with unittest.mock.patch.object(
                    ensurepip._structs, 'get_config_var',
                    lambda name: Path(tmpdir),
            ):
                self.assertEqual(ensurepip.version(), '1.2.3b1')

    def test_get_packages_no_dir(self):
        # Test _get_packages() without a wheel package directory
        with unittest.mock.patch.object(
                ensurepip._structs, 'get_config_var',
                lambda name: None,
        ):
            # when bundled wheel packages are used, we get bundled pip version
            self.assertEqual(
                ensurepip._structs.PIP_REMOTE_DIST.project_version,
                ensurepip.version(),
            )

        # use bundled wheel packages
        self.assertTrue(
            isinstance(
                ensurepip._wheelhouses.discover_ondisk_packages()['pip'],
                ensurepip._structs.BundledDistributionPackage,
            ),
        )

    def test_get_packages_with_dir(self):
        # Test _get_packages() with a wheel package directory
        pip_filename = "pip-20.2.2-py2.py3-none-any.whl"

        with tempfile.TemporaryDirectory() as tmpdir:
            tmp_path = Path(tmpdir)
            self.touch(tmpdir, pip_filename)
            # not used, make sure that it's ignored
            self.touch(tmpdir, "wheel-0.34.2-py2.py3-none-any.whl")

            with unittest.mock.patch.object(
                    ensurepip._structs, 'get_config_var',
                    lambda name: tmp_path,
            ):
                packages = ensurepip._wheelhouses.discover_ondisk_packages()

            pip_package = packages['pip']
            assert isinstance(  # type checker hint
                pip_package,
                ensurepip._structs.ReplacementDistributionPackage,
            )

            self.assertEqual(pip_package.project_version, '20.2.2')
            self.assertEqual(
                pip_package.wheel_path.resolve(),
                (tmp_path / pip_filename).resolve(),
            )

            # wheel package is ignored
            self.assertEqual(sorted(packages.keys()), ['pip'])

    def test_returns_version(self):
        pip_url = ensurepip._structs.PIP_REMOTE_DIST.wheel_file_url
        self.assertIn(
            f'/packages/py3/p/pip/pip-{ensurepip.version()}-',
            pip_url,
        )


class EnsurepipMixin:

    def setUp(self):
        run_pip_patch = unittest.mock.patch("ensurepip._run_pip")
        self.run_pip = run_pip_patch.start()
        self.run_pip.return_value = 0
        self.addCleanup(run_pip_patch.stop)

        # Avoid side effects on the actual os module
        real_devnull = os.devnull
        os_patch = unittest.mock.patch("ensurepip.os")
        patched_os = os_patch.start()
        self.addCleanup(os_patch.stop)
        patched_os.devnull = real_devnull
        self.os_environ = patched_os.environ = os.environ.copy()

        shutil_patch = unittest.mock.patch("ensurepip.shutil")
        shutil_patch.start()
        self.addCleanup(shutil_patch.stop)


class TestBootstrap(EnsurepipMixin, unittest.TestCase):

    def test_basic_bootstrapping(self):
        ensurepip.bootstrap()

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 1)

    def test_replacement_wheel_bootstrapping(self):
        ensurepip._structs._get_wheel_pkg_dir_from_sysconfig.cache_clear()
        ensurepip._wheelhouses.discover_ondisk_packages.cache_clear()

        pip_wheel_name = (
            f'pip-{ensurepip._structs.PIP_REMOTE_DIST.project_version !s}-'
            'py3-none-any.whl'
        )

        with tempfile.TemporaryDirectory() as tmpdir:
            tmp_path = Path(tmpdir)
            tmp_wheel_path = tmp_path / pip_wheel_name
            tmp_wheel_path.touch()

            with unittest.mock.patch.object(
                    ensurepip._structs, 'get_config_var',
                    lambda name: tmp_path,
            ):
                ensurepip.bootstrap()

        ensurepip._structs._get_wheel_pkg_dir_from_sysconfig.cache_clear()
        ensurepip._wheelhouses.discover_ondisk_packages.cache_clear()

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(Path(additional_paths[-1]).name, pip_wheel_name)

    def test_bootstrapping_with_root(self):
        ensurepip.bootstrap(root="/foo/bar/")

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "--root", "/foo/bar/",
                "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_user(self):
        ensurepip.bootstrap(user=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "--user", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_upgrade(self):
        ensurepip.bootstrap(upgrade=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "--upgrade", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_1(self):
        ensurepip.bootstrap(verbosity=1)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "-v", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_2(self):
        ensurepip.bootstrap(verbosity=2)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "-vv", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_3(self):
        ensurepip.bootstrap(verbosity=3)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "-vvv", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_regular_install(self):
        ensurepip.bootstrap()
        self.assertEqual(self.os_environ["ENSUREPIP_OPTIONS"], "install")

    def test_bootstrapping_with_alt_install(self):
        ensurepip.bootstrap(altinstall=True)
        self.assertEqual(self.os_environ["ENSUREPIP_OPTIONS"], "altinstall")

    def test_bootstrapping_with_default_pip(self):
        ensurepip.bootstrap(default_pip=True)
        self.assertNotIn("ENSUREPIP_OPTIONS", self.os_environ)

    def test_altinstall_default_pip_conflict(self):
        with self.assertRaises(ValueError):
            ensurepip.bootstrap(altinstall=True, default_pip=True)
        self.assertFalse(self.run_pip.called)

    def test_pip_environment_variables_removed(self):
        # ensurepip deliberately ignores all pip environment variables
        # See http://bugs.python.org/issue19734 for details
        self.os_environ["PIP_THIS_SHOULD_GO_AWAY"] = "test fodder"
        ensurepip.bootstrap()
        self.assertNotIn("PIP_THIS_SHOULD_GO_AWAY", self.os_environ)

    def test_pip_config_file_disabled(self):
        # ensurepip deliberately ignores the pip config file
        # See http://bugs.python.org/issue20053 for details
        ensurepip.bootstrap()
        self.assertEqual(self.os_environ["PIP_CONFIG_FILE"], os.devnull)


class TestBundle(EnsurepipMixin, unittest.TestCase):
    def test_wheel_hash_mismatch(self):
        wheel_contents_stub = memoryview(randbytes(randint(256, 512)))
        sha256_hash = sha256(wheel_contents_stub).hexdigest()
        remote_dist_stub = ensurepip._structs._RemoteDistributionPackage(
            'pypi-pkg', '3.2.1',
            sha256_hash,
        )

        class MockedHTTPSOpener:
            def open(self, url, data, timeout):
                assert 'pypi-pkg' in url
                assert data is None  # HTTP GET
                # Intentionally corrupt the wheel:
                return BytesIO(wheel_contents_stub.tobytes()[:-1])

        with (
            unittest.mock.patch.object(
                urllib.request,
                '_opener',
                None,
            ),
            self.assertRaisesRegex(
                ValueError,
                r"^The payload's hash is invalid for ",
            )
        ):
            urllib.request.install_opener(MockedHTTPSOpener())
            remote_dist_stub.download_verified_wheel_contents()

    def test_bundle_cached(self):
        wheel_contents_stub = memoryview(randbytes(randint(256, 512)))
        sha256_hash = sha256(wheel_contents_stub).hexdigest()
        remote_dist_stub = ensurepip._structs._RemoteDistributionPackage(
            'pip', '1.2.3',
            sha256_hash,
        )
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp_path = Path(tmpdir)
            (
                tmp_path / remote_dist_stub.wheel_file_name
            ).write_bytes(wheel_contents_stub)
            test_cases = (
                ('no CLI args', (), []),
                (
                    'verbose',
                    ('-v',),
                    [
                        unittest.mock.call(
                            'A valid `pip-1.2.3-py3-none-any.whl` '
                            'is already present in cache. Skipping download.',
                        ),
                        unittest.mock.call('\n')
                    ]
                ),
            )
            for case_name, case_cli_args, expected_stderr_writes in test_cases:
                with self.subTest(case_name):
                    with (
                        unittest.mock.patch.object(
                            ensurepip._bundler, 'BUNDLED_WHEELS_PATH',
                            tmp_path,
                        ),
                        unittest.mock.patch.object(
                            ensurepip._bundler,
                            'REMOTE_DIST_PKGS',
                            [remote_dist_stub],
                        ),
                        unittest.mock.patch.object(
                            sys, 'argv', [sys.executable, *case_cli_args],
                        ),
                        unittest.mock.patch.object(
                            sys.stderr, 'write',
                        ) as stderr_write_mock,
                    ):
                        run_module('ensurepip.bundle', run_name='__main__')
                    self.assertEqual(
                        stderr_write_mock.call_args_list,
                        expected_stderr_writes,
                    )

    def test_bundle_download(self):
        wheel_contents_stub = memoryview(randbytes(randint(256, 512)))
        sha256_hash = sha256(wheel_contents_stub).hexdigest()
        remote_dist_stub = ensurepip._structs._RemoteDistributionPackage(
            'pip', '1.2.3',
            sha256_hash,
        )

        class MockedHTTPSOpener:
            def open(self, url, data, timeout):
                assert 'pip' in url
                assert data is None  # HTTP GET
                return BytesIO(wheel_contents_stub)

        test_cases = (
            ('no CLI args', (), []),
            (
                'verbose',
                ('-v',),
                [
                    unittest.mock.call(
                        'Downloading `pip-1.2.3-py3-none-any.whl`...'
                    ),
                    unittest.mock.call('\n'),
                    unittest.mock.call(
                        'Saving `pip-1.2.3-py3-none-any.whl` to disk...',
                    ),
                    unittest.mock.call('\n')
                ]
            ),
        )
        for case_name, case_cli_args, expected_stderr_writes in test_cases:
            with self.subTest(case_name):
                with tempfile.TemporaryDirectory() as tmpdir:
                    tmp_path = Path(tmpdir)
                    with (
                        unittest.mock.patch.object(
                            ensurepip._bundler, 'BUNDLED_WHEELS_PATH',
                            Path(tmpdir),
                        ),
                        unittest.mock.patch.object(
                            ensurepip._bundler,
                            'REMOTE_DIST_PKGS',
                            [remote_dist_stub],
                        ),
                        unittest.mock.patch.object(
                            sys, 'argv', [sys.executable, *case_cli_args],
                        ),
                        unittest.mock.patch.object(
                            sys.stderr, 'write',
                        ) as stderr_write_mock,
                        unittest.mock.patch.object(
                            urllib.request,
                            '_opener',
                            None,
                        ),
                    ):
                        urllib.request.install_opener(MockedHTTPSOpener())
                        run_module('ensurepip.bundle', run_name='__main__')
                    self.assertEqual(
                        (
                            tmp_path / remote_dist_stub.wheel_file_name
                        ).read_bytes(),
                        wheel_contents_stub,
                    )
                self.assertEqual(
                    stderr_write_mock.call_args_list,
                    expected_stderr_writes,
                )


@contextlib.contextmanager
def fake_pip(version=ensurepip.version()):
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


    def test_uninstall(self):
        with fake_pip():
            ensurepip._uninstall_helper()

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "pip",
            ]
        )

    def test_uninstall_with_verbosity_1(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=1)

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "-v", "pip",
            ]
        )

    def test_uninstall_with_verbosity_2(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=2)

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "-vv", "pip",
            ]
        )

    def test_uninstall_with_verbosity_3(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=3)

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "-vvv",
                "pip"
            ]
        )

    def test_pip_environment_variables_removed(self):
        # ensurepip deliberately ignores all pip environment variables
        # See http://bugs.python.org/issue19734 for details
        self.os_environ["PIP_THIS_SHOULD_GO_AWAY"] = "test fodder"
        with fake_pip():
            ensurepip._uninstall_helper()
        self.assertNotIn("PIP_THIS_SHOULD_GO_AWAY", self.os_environ)

    def test_pip_config_file_disabled(self):
        # ensurepip deliberately ignores the pip config file
        # See http://bugs.python.org/issue20053 for details
        with fake_pip():
            ensurepip._uninstall_helper()
        self.assertEqual(self.os_environ["PIP_CONFIG_FILE"], os.devnull)


# Basic testing of the main functions and their argument parsing

EXPECTED_VERSION_OUTPUT = "pip " + ensurepip.version()

class TestBootstrappingMainFunction(EnsurepipMixin, unittest.TestCase):

    def test_bootstrap_version(self):
        with test.support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                ensurepip._main(["--version"])
        result = stdout.getvalue().strip()
        self.assertEqual(result, EXPECTED_VERSION_OUTPUT)
        self.assertFalse(self.run_pip.called)

    def test_basic_bootstrapping(self):
        exit_code = ensurepip._main([])

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 1)
        self.assertEqual(exit_code, 0)

    def test_bootstrapping_error_code(self):
        self.run_pip.return_value = 2
        exit_code = ensurepip._main([])
        self.assertEqual(exit_code, 2)


class TestUninstallationMainFunction(EnsurepipMixin, unittest.TestCase):

    def test_uninstall_version(self):
        with test.support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                ensurepip._uninstall._main(["--version"])
        result = stdout.getvalue().strip()
        self.assertEqual(result, EXPECTED_VERSION_OUTPUT)
        self.assertFalse(self.run_pip.called)

    def test_basic_uninstall(self):
        with fake_pip():
            exit_code = ensurepip._uninstall._main([])

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "pip",
            ]
        )

        self.assertEqual(exit_code, 0)

    def test_uninstall_error_code(self):
        with fake_pip():
            self.run_pip.return_value = 2
            exit_code = ensurepip._uninstall._main([])
        self.assertEqual(exit_code, 2)


if __name__ == "__main__":
    unittest.main()
