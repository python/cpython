import unittest
import unittest.mock
import ensurepip
import test.support


class TestEnsurePipVersion(unittest.TestCase):

    def test_returns_version(self):
        self.assertEqual(ensurepip._PIP_VERSION, ensurepip.version())


class TestBootstrap(unittest.TestCase):

    def setUp(self):
        run_pip_patch = unittest.mock.patch("ensurepip._run_pip")
        self.run_pip = run_pip_patch.start()
        self.addCleanup(run_pip_patch.stop)

        os_environ_patch = unittest.mock.patch("ensurepip.os.environ", {})
        self.os_environ = os_environ_patch.start()
        self.addCleanup(os_environ_patch.stop)

    def test_basic_bootstrapping(self):
        ensurepip.bootstrap()

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 2)

    def test_bootstrapping_with_root(self):
        ensurepip.bootstrap(root="/foo/bar/")

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "--root", "/foo/bar/",
                "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_user(self):
        ensurepip.bootstrap(user=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "--user", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_upgrade(self):
        ensurepip.bootstrap(upgrade=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "--upgrade", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_1(self):
        ensurepip.bootstrap(verbosity=1)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "-v", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_2(self):
        ensurepip.bootstrap(verbosity=2)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "-vv", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_3(self):
        ensurepip.bootstrap(verbosity=3)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-index", "--find-links",
                unittest.mock.ANY, "--pre", "-vvv", "setuptools", "pip",
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


if __name__ == "__main__":
    test.support.run_unittest(__name__)
