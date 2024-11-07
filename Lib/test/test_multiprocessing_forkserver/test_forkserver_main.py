import os
import sys
import unittest
from unittest import mock

from multiprocessing import forkserver


class TestForkserverMain(unittest.TestCase):

    def setUp(self):
        self._orig_sys_path = list(sys.path)

    def tearDown(self):
        sys.path[:] = self._orig_sys_path

    @mock.patch("multiprocessing.process.current_process")
    @mock.patch("multiprocessing.spawn.import_main_path")
    @mock.patch("multiprocessing.util._close_stdin")
    def test_preload_kwargs(
        self,
        mock_close_stdin,
        mock_import_main_path,
        mock_current_process,
    ):
        # Very much a whitebox test of the first stanza of main before
        # we start diddling with file descriptors and pipes.
        mock_close_stdin.side_effect = RuntimeError("stop test")
        self.assertNotIn(
            "colorsys",
            sys.modules.keys(),
            msg="Thie test requires a module that has not yet been imported.",
        )

        with self.assertRaisesRegex(RuntimeError, "stop test"):
            forkserver.main(None, None, ["sys", "colorsys"])
        mock_current_process.assert_not_called()
        mock_import_main_path.assert_not_called()
        self.assertIn("colorsys", sys.modules.keys())
        self.assertEqual(sys.path, self._orig_sys_path)  # unmodified

        del sys.modules["colorsys"]  # unimport
        fake_path = os.path.dirname(__file__)
        with self.assertRaisesRegex(RuntimeError, "stop test"):
            forkserver.main(None, None, ["sys", "colorsys"], sys_path=[fake_path])
        self.assertEqual(
            sys.path, [fake_path], msg="sys.path should have been overridden"
        )
        self.assertNotIn(
            "colorsys",
            sys.modules.keys(),
            msg="import of colorsys should have failed with unusual sys.path",
        )


if __name__ == "__main__":
    unittest.main()
