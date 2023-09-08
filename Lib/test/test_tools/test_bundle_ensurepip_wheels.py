import sys
import tempfile
import unittest
import unittest.mock
import urllib.request
from hashlib import sha256
from io import BytesIO
from pathlib import Path
from random import randbytes
from test import support, test_tools
from test.support import captured_stderr

import ensurepip

test_tools.skip_if_missing('build')
with test_tools.imports_under_tool('build'):
    import bundle_ensurepip_wheels as bew


# Disable fancy GitHub actions output during the tests
@unittest.mock.patch.object(bew, 'GITHUB_ACTIONS', False)
class TestBundle(unittest.TestCase):
    contents = randbytes(512)
    checksum = sha256(contents).hexdigest()
    projects = [('pip', '1.2.3', checksum)]
    pip_filename = "pip-1.2.3-py3-none-any.whl"

    def test__wheel_url(self):
        self.assertEqual(
            bew._wheel_url('pip', '1.2.3'),
            'https://files.pythonhosted.org/packages/py3/p/pip/pip-1.2.3-py3-none-any.whl',
        )

    def test__get_projects(self):
        self.assertListEqual(
            bew._get_projects(),
            [('pip', ensurepip._PIP_VERSION, ensurepip._PIP_SHA_256)],
        )

    def test__is_valid_wheel(self):
        self.assertTrue(bew._is_valid_wheel(self.contents, checksum=self.checksum))

    def test_cached_wheel(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir = Path(tmpdir)
            (tmpdir / self.pip_filename).write_bytes(self.contents)
            with (
                captured_stderr() as stderr,
                unittest.mock.patch.object(bew, 'WHEEL_DIR', tmpdir),
                unittest.mock.patch.object(bew, '_get_projects', lambda: self.projects),
            ):
                exit_code = bew.download_wheels()
        self.assertEqual(exit_code, 0)
        stderr = stderr.getvalue()
        self.assertIn("A valid 'pip' wheel already exists!", stderr)

    def test_invalid_checksum(self):
        class MockedHTTPSOpener:
            @staticmethod
            def open(url, data, timeout):
                assert 'pip' in url
                assert data is None  # HTTP GET
                # Intentionally corrupt the wheel:
                return BytesIO(self.contents[:-1])

        with (
            captured_stderr() as stderr,
            unittest.mock.patch.object(urllib.request, '_opener', None),
            unittest.mock.patch.object(bew, '_get_projects', lambda: self.projects),
        ):
            urllib.request.install_opener(MockedHTTPSOpener())
            exit_code = bew.download_wheels()
        self.assertEqual(exit_code, 1)
        stderr = stderr.getvalue()
        self.assertIn("Failed to validate checksum for", stderr)

    def test_download_wheel(self):
        class MockedHTTPSOpener:
            @staticmethod
            def open(url, data, timeout):
                assert 'pip' in url
                assert data is None  # HTTP GET
                return BytesIO(self.contents)

        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir = Path(tmpdir)
            with (
                captured_stderr() as stderr,
                unittest.mock.patch.object(urllib.request, '_opener', None),
                unittest.mock.patch.object(bew, 'WHEEL_DIR', tmpdir),
                unittest.mock.patch.object(bew, '_get_projects', lambda: self.projects),
            ):
                urllib.request.install_opener(MockedHTTPSOpener())
                exit_code = bew.download_wheels()
            self.assertEqual((tmpdir / self.pip_filename).read_bytes(), self.contents)
        self.assertEqual(exit_code, 0)
        stderr = stderr.getvalue()
        self.assertIn("Downloading 'https://files.pythonhosted.org/packages/py3/p/pip/pip-1.2.3-py3-none-any.whl'",
                      stderr)
        self.assertIn("Writing 'pip-1.2.3-py3-none-any.whl' to disk", stderr)
