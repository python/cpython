import os.path
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.common.files import (
            iter_files, _walk_tree, glob_tree,
            )


def fixpath(filename):
    return filename.replace('/', os.path.sep)


class IterFilesTests(unittest.TestCase):

    maxDiff = None

    _return_walk = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def set_files(self, *filesperroot):
        roots = []
        result = []
        for root, files in filesperroot:
            root = fixpath(root)
            roots.append(root)
            result.append([os.path.join(root, fixpath(f))
                           for f in files])
        self._return_walk = result
        return roots

    def _walk(self, root, *, suffix=None, walk=None):
        self.calls.append(('_walk', (root, suffix, walk)))
        return iter(self._return_walk.pop(0))

    def _glob(self, root, *, suffix=None):
        self.calls.append(('_glob', (root, suffix)))
        return iter(self._return_walk.pop(0))

    def test_typical(self):
        dirnames = self.set_files(
            ('spam', ['file1.c', 'file2.c']),
            ('eggs', ['ham/file3.h']),
            )
        suffixes = ('.c', '.h')

        files = list(iter_files(dirnames, suffixes,
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file2.c'),
            fixpath('eggs/ham/file3.h'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', None, _walk_tree)),
            ('_walk', ('eggs', None, _walk_tree)),
            ])

    def test_single_root(self):
        self._return_walk = [
                [fixpath('spam/file1.c'), fixpath('spam/file2.c')],
                ]

        files = list(iter_files('spam', '.c',
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file2.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', '.c', _walk_tree)),
            ])

    def test_one_root(self):
        self._return_walk = [
                [fixpath('spam/file1.c'), fixpath('spam/file2.c')],
                ]

        files = list(iter_files(['spam'], '.c',
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file2.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', '.c', _walk_tree)),
            ])

    def test_multiple_roots(self):
        dirnames = self.set_files(
            ('spam', ['file1.c', 'file2.c']),
            ('eggs', ['ham/file3.c']),
            )

        files = list(iter_files(dirnames, '.c',
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file2.c'),
            fixpath('eggs/ham/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', '.c', _walk_tree)),
            ('_walk', ('eggs', '.c', _walk_tree)),
            ])

    def test_no_roots(self):
        files = list(iter_files([], '.c',
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [])
        self.assertEqual(self.calls, [])

    def test_single_suffix(self):
        self._return_walk = [
                [fixpath('spam/file1.c'),
                 fixpath('spam/eggs/file3.c'),
                 ],
                ]

        files = list(iter_files('spam', '.c',
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/eggs/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', '.c', _walk_tree)),
            ])

    def test_one_suffix(self):
        self._return_walk = [
                [fixpath('spam/file1.c'),
                 fixpath('spam/file1.h'),
                 fixpath('spam/file1.o'),
                 fixpath('spam/eggs/file3.c'),
                 ],
                ]

        files = list(iter_files('spam', ['.c'],
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/eggs/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', None, _walk_tree)),
            ])

    def test_multiple_suffixes(self):
        self._return_walk = [
                [fixpath('spam/file1.c'),
                 fixpath('spam/file1.h'),
                 fixpath('spam/file1.o'),
                 fixpath('spam/eggs/file3.c'),
                 ],
                ]

        files = list(iter_files('spam', ('.c', '.h'),
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file1.h'),
            fixpath('spam/eggs/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', None, _walk_tree)),
            ])

    def test_no_suffix(self):
        expected = [fixpath('spam/file1.c'),
                    fixpath('spam/file1.h'),
                    fixpath('spam/file1.o'),
                    fixpath('spam/eggs/file3.c'),
                    ]
        for suffix in (None, '', ()):
            with self.subTest(suffix):
                self.calls.clear()
                self._return_walk = [list(expected)]

                files = list(iter_files('spam', suffix,
                                        _glob=self._glob,
                                        _walk=self._walk))

                self.assertEqual(files, expected)
                self.assertEqual(self.calls, [
                    ('_walk', ('spam', suffix, _walk_tree)),
                    ])

    def test_relparent(self):
        dirnames = self.set_files(
            ('/x/y/z/spam', ['file1.c', 'file2.c']),
            ('/x/y/z/eggs', ['ham/file3.c']),
            )

        files = list(iter_files(dirnames, '.c', fixpath('/x/y'),
                                _glob=self._glob,
                                _walk=self._walk))

        self.assertEqual(files, [
            fixpath('z/spam/file1.c'),
            fixpath('z/spam/file2.c'),
            fixpath('z/eggs/ham/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', (fixpath('/x/y/z/spam'), '.c', _walk_tree)),
            ('_walk', (fixpath('/x/y/z/eggs'), '.c', _walk_tree)),
            ])

    def test_glob(self):
        dirnames = self.set_files(
            ('spam', ['file1.c', 'file2.c']),
            ('eggs', ['ham/file3.c']),
            )

        files = list(iter_files(dirnames, '.c',
                                get_files=glob_tree,
                                _walk=self._walk,
                                _glob=self._glob))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file2.c'),
            fixpath('eggs/ham/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_glob', ('spam', '.c')),
            ('_glob', ('eggs', '.c')),
            ])


    def test_alt_walk_func(self):
        dirnames = self.set_files(
            ('spam', ['file1.c', 'file2.c']),
            ('eggs', ['ham/file3.c']),
            )
        def get_files(root):
            return None

        files = list(iter_files(dirnames, '.c',
                                get_files=get_files,
                                _walk=self._walk,
                                _glob=self._glob))

        self.assertEqual(files, [
            fixpath('spam/file1.c'),
            fixpath('spam/file2.c'),
            fixpath('eggs/ham/file3.c'),
            ])
        self.assertEqual(self.calls, [
            ('_walk', ('spam', '.c', get_files)),
            ('_walk', ('eggs', '.c', get_files)),
            ])






