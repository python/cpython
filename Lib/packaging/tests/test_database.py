import os
import io
import csv
import sys
import shutil
import tempfile
from hashlib import md5
from textwrap import dedent

from packaging.tests.test_util import GlobTestCaseBase
from packaging.tests.support import requires_zlib

from packaging.config import get_resources_dests
from packaging.errors import PackagingError
from packaging.metadata import Metadata
from packaging.tests import unittest, support
from packaging.database import (
    Distribution, EggInfoDistribution, get_distribution, get_distributions,
    provides_distribution, obsoletes_distribution, get_file_users,
    enable_cache, disable_cache, distinfo_dirname, _yield_distributions,
    get_file, get_file_path)

# TODO Add a test for getting a distribution provided by another distribution
# TODO Add a test for absolute pathed RECORD items (e.g. /etc/myapp/config.ini)
# TODO Add tests from the former pep376 project (zipped site-packages, etc.)


def get_hexdigest(filename):
    with open(filename, 'rb') as file:
        checksum = md5(file.read())
    return checksum.hexdigest()


def record_pieces(path):
    path = os.path.join(*path)
    digest = get_hexdigest(path)
    size = os.path.getsize(path)
    return path, digest, size


class FakeDistsMixin:

    def setUp(self):
        super(FakeDistsMixin, self).setUp()
        self.addCleanup(enable_cache)
        disable_cache()

        # make a copy that we can write into for our fake installed
        # distributions
        tmpdir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmpdir)
        self.fake_dists_path = os.path.realpath(
            os.path.join(tmpdir, 'fake_dists'))
        fake_dists_src = os.path.abspath(
            os.path.join(os.path.dirname(__file__), 'fake_dists'))
        shutil.copytree(fake_dists_src, self.fake_dists_path)
        # XXX ugly workaround: revert copystat calls done by shutil behind our
        # back (to avoid getting a read-only copy of a read-only file).  we
        # could pass a custom copy_function to change the mode of files, but
        # shutil gives no control over the mode of directories :(
        # see http://bugs.python.org/issue1666318
        for root, dirs, files in os.walk(self.fake_dists_path):
            os.chmod(root, 0o755)
            for f in files:
                os.chmod(os.path.join(root, f), 0o644)
            for d in dirs:
                os.chmod(os.path.join(root, d), 0o755)


class CommonDistributionTests(FakeDistsMixin):
    """Mixin used to test the interface common to both Distribution classes.

    Derived classes define cls, sample_dist, dirs and records.  These
    attributes are used in test methods.  See source code for details.
    """

    def test_instantiation(self):
        # check that useful attributes are here
        name, version, distdir = self.sample_dist
        here = os.path.abspath(os.path.dirname(__file__))
        dist_path = os.path.join(here, 'fake_dists', distdir)

        dist = self.dist = self.cls(dist_path)
        self.assertEqual(dist.path, dist_path)
        self.assertEqual(dist.name, name)
        self.assertEqual(dist.metadata['Name'], name)
        self.assertIsInstance(dist.metadata, Metadata)
        self.assertEqual(dist.version, version)
        self.assertEqual(dist.metadata['Version'], version)

    @requires_zlib
    def test_repr(self):
        dist = self.cls(self.dirs[0])
        # just check that the class name is in the repr
        self.assertIn(self.cls.__name__, repr(dist))

    @requires_zlib
    def test_comparison(self):
        # tests for __eq__ and __hash__
        dist = self.cls(self.dirs[0])
        dist2 = self.cls(self.dirs[0])
        dist3 = self.cls(self.dirs[1])
        self.assertIn(dist, {dist: True})
        self.assertEqual(dist, dist)

        self.assertIsNot(dist, dist2)
        self.assertEqual(dist, dist2)
        self.assertNotEqual(dist, dist3)
        self.assertNotEqual(dist, ())

    def test_list_installed_files(self):
        for dir_ in self.dirs:
            dist = self.cls(dir_)
            for path, md5_, size in dist.list_installed_files():
                record_data = self.records[dist.path]
                self.assertIn(path, record_data)
                self.assertEqual(md5_, record_data[path][0])
                self.assertEqual(size, record_data[path][1])


