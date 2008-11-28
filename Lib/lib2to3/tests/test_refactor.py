"""
Unit tests for refactor.py.
"""

import sys
import os
import operator
import StringIO
import tempfile
import unittest

from lib2to3 import refactor, pygram, fixer_base

from . import support


FIXER_DIR = os.path.join(os.path.dirname(__file__), "data/fixers")

sys.path.append(FIXER_DIR)
try:
    _DEFAULT_FIXERS = refactor.get_fixers_from_package("myfixes")
finally:
    sys.path.pop()

class TestRefactoringTool(unittest.TestCase):

    def setUp(self):
        sys.path.append(FIXER_DIR)

    def tearDown(self):
        sys.path.pop()

    def check_instances(self, instances, classes):
        for inst, cls in zip(instances, classes):
            if not isinstance(inst, cls):
                self.fail("%s are not instances of %s" % instances, classes)

    def rt(self, options=None, fixers=_DEFAULT_FIXERS, explicit=None):
        return refactor.RefactoringTool(fixers, options, explicit)

    def test_print_function_option(self):
        gram = pygram.python_grammar
        save = gram.keywords["print"]
        try:
            rt = self.rt({"print_function" : True})
            self.assertRaises(KeyError, operator.itemgetter("print"),
                              gram.keywords)
        finally:
            gram.keywords["print"] = save

    def test_fixer_loading_helpers(self):
        contents = ["explicit", "first", "last", "parrot", "preorder"]
        non_prefixed = refactor.get_all_fix_names("myfixes")
        prefixed = refactor.get_all_fix_names("myfixes", False)
        full_names = refactor.get_fixers_from_package("myfixes")
        self.assertEqual(prefixed, ["fix_" + name for name in contents])
        self.assertEqual(non_prefixed, contents)
        self.assertEqual(full_names,
                         ["myfixes.fix_" + name for name in contents])

    def test_get_headnode_dict(self):
        class NoneFix(fixer_base.BaseFix):
            PATTERN = None

        class FileInputFix(fixer_base.BaseFix):
            PATTERN = "file_input< any * >"

        no_head = NoneFix({}, [])
        with_head = FileInputFix({}, [])
        d = refactor.get_headnode_dict([no_head, with_head])
        expected = {None: [no_head],
                    pygram.python_symbols.file_input : [with_head]}
        self.assertEqual(d, expected)

    def test_fixer_loading(self):
        from myfixes.fix_first import FixFirst
        from myfixes.fix_last import FixLast
        from myfixes.fix_parrot import FixParrot
        from myfixes.fix_preorder import FixPreorder

        rt = self.rt()
        pre, post = rt.get_fixers()

        self.check_instances(pre, [FixPreorder])
        self.check_instances(post, [FixFirst, FixParrot, FixLast])

    def test_naughty_fixers(self):
        self.assertRaises(ImportError, self.rt, fixers=["not_here"])
        self.assertRaises(refactor.FixerError, self.rt, fixers=["no_fixer_cls"])
        self.assertRaises(refactor.FixerError, self.rt, fixers=["bad_order"])

    def test_refactor_string(self):
        rt = self.rt()
        input = "def parrot(): pass\n\n"
        tree = rt.refactor_string(input, "<test>")
        self.assertNotEqual(str(tree), input)

        input = "def f(): pass\n\n"
        tree = rt.refactor_string(input, "<test>")
        self.assertEqual(str(tree), input)

    def test_refactor_stdin(self):

        class MyRT(refactor.RefactoringTool):

            def print_output(self, lines):
                diff_lines.extend(lines)

        diff_lines = []
        rt = MyRT(_DEFAULT_FIXERS)
        save = sys.stdin
        sys.stdin = StringIO.StringIO("def parrot(): pass\n\n")
        try:
            rt.refactor_stdin()
        finally:
            sys.stdin = save
        expected = """--- <stdin> (original)
+++ <stdin> (refactored)
@@ -1,2 +1,2 @@
-def parrot(): pass
+def cheese(): pass""".splitlines()
        self.assertEqual(diff_lines[:-1], expected)

    def test_refactor_file(self):
        test_file = os.path.join(FIXER_DIR, "parrot_example.py")
        old_contents = open(test_file, "r").read()
        rt = self.rt()

        rt.refactor_file(test_file)
        self.assertEqual(old_contents, open(test_file, "r").read())

        rt.refactor_file(test_file, True)
        try:
            self.assertNotEqual(old_contents, open(test_file, "r").read())
        finally:
            open(test_file, "w").write(old_contents)

    def test_refactor_docstring(self):
        rt = self.rt()

        def example():
            """
            >>> example()
            42
            """
        out = rt.refactor_docstring(example.__doc__, "<test>")
        self.assertEqual(out, example.__doc__)

        def parrot():
            """
            >>> def parrot():
            ...      return 43
            """
        out = rt.refactor_docstring(parrot.__doc__, "<test>")
        self.assertNotEqual(out, parrot.__doc__)

    def test_explicit(self):
        from myfixes.fix_explicit import FixExplicit

        rt = self.rt(fixers=["myfixes.fix_explicit"])
        self.assertEqual(len(rt.post_order), 0)

        rt = self.rt(explicit=["myfixes.fix_explicit"])
        for fix in rt.post_order:
            if isinstance(fix, FixExplicit):
                break
        else:
            self.fail("explicit fixer not loaded")
