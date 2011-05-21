"""Tests for packaging.resources."""

import os
import sys
import shutil
import tempfile
from textwrap import dedent
from packaging.config import get_resources_dests
from packaging.database import disable_cache, enable_cache
from packaging.resources import get_file, get_file_path

from packaging.tests import unittest
from packaging.tests.test_util import GlobTestCaseBase


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
            ('mailman/foo/', os.path.join('**', 'bar', '*.cfg'), '{config}/baz'),
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
    return unittest.makeSuite(DataFilesTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