class TestDistribution(CommonDistributionTests, unittest.TestCase):

    cls = Distribution
    sample_dist = 'choxie', '2.0.0.9', 'choxie-2.0.0.9.dist-info'

    def setUp(self):
        super(TestDistribution, self).setUp()
        self.dirs = [os.path.join(self.fake_dists_path, f)
                     for f in os.listdir(self.fake_dists_path)
                     if f.endswith('.dist-info')]

        self.records = {}
        for distinfo_dir in self.dirs:

            record_file = os.path.join(distinfo_dir, 'RECORD')
            with open(record_file, 'w') as file:
                record_writer = csv.writer(
                    file, delimiter=',', quoting=csv.QUOTE_NONE,
                    lineterminator='\n')

                dist_location = distinfo_dir.replace('.dist-info', '')

                for path, dirs, files in os.walk(dist_location):
                    for f in files:
                        record_writer.writerow(record_pieces((path, f)))
                for file in ('INSTALLER', 'METADATA', 'REQUESTED'):
                    record_writer.writerow(record_pieces((distinfo_dir, file)))
                record_writer.writerow([record_file])

            with open(record_file) as file:
                record_reader = csv.reader(file, lineterminator='\n')
                record_data = {}
                for row in record_reader:
                    if row == []:
                        continue
                    path, md5_, size = (row[:] +
                                        [None for i in range(len(row), 3)])
                    record_data[path] = md5_, size
            self.records[distinfo_dir] = record_data

    def test_instantiation(self):
        super(TestDistribution, self).test_instantiation()
        self.assertIsInstance(self.dist.requested, bool)

    def test_uses(self):
        # Test to determine if a distribution uses a specified file.
        # Criteria to test against
        distinfo_name = 'grammar-1.0a4'
        distinfo_dir = os.path.join(self.fake_dists_path,
                                    distinfo_name + '.dist-info')
        true_path = [self.fake_dists_path, distinfo_name,
                     'grammar', 'utils.py']
        true_path = os.path.join(*true_path)
        false_path = [self.fake_dists_path, 'towel_stuff-0.1', 'towel_stuff',
                      '__init__.py']
        false_path = os.path.join(*false_path)

        # Test if the distribution uses the file in question
        dist = Distribution(distinfo_dir)
        self.assertTrue(dist.uses(true_path), 'dist %r is supposed to use %r' %
                        (dist, true_path))
        self.assertFalse(dist.uses(false_path), 'dist %r is not supposed to '
                         'use %r' % (dist, true_path))

    def test_get_distinfo_file(self):
        # Test the retrieval of dist-info file objects.
        distinfo_name = 'choxie-2.0.0.9'
        other_distinfo_name = 'grammar-1.0a4'
        distinfo_dir = os.path.join(self.fake_dists_path,
                                    distinfo_name + '.dist-info')
        dist = Distribution(distinfo_dir)
        # Test for known good file matches
        distinfo_files = [
            # Relative paths
            'INSTALLER', 'METADATA',
            # Absolute paths
            os.path.join(distinfo_dir, 'RECORD'),
            os.path.join(distinfo_dir, 'REQUESTED'),
        ]

        for distfile in distinfo_files:
            with dist.get_distinfo_file(distfile) as value:
                self.assertIsInstance(value, io.TextIOWrapper)
                # Is it the correct file?
                self.assertEqual(value.name,
                                 os.path.join(distinfo_dir, distfile))

        # Test an absolute path that is part of another distributions dist-info
        other_distinfo_file = os.path.join(
            self.fake_dists_path, other_distinfo_name + '.dist-info',
            'REQUESTED')
        self.assertRaises(PackagingError, dist.get_distinfo_file,
                          other_distinfo_file)
        # Test for a file that should not exist
        self.assertRaises(PackagingError, dist.get_distinfo_file,
                          'MAGICFILE')

    def test_list_distinfo_files(self):
        distinfo_name = 'towel_stuff-0.1'
        distinfo_dir = os.path.join(self.fake_dists_path,
                                    distinfo_name + '.dist-info')
        dist = Distribution(distinfo_dir)
        # Test for the iteration of the raw path
        distinfo_files = [os.path.join(distinfo_dir, filename) for filename in
                          os.listdir(distinfo_dir)]
        found = dist.list_distinfo_files()
        self.assertEqual(sorted(found), sorted(distinfo_files))
        # Test for the iteration of local absolute paths
        distinfo_files = [os.path.join(sys.prefix, distinfo_dir, path) for
                          path in distinfo_files]
        found = sorted(dist.list_distinfo_files(local=True))
        if os.sep != '/':
            self.assertNotIn('/', found[0])
            self.assertIn(os.sep, found[0])
        self.assertEqual(found, sorted(distinfo_files))

    def test_get_resources_path(self):
        distinfo_name = 'babar-0.1'
        distinfo_dir = os.path.join(self.fake_dists_path,
                                    distinfo_name + '.dist-info')
        dist = Distribution(distinfo_dir)
        resource_path = dist.get_resource_path('babar.png')
        self.assertEqual(resource_path, 'babar.png')
        self.assertRaises(KeyError, dist.get_resource_path, 'notexist')


