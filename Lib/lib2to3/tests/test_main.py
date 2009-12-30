# -*- coding: utf-8 -*-
import sys
import codecs
import logging
import StringIO
import unittest

from lib2to3 import main


class TestMain(unittest.TestCase):

    def tearDown(self):
        # Clean up logging configuration down by main.
        del logging.root.handlers[:]

    def run_2to3_capture(self, args, in_capture, out_capture, err_capture):
        save_stdin = sys.stdin
        save_stdout = sys.stdout
        save_stderr = sys.stderr
        sys.stdin = in_capture
        sys.stdout = out_capture
        sys.stderr = err_capture
        try:
            return main.main("lib2to3.fixes", args)
        finally:
            sys.stdin = save_stdin
            sys.stdout = save_stdout
            sys.stderr = save_stderr

    def test_unencodable_diff(self):
        input_stream = StringIO.StringIO(u"print 'nothing'\nprint u'über'\n")
        out = StringIO.StringIO()
        out_enc = codecs.getwriter("ascii")(out)
        err = StringIO.StringIO()
        ret = self.run_2to3_capture(["-"], input_stream, out_enc, err)
        self.assertEqual(ret, 0)
        output = out.getvalue()
        self.assertTrue("-print 'nothing'" in output)
        self.assertTrue("WARNING: couldn't encode <stdin>'s diff for "
                        "your terminal" in err.getvalue())
