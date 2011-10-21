import os
import tempfile

from packaging.dist import Distribution
from packaging.tests import support, unittest


class TestingSupportTestCase(unittest.TestCase):

    def test_fake_dec(self):
        @support.fake_dec(1, 2, k=3)
        def func(arg0, *args, **kargs):
            return arg0, args, kargs
        self.assertEqual(func(-1, -2, k=-3), (-1, (-2,), {'k': -3}))

    def test_TempdirManager(self):
        files = {}

        class Tester(support.TempdirManager, unittest.TestCase):

            def test_mktempfile(self2):
                tmpfile = self2.mktempfile()
                files['test_mktempfile'] = tmpfile.name
                self.assertTrue(os.path.isfile(tmpfile.name))

            def test_mkdtemp(self2):
                tmpdir = self2.mkdtemp()
                files['test_mkdtemp'] = tmpdir
                self.assertTrue(os.path.isdir(tmpdir))

            def test_write_file(self2):
                tmpdir = self2.mkdtemp()
                files['test_write_file'] = tmpdir
                self2.write_file((tmpdir, 'file1'), 'me file 1')
                file1 = os.path.join(tmpdir, 'file1')
                self.assertTrue(os.path.isfile(file1))
                text = ''
                with open(file1, 'r') as f:
                    text = f.read()
                self.assertEqual(text, 'me file 1')

            def test_create_dist(self2):
                project_dir, dist = self2.create_dist()
                files['test_create_dist'] = project_dir
                self.assertTrue(os.path.isdir(project_dir))
                self.assertIsInstance(dist, Distribution)

            def test_assertIsFile(self2):
                fd, fn = tempfile.mkstemp()
                os.close(fd)
                self.addCleanup(support.unlink, fn)
                self2.assertIsFile(fn)
                self.assertRaises(AssertionError, self2.assertIsFile, 'foO')

            def test_assertIsNotFile(self2):
                tmpdir = self2.mkdtemp()
                self2.assertIsNotFile(tmpdir)

        tester = Tester()
        for name in ('test_mktempfile', 'test_mkdtemp', 'test_write_file',
                     'test_create_dist', 'test_assertIsFile',
                     'test_assertIsNotFile'):
            tester.setUp()
            try:
                getattr(tester, name)()
            finally:
                tester.tearDown()

            # check clean-up
            if name in files:
                self.assertFalse(os.path.exists(files[name]))


def test_suite():
    return unittest.makeSuite(TestingSupportTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