class TestEggInfoDistribution(CommonDistributionTests,
                              support.LoggingCatcher,
                              unittest.TestCase):

    cls = EggInfoDistribution
    sample_dist = 'bacon', '0.1', 'bacon-0.1.egg-info'

    def setUp(self):
        super(TestEggInfoDistribution, self).setUp()

        self.dirs = [os.path.join(self.fake_dists_path, f)
                     for f in os.listdir(self.fake_dists_path)
                     if f.endswith('.egg') or f.endswith('.egg-info')]

        self.records = {}

    @unittest.skip('not implemented yet')
    def test_list_installed_files(self):
        # EggInfoDistribution defines list_installed_files but there is no
        # test for it yet; someone with setuptools expertise needs to add a
        # file with the list of installed files for one of the egg fake dists
        # and write the support code to populate self.records (and then delete
        # this method)
        pass


class TestDatabase(support.LoggingCatcher,
                   FakeDistsMixin,
                   unittest.TestCase):

    def setUp(self):
        super(TestDatabase, self).setUp()
        sys.path.insert(0, self.fake_dists_path)
        self.addCleanup(sys.path.remove, self.fake_dists_path)

    def test_distinfo_dirname(self):
        # Given a name and a version, we expect the distinfo_dirname function
        # to return a standard distribution information directory name.

        items = [
            # (name, version, standard_dirname)
            # Test for a very simple single word name and decimal version
            # number
            ('docutils', '0.5', 'docutils-0.5.dist-info'),
            # Test for another except this time with a '-' in the name, which
            # needs to be transformed during the name lookup
            ('python-ldap', '2.5', 'python_ldap-2.5.dist-info'),
            # Test for both '-' in the name and a funky version number
            ('python-ldap', '2.5 a---5', 'python_ldap-2.5 a---5.dist-info'),
            ]

        # Loop through the items to validate the results
        for name, version, standard_dirname in items:
            dirname = distinfo_dirname(name, version)
            self.assertEqual(dirname, standard_dirname)

    @requires_zlib
    def test_get_distributions(self):
        # Lookup all distributions found in the ``sys.path``.
        # This test could potentially pick up other installed distributions
        fake_dists = [('grammar', '1.0a4'), ('choxie', '2.0.0.9'),
                      ('towel-stuff', '0.1'), ('babar', '0.1')]
        found_dists = []

        # Verify the fake dists have been found.
        dists = [dist for dist in get_distributions()]
        for dist in dists:
            self.assertIsInstance(dist, Distribution)
            if (dist.name in dict(fake_dists) and
                dist.path.startswith(self.fake_dists_path)):
                found_dists.append((dist.name, dist.version))
            else:
                # check that it doesn't find anything more than this
                self.assertFalse(dist.path.startswith(self.fake_dists_path))
            # otherwise we don't care what other distributions are found

        # Finally, test that we found all that we were looking for
        self.assertEqual(sorted(found_dists), sorted(fake_dists))

        # Now, test if the egg-info distributions are found correctly as well
        fake_dists += [('bacon', '0.1'), ('cheese', '2.0.2'),
                       ('coconuts-aster', '10.3'),
                       ('banana', '0.4'), ('strawberry', '0.6'),
                       ('truffles', '5.0'), ('nut', 'funkyversion')]
        found_dists = []

        dists = [dist for dist in get_distributions(use_egg_info=True)]
        for dist in dists:
            self.assertIsInstance(dist, (Distribution, EggInfoDistribution))
            if (dist.name in dict(fake_dists) and
                dist.path.startswith(self.fake_dists_path)):
                found_dists.append((dist.name, dist.version))
            else:
                self.assertFalse(dist.path.startswith(self.fake_dists_path))

        self.assertEqual(sorted(fake_dists), sorted(found_dists))

    @requires_zlib
    def test_get_distribution(self):
        # Test for looking up a distribution by name.
        # Test the lookup of the towel-stuff distribution
        name = 'towel-stuff'  # Note: This is different from the directory name

        # Lookup the distribution
        dist = get_distribution(name)
        self.assertIsInstance(dist, Distribution)
        self.assertEqual(dist.name, name)

        # Verify that an unknown distribution returns None
        self.assertIsNone(get_distribution('bogus'))

        # Verify partial name matching doesn't work
        self.assertIsNone(get_distribution('towel'))

        # Verify that it does not find egg-info distributions, when not
        # instructed to
        self.assertIsNone(get_distribution('bacon'))
        self.assertIsNone(get_distribution('cheese'))
        self.assertIsNone(get_distribution('strawberry'))
        self.assertIsNone(get_distribution('banana'))

        # Now check that it works well in both situations, when egg-info
        # is a file and directory respectively.
        dist = get_distribution('cheese', use_egg_info=True)
        self.assertIsInstance(dist, EggInfoDistribution)
        self.assertEqual(dist.name, 'cheese')

        dist = get_distribution('bacon', use_egg_info=True)
        self.assertIsInstance(dist, EggInfoDistribution)
        self.assertEqual(dist.name, 'bacon')

        dist = get_distribution('banana', use_egg_info=True)
        self.assertIsInstance(dist, EggInfoDistribution)
        self.assertEqual(dist.name, 'banana')

        dist = get_distribution('strawberry', use_egg_info=True)
        self.assertIsInstance(dist, EggInfoDistribution)
        self.assertEqual(dist.name, 'strawberry')

    def test_get_file_users(self):
        # Test the iteration of distributions that use a file.
        name = 'towel_stuff-0.1'
        path = os.path.join(self.fake_dists_path, name,
                            'towel_stuff', '__init__.py')
        for dist in get_file_users(path):
            self.assertIsInstance(dist, Distribution)
            self.assertEqual(dist.name, name)

    @requires_zlib
    def test_provides(self):
        # Test for looking up distributions by what they provide
        checkLists = lambda x, y: self.assertEqual(sorted(x), sorted(y))

        l = [dist.name for dist in provides_distribution('truffles')]
        checkLists(l, ['choxie', 'towel-stuff'])

        l = [dist.name for dist in provides_distribution('truffles', '1.0')]
        checkLists(l, ['choxie'])

        l = [dist.name for dist in provides_distribution('truffles', '1.0',
                                                         use_egg_info=True)]
        checkLists(l, ['choxie', 'cheese'])

        l = [dist.name for dist in provides_distribution('truffles', '1.1.2')]
        checkLists(l, ['towel-stuff'])

        l = [dist.name for dist in provides_distribution('truffles', '1.1')]
        checkLists(l, ['towel-stuff'])

        l = [dist.name for dist in provides_distribution('truffles',
                                                         '!=1.1,<=2.0')]
        checkLists(l, ['choxie'])

        l = [dist.name for dist in provides_distribution('truffles',
                                                         '!=1.1,<=2.0',
                                                          use_egg_info=True)]
        checkLists(l, ['choxie', 'bacon', 'cheese'])

        l = [dist.name for dist in provides_distribution('truffles', '>1.0')]
        checkLists(l, ['towel-stuff'])

        l = [dist.name for dist in provides_distribution('truffles', '>1.5')]
        checkLists(l, [])

        l = [dist.name for dist in provides_distribution('truffles', '>1.5',
                                                         use_egg_info=True)]
        checkLists(l, ['bacon'])

        l = [dist.name for dist in provides_distribution('truffles', '>=1.0')]
        checkLists(l, ['choxie', 'towel-stuff'])

        l = [dist.name for dist in provides_distribution('strawberry', '0.6',
                                                         use_egg_info=True)]
        checkLists(l, ['coconuts-aster'])

        l = [dist.name for dist in provides_distribution('strawberry', '>=0.5',
                                                         use_egg_info=True)]
        checkLists(l, ['coconuts-aster'])

        l = [dist.name for dist in provides_distribution('strawberry', '>0.6',
                                                         use_egg_info=True)]
        checkLists(l, [])

        l = [dist.name for dist in provides_distribution('banana', '0.4',
                                                         use_egg_info=True)]
        checkLists(l, ['coconuts-aster'])

        l = [dist.name for dist in provides_distribution('banana', '>=0.3',
                                                         use_egg_info=True)]
        checkLists(l, ['coconuts-aster'])

        l = [dist.name for dist in provides_distribution('banana', '!=0.4',
                                                         use_egg_info=True)]
        checkLists(l, [])

    @requires_zlib
    def test_obsoletes(self):
        # Test looking for distributions based on what they obsolete
        checkLists = lambda x, y: self.assertEqual(sorted(x), sorted(y))

        l = [dist.name for dist in obsoletes_distribution('truffles', '1.0')]
        checkLists(l, [])

        l = [dist.name for dist in obsoletes_distribution('truffles', '1.0',
                                                          use_egg_info=True)]
        checkLists(l, ['cheese', 'bacon'])

        l = [dist.name for dist in obsoletes_distribution('truffles', '0.8')]
        checkLists(l, ['choxie'])

        l = [dist.name for dist in obsoletes_distribution('truffles', '0.8',
                                                          use_egg_info=True)]
        checkLists(l, ['choxie', 'cheese'])

        l = [dist.name for dist in obsoletes_distribution('truffles', '0.9.6')]
        checkLists(l, ['choxie', 'towel-stuff'])

        l = [dist.name for dist in obsoletes_distribution('truffles',
                                                          '0.5.2.3')]
        checkLists(l, ['choxie', 'towel-stuff'])

        l = [dist.name for dist in obsoletes_distribution('truffles', '0.2')]
        checkLists(l, ['towel-stuff'])

    @requires_zlib
    def test_yield_distribution(self):
        # tests the internal function _yield_distributions
        checkLists = lambda x, y: self.assertEqual(sorted(x), sorted(y))

        eggs = [('bacon', '0.1'), ('banana', '0.4'), ('strawberry', '0.6'),
                ('truffles', '5.0'), ('cheese', '2.0.2'),
                ('coconuts-aster', '10.3'), ('nut', 'funkyversion')]
        dists = [('choxie', '2.0.0.9'), ('grammar', '1.0a4'),
                 ('towel-stuff', '0.1'), ('babar', '0.1')]

        checkLists([], _yield_distributions(False, False, sys.path))

        found = [(dist.name, dist.version)
                 for dist in _yield_distributions(False, True, sys.path)
                 if dist.path.startswith(self.fake_dists_path)]
        checkLists(eggs, found)

        found = [(dist.name, dist.version)
                 for dist in _yield_distributions(True, False, sys.path)
                 if dist.path.startswith(self.fake_dists_path)]
        checkLists(dists, found)

        found = [(dist.name, dist.version)
                 for dist in _yield_distributions(True, True, sys.path)
                 if dist.path.startswith(self.fake_dists_path)]
        checkLists(dists + eggs, found)