#    def test_no_dirnames(self):
#        dirnames = []
#        filter_by_name = None
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [])
#        self.assertEqual(self.calls, [])
#
#    def test_no_filter(self):
#        self._return_walk = [
#                [('spam', (), ('file1', 'file2.c', 'file3.h', 'file4.o')),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                ]
#        filter_by_name = None
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [
#            fixpath('spam/file1'),
#            fixpath('spam/file2.c'),
#            fixpath('spam/file3.h'),
#            fixpath('spam/file4.o'),
#            ])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ])
#
#    def test_no_files(self):
#        self._return_walk = [
#                [('spam', (), ()),
#                 ],
#                [(fixpath('eggs/ham'), (), ()),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                fixpath('eggs/ham'),
#                ]
#        filter_by_name = None
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ('_walk', (fixpath('eggs/ham'),)),
#            ])
#
#    def test_tree(self):
#        self._return_walk = [
#                [('spam', ('sub1', 'sub2', 'sub3'), ('file1',)),
#                 (fixpath('spam/sub1'), ('sub1sub1',), ('file2', 'file3')),
#                 (fixpath('spam/sub1/sub1sub1'), (), ('file4',)),
#                 (fixpath('spam/sub2'), (), ()),
#                 (fixpath('spam/sub3'), (), ('file5',)),
#                 ],
#                [(fixpath('eggs/ham'), (), ('file6',)),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                fixpath('eggs/ham'),
#                ]
#        filter_by_name = None
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [
#            fixpath('spam/file1'),
#            fixpath('spam/sub1/file2'),
#            fixpath('spam/sub1/file3'),
#            fixpath('spam/sub1/sub1sub1/file4'),
#            fixpath('spam/sub3/file5'),
#            fixpath('eggs/ham/file6'),
#            ])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ('_walk', (fixpath('eggs/ham'),)),
#            ])
#
#    def test_filter_suffixes(self):
#        self._return_walk = [
#                [('spam', (), ('file1', 'file2.c', 'file3.h', 'file4.o')),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                ]
#        filter_by_name = ('.c', '.h')
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [
#            fixpath('spam/file2.c'),
#            fixpath('spam/file3.h'),
#            ])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ])
#
#    def test_some_filtered(self):
#        self._return_walk = [
#                [('spam', (), ('file1', 'file2', 'file3', 'file4')),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                ]
#        def filter_by_name(filename, results=[False, True, False, True]):
#            self.calls.append(('filter_by_name', (filename,)))
#            return results.pop(0)
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [
#            fixpath('spam/file2'),
#            fixpath('spam/file4'),
#            ])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ('filter_by_name', ('file1',)),
#            ('filter_by_name', ('file2',)),
#            ('filter_by_name', ('file3',)),
#            ('filter_by_name', ('file4',)),
#            ])
#
#    def test_none_filtered(self):
#        self._return_walk = [
#                [('spam', (), ('file1', 'file2', 'file3', 'file4')),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                ]
#        def filter_by_name(filename, results=[True, True, True, True]):
#            self.calls.append(('filter_by_name', (filename,)))
#            return results.pop(0)
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [
#            fixpath('spam/file1'),
#            fixpath('spam/file2'),
#            fixpath('spam/file3'),
#            fixpath('spam/file4'),
#            ])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ('filter_by_name', ('file1',)),
#            ('filter_by_name', ('file2',)),
#            ('filter_by_name', ('file3',)),
#            ('filter_by_name', ('file4',)),
#            ])
#
#    def test_all_filtered(self):
#        self._return_walk = [
#                [('spam', (), ('file1', 'file2', 'file3', 'file4')),
#                 ],
#                ]
#        dirnames = [
#                'spam',
#                ]
#        def filter_by_name(filename, results=[False, False, False, False]):
#            self.calls.append(('filter_by_name', (filename,)))
#            return results.pop(0)
#
#        files = list(iter_files(dirnames, filter_by_name,
#                                _walk=self._walk))
#
#        self.assertEqual(files, [])
#        self.assertEqual(self.calls, [
#            ('_walk', ('spam',)),
#            ('filter_by_name', ('file1',)),
#            ('filter_by_name', ('file2',)),
#            ('filter_by_name', ('file3',)),
#            ('filter_by_name', ('file4',)),
#            ])
