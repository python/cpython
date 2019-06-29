import os.path
import unittest

from test.test_c_statics.cg.files import iter_files


def fixpath(filename):
    return filename.replace('/', os.path.sep)


class IterFilesTests(unittest.TestCase):

    _return_walk = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def _walk(self, dirname):
        self.calls.append(('_walk', (dirname,)))
        return iter(self._return_walk.pop(0))

    def test_basic(self):
        self._return_walk = [
                [('spam', (), ('file1', 'file2')),
                 ],
                [(fixpath('eggs/ham'), (), ('file3',)),
                 ],
                ]
        dirnames = [
                'spam',
                fixpath('eggs/ham'),
                ]
        filter_by_name = None

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1'),
            fixpath('spam/file2'),
            fixpath('eggs/ham/file3'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ('_walk', (fixpath('eggs/ham'),)),
            ])

    def test_no_dirnames(self):
        dirnames = []
        filter_by_name = None

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [])
        self.assertEqual(self.calls, [])

    def test_no_filter(self):
        self._return_walk = [
                [('spam', (), ('file1', 'file2.c', 'file3.h', 'file4.o')),
                 ],
                ]
        dirnames = [
                'spam',
                ]
        filter_by_name = None

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1'),
            fixpath('spam/file2.c'),
            fixpath('spam/file3.h'),
            fixpath('spam/file4.o'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ])

    def test_no_files(self):
        self._return_walk = [
                [('spam', (), ()),
                 ],
                [(fixpath('eggs/ham'), (), ()),
                 ],
                ]
        dirnames = [
                'spam',
                fixpath('eggs/ham'),
                ]
        filter_by_name = None

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ('_walk', (fixpath('eggs/ham'),)),
            ])

    def test_tree(self):
        self._return_walk = [
                [('spam', ('sub1', 'sub2', 'sub3'), ('file1',)),
                 (fixpath('spam/sub1'), ('sub1sub1',), ('file2', 'file3')),
                 (fixpath('spam/sub1/sub1sub1'), (), ('file4',)),
                 (fixpath('spam/sub2'), (), ()),
                 (fixpath('spam/sub3'), (), ('file5',)),
                 ],
                [(fixpath('eggs/ham'), (), ('file6',)),
                 ],
                ]
        dirnames = [
                'spam',
                fixpath('eggs/ham'),
                ]
        filter_by_name = None

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1'),
            fixpath('spam/sub1/file2'),
            fixpath('spam/sub1/file3'),
            fixpath('spam/sub1/sub1sub1/file4'),
            fixpath('spam/sub3/file5'),
            fixpath('eggs/ham/file6'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ('_walk', (fixpath('eggs/ham'),)),
            ])

    def test_filter_suffixes(self):
        self._return_walk = [
                [('spam', (), ('file1', 'file2.c', 'file3.h', 'file4.o')),
                 ],
                ]
        dirnames = [
                'spam',
                ]
        filter_by_name = ('.c', '.h')

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file2.c'),
            fixpath('spam/file3.h'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ])

    def test_some_filtered(self):
        self._return_walk = [
                [('spam', (), ('file1', 'file2', 'file3', 'file4')),
                 ],
                ]
        dirnames = [
                'spam',
                ]
        def filter_by_name(filename, results=[False, True, False, True]):
            self.calls.append(('filter_by_name', (filename,)))
            return results.pop(0)

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file2'),
            fixpath('spam/file4'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ('filter_by_name', ('file1',)),
            ('filter_by_name', ('file2',)),
            ('filter_by_name', ('file3',)),
            ('filter_by_name', ('file4',)),
            ])

    def test_none_filtered(self):
        self._return_walk = [
                [('spam', (), ('file1', 'file2', 'file3', 'file4')),
                 ],
                ]
        dirnames = [
                'spam',
                ]
        def filter_by_name(filename, results=[True, True, True, True]):
            self.calls.append(('filter_by_name', (filename,)))
            return results.pop(0)

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1'),
            fixpath('spam/file2'),
            fixpath('spam/file3'),
            fixpath('spam/file4'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ('filter_by_name', ('file1',)),
            ('filter_by_name', ('file2',)),
            ('filter_by_name', ('file3',)),
            ('filter_by_name', ('file4',)),
            ])

    def test_all_filtered(self):
        self._return_walk = [
                [('spam', (), ('file1', 'file2', 'file3', 'file4')),
                 ],
                ]
        dirnames = [
                'spam',
                ]
        def filter_by_name(filename, results=[False, False, False, False]):
            self.calls.append(('filter_by_name', (filename,)))
            return results.pop(0)

        files = list(iter_files(dirnames, filter_by_name,
                                _walk=self._walk))

        self.assertEqual(files, [])
        self.assertEqual(self.calls, [
            ('_walk', ('spam',)),
            ('filter_by_name', ('file1',)),
            ('filter_by_name', ('file2',)),
            ('filter_by_name', ('file3',)),
            ('filter_by_name', ('file4',)),
            ])