class DataFilesTestCase(GlobTestCaseBase):

    def assertRulesMatch(self, rules, spec):
        tempdir = self.build_files_tree(spec)
        expected = self.clean_tree(spec)
        result = get_resources_dests(tempdir, rules)
        self.assertEqual(expected, result)

    def clean_tree(self, spec):
        files = {}
        for path, value in spec.items():
            if value is not None:
                files[path] = value
        return files

    def test_simple_glob(self):
        rules = [('', '*.tpl', '{data}')]
        spec = {'coucou.tpl': '{data}/coucou.tpl',
                'Donotwant': None}
        self.assertRulesMatch(rules, spec)

    def test_multiple_match(self):
        rules = [('scripts', '*.bin', '{appdata}'),
                 ('scripts', '*', '{appscript}')]
        spec = {'scripts/script.bin': '{appscript}/script.bin',
                'Babarlikestrawberry': None}
        self.assertRulesMatch(rules, spec)

    def test_set_match(self):
        rules = [('scripts', '*.{bin,sh}', '{appscript}')]
        spec = {'scripts/script.bin': '{appscript}/script.bin',
                'scripts/babar.sh':  '{appscript}/babar.sh',
                'Babarlikestrawberry': None}
        self.assertRulesMatch(rules, spec)

    def test_set_match_multiple(self):
        rules = [('scripts', 'script{s,}.{bin,sh}', '{appscript}')]
        spec = {'scripts/scripts.bin': '{appscript}/scripts.bin',
                'scripts/script.sh':  '{appscript}/script.sh',
                'Babarlikestrawberry': None}
        self.assertRulesMatch(rules, spec)

    def test_set_match_exclude(self):
        rules = [('scripts', '*', '{appscript}'),
                 ('', os.path.join('**', '*.sh'), None)]
        spec = {'scripts/scripts.bin': '{appscript}/scripts.bin',
                'scripts/script.sh':  None,
                'Babarlikestrawberry': None}
        self.assertRulesMatch(rules, spec)

    def test_glob_in_base(self):
        rules = [('scrip*', '*.bin', '{appscript}')]
        spec = {'scripts/scripts.bin': '{appscript}/scripts.bin',
                'scripouille/babar.bin': '{appscript}/babar.bin',
                'scriptortu/lotus.bin': '{appscript}/lotus.bin',
                'Babarlikestrawberry': None}
        self.assertRulesMatch(rules, spec)

    def test_recursive_glob(self):
        rules = [('', os.path.join('**', '*.bin'), '{binary}')]
        spec = {'binary0.bin': '{binary}/binary0.bin',
                'scripts/binary1.bin': '{binary}/scripts/binary1.bin',
                'scripts/bin/binary2.bin': '{binary}/scripts/bin/binary2.bin',
                'you/kill/pandabear.guy': None}
        self.assertRulesMatch(rules, spec)

    def test_final_exemple_glob(self):
        rules = [
            ('mailman/database/schemas/', '*', '{appdata}/schemas'),
            ('', os.path.join('**', '*.tpl'), '{appdata}/templates'),
            ('', os.path.join('developer-docs', '**', '*.txt'), '{doc}'),
            ('', 'README', '{doc}'),
            ('mailman/etc/', '*', '{config}'),
            ('mailman/foo/', os.path.join('**', 'bar', '*.cfg'),
             '{config}/baz'),
            ('mailman/foo/', os.path.join('**', '*.cfg'), '{config}/hmm'),
            ('', 'some-new-semantic.sns', '{funky-crazy-category}'),
        ]
        spec = {
            'README': '{doc}/README',
            'some.tpl': '{appdata}/templates/some.tpl',
            'some-new-semantic.sns':
                '{funky-crazy-category}/some-new-semantic.sns',
            'mailman/database/mailman.db': None,
            'mailman/database/schemas/blah.schema':
                '{appdata}/schemas/blah.schema',
            'mailman/etc/my.cnf': '{config}/my.cnf',
            'mailman/foo/some/path/bar/my.cfg':
                '{config}/hmm/some/path/bar/my.cfg',
            'mailman/foo/some/path/other.cfg':
                '{config}/hmm/some/path/other.cfg',
            'developer-docs/index.txt': '{doc}/developer-docs/index.txt',
            'developer-docs/api/toc.txt': '{doc}/developer-docs/api/toc.txt',
        }
        self.maxDiff = None
        self.assertRulesMatch(rules, spec)

    def test_get_file(self):
        # Create a fake dist
        temp_site_packages = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, temp_site_packages)

        dist_name = 'test'
        dist_info = os.path.join(temp_site_packages, 'test-0.1.dist-info')
        os.mkdir(dist_info)

        metadata_path = os.path.join(dist_info, 'METADATA')
        resources_path = os.path.join(dist_info, 'RESOURCES')

        with open(metadata_path, 'w') as fp:
            fp.write(dedent("""\
                Metadata-Version: 1.2
                Name: test
                Version: 0.1
                Summary: test
                Author: me
                """))

        test_path = 'test.cfg'

        fd, test_resource_path = tempfile.mkstemp()
        os.close(fd)
        self.addCleanup(os.remove, test_resource_path)

        with open(test_resource_path, 'w') as fp:
            fp.write('Config')

        with open(resources_path, 'w') as fp:
            fp.write('%s,%s' % (test_path, test_resource_path))

        # Add fake site-packages to sys.path to retrieve fake dist
        self.addCleanup(sys.path.remove, temp_site_packages)
        sys.path.insert(0, temp_site_packages)

        # Force packaging.database to rescan the sys.path
        self.addCleanup(enable_cache)
        disable_cache()

        # Try to retrieve resources paths and files
        self.assertEqual(get_file_path(dist_name, test_path),
                         test_resource_path)
        self.assertRaises(KeyError, get_file_path, dist_name, 'i-dont-exist')

        with get_file(dist_name, test_path) as fp:
            self.assertEqual(fp.read(), 'Config')
        self.assertRaises(KeyError, get_file, dist_name, 'i-dont-exist')


def test_suite():
    suite = unittest.TestSuite()
    load = unittest.defaultTestLoader.loadTestsFromTestCase
    suite.addTest(load(TestDistribution))
    suite.addTest(load(TestEggInfoDistribution))
    suite.addTest(load(TestDatabase))
    suite.addTest(load(DataFilesTestCase))
    return suite


if __name__ == "__main__":
    unittest.main(defaultTest='test_suite')
