"""Tests for packaging.create."""
import io
import os
import sys
import sysconfig
from textwrap import dedent
from packaging.create import MainProgram, ask_yn, ask, main

from packaging.tests import support, unittest


class CreateTestCase(support.TempdirManager,
                     support.EnvironRestorer,
                     unittest.TestCase):

    restore_environ = ['PLAT']

    def setUp(self):
        super(CreateTestCase, self).setUp()
        self._stdin = sys.stdin  # TODO use Inputs
        self._stdout = sys.stdout
        sys.stdin = io.StringIO()
        sys.stdout = io.StringIO()
        self._cwd = os.getcwd()
        self.wdir = self.mkdtemp()
        os.chdir(self.wdir)
        # patch sysconfig
        self._old_get_paths = sysconfig.get_paths
        sysconfig.get_paths = lambda *args, **kwargs: {
            'man': sys.prefix + '/share/man',
            'doc': sys.prefix + '/share/doc/pyxfoil', }

    def tearDown(self):
        sys.stdin = self._stdin
        sys.stdout = self._stdout
        os.chdir(self._cwd)
        sysconfig.get_paths = self._old_get_paths
        super(CreateTestCase, self).tearDown()

    def test_ask_yn(self):
        sys.stdin.write('y\n')
        sys.stdin.seek(0)
        self.assertEqual('y', ask_yn('is this a test'))

    def test_ask(self):
        sys.stdin.write('a\n')
        sys.stdin.write('b\n')
        sys.stdin.seek(0)
        self.assertEqual('a', ask('is this a test'))
        self.assertEqual('b', ask(str(list(range(0, 70))), default='c',
                                  lengthy=True))

    def test_set_multi(self):
        mainprogram = MainProgram()
        sys.stdin.write('aaaaa\n')
        sys.stdin.seek(0)
        mainprogram.data['author'] = []
        mainprogram._set_multi('_set_multi test', 'author')
        self.assertEqual(['aaaaa'], mainprogram.data['author'])

    def test_find_files(self):
        # making sure we scan a project dir correctly
        mainprogram = MainProgram()

        # building the structure
        tempdir = self.wdir
        dirs = ['pkg1', 'data', 'pkg2', 'pkg2/sub']
        files = ['README', 'setup.cfg', 'foo.py',
                 'pkg1/__init__.py', 'pkg1/bar.py',
                 'data/data1', 'pkg2/__init__.py',
                 'pkg2/sub/__init__.py']

        for dir_ in dirs:
            os.mkdir(os.path.join(tempdir, dir_))

        for file_ in files:
            path = os.path.join(tempdir, file_)
            self.write_file(path, 'xxx')

        mainprogram._find_files()
        mainprogram.data['packages'].sort()

        # do we have what we want?
        self.assertEqual(mainprogram.data['packages'],
                         ['pkg1', 'pkg2', 'pkg2.sub'])
        self.assertEqual(mainprogram.data['modules'], ['foo'])
        data_fn = os.path.join('data', 'data1')
        self.assertEqual(set(mainprogram.data['extra_files']),
                         set(['setup.cfg', 'README', data_fn]))

    def test_convert_setup_py_to_cfg(self):
        self.write_file((self.wdir, 'setup.py'),
                        dedent("""
        # coding: utf-8
        from distutils.core import setup

        long_description = '''My super Death-scription
        barbar is now on the public domain,
        ho, baby !'''

        setup(name='pyxfoil',
              version='0.2',
              description='Python bindings for the Xfoil engine',
              long_description=long_description,
              maintainer='André Espaze',
              maintainer_email='andre.espaze@logilab.fr',
              url='http://www.python-science.org/project/pyxfoil',
              license='GPLv2',
              packages=['pyxfoil', 'babar', 'me'],
              data_files=[
                  ('share/doc/pyxfoil', ['README.rst']),
                  ('share/man', ['pyxfoil.1']),
                         ],
              py_modules=['my_lib', 'mymodule'],
              package_dir={
                  'babar': '',
                  'me': 'Martinique/Lamentin',
                          },
              package_data={
                  'babar': ['Pom', 'Flora', 'Alexander'],
                  'me': ['dady', 'mumy', 'sys', 'bro'],
                  '':  ['setup.py', 'README'],
                  'pyxfoil': ['fengine.so'],
                           },
              scripts=['my_script', 'bin/run'],
              )
        """), encoding='utf-8')
        sys.stdin.write('y\n')
        sys.stdin.seek(0)
        main()

        with open(os.path.join(self.wdir, 'setup.cfg'), encoding='utf-8') as fp:
            lines = set(line.rstrip() for line in fp)

        # FIXME don't use sets
        self.assertEqual(lines, set(['',
            '[metadata]',
            'version = 0.2',
            'name = pyxfoil',
            'maintainer = André Espaze',
            'description = My super Death-scription',
            '       |barbar is now on the public domain,',
            '       |ho, baby !',
            'maintainer_email = andre.espaze@logilab.fr',
            'home_page = http://www.python-science.org/project/pyxfoil',
            'download_url = UNKNOWN',
            'summary = Python bindings for the Xfoil engine',
            '[files]',
            'modules = my_lib',
            '    mymodule',
            'packages = pyxfoil',
            '    babar',
            '    me',
            'extra_files = Martinique/Lamentin/dady',
            '    Martinique/Lamentin/mumy',
            '    Martinique/Lamentin/sys',
            '    Martinique/Lamentin/bro',
            '    Pom',
            '    Flora',
            '    Alexander',
            '    setup.py',
            '    README',
            '    pyxfoil/fengine.so',
            'scripts = my_script',
            '    bin/run',
            'resources =',
            '    README.rst = {doc}',
            '    pyxfoil.1 = {man}',
        ]))

    def test_convert_setup_py_to_cfg_with_description_in_readme(self):
        self.write_file((self.wdir, 'setup.py'),
                        dedent("""
        # coding: utf-8
        from distutils.core import setup
        with open('README.txt') as fp:
            long_description = fp.read()

        setup(name='pyxfoil',
              version='0.2',
              description='Python bindings for the Xfoil engine',
              long_description=long_description,
              maintainer='André Espaze',
              maintainer_email='andre.espaze@logilab.fr',
              url='http://www.python-science.org/project/pyxfoil',
              license='GPLv2',
              packages=['pyxfoil'],
              package_data={'pyxfoil': ['fengine.so', 'babar.so']},
              data_files=[
                ('share/doc/pyxfoil', ['README.rst']),
                ('share/man', ['pyxfoil.1']),
              ],
        )
        """), encoding='utf-8')
        self.write_file((self.wdir, 'README.txt'),
                        dedent('''
My super Death-scription
barbar is now in the public domain,
ho, baby!
                        '''))
        sys.stdin.write('y\n')
        sys.stdin.seek(0)
        # FIXME Out of memory error.
        main()
        with open(os.path.join(self.wdir, 'setup.cfg'), encoding='utf-8') as fp:
            lines = set(line.rstrip() for line in fp)

        self.assertEqual(lines, set(['',
            '[metadata]',
            'version = 0.2',
            'name = pyxfoil',
            'maintainer = André Espaze',
            'maintainer_email = andre.espaze@logilab.fr',
            'home_page = http://www.python-science.org/project/pyxfoil',
            'download_url = UNKNOWN',
            'summary = Python bindings for the Xfoil engine',
            'description-file = README.txt',
            '[files]',
            'packages = pyxfoil',
            'extra_files = pyxfoil/fengine.so',
            '    pyxfoil/babar.so',
            'resources =',
            '    README.rst = {doc}',
            '    pyxfoil.1 = {man}',
        ]))


def test_suite():
    return unittest.makeSuite(CreateTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
