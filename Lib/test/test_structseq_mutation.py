"""Regression tests for gh-155322: mutating a heap PyStructSequence type's
writable size attributes (n_fields / n_sequence_fields / n_unnamed_fields)
must not crash the interpreter when a new instance is constructed.
"""
import os
import time
import unittest

try:
    import resource
except ImportError:
    resource = None


class StructSeqMutationTests(unittest.TestCase):

    def _check_overflow_raises(self, structseq_type, sample_args):
        structseq_type(sample_args)

        original = structseq_type.n_fields
        try:
            structseq_type.n_fields = 100000
            with self.assertRaises(TypeError):
                structseq_type(sample_args)
        finally:
            structseq_type.n_fields = original

        structseq_type(sample_args)

    def test_terminal_size_n_fields_overflow(self):
        self._check_overflow_raises(os.terminal_size, (80, 24))

    def test_stat_result_n_fields_overflow(self):
        st = os.stat(".")
        self._check_overflow_raises(os.stat_result, tuple(st))

    def test_struct_time_n_fields_overflow(self):
        t = time.localtime()
        self._check_overflow_raises(time.struct_time, tuple(t))

    @unittest.skipIf(resource is None, "resource module not available")
    def test_struct_rusage_n_fields_overflow(self):
        ru = resource.getrusage(resource.RUSAGE_SELF)
        self._check_overflow_raises(resource.struct_rusage, tuple(ru))

    def test_terminal_size_n_sequence_fields_overflow(self):
        original = os.terminal_size.n_sequence_fields
        try:
            os.terminal_size.n_sequence_fields = 100000
            with self.assertRaises(TypeError):
                os.terminal_size((80, 24))
        finally:
            os.terminal_size.n_sequence_fields = original
        os.terminal_size((80, 24))

    def test_terminal_size_negative_n_fields(self):
        original = os.terminal_size.n_fields
        try:
            os.terminal_size.n_fields = -1
            with self.assertRaises(TypeError):
                os.terminal_size((80, 24))
        finally:
            os.terminal_size.n_fields = original
        os.terminal_size((80, 24))

    def test_n_sequence_fields_exceeds_n_fields(self):
        orig_seq = time.struct_time.n_sequence_fields
        orig_fields = time.struct_time.n_fields
        try:
            time.struct_time.n_fields = 2
            time.struct_time.n_sequence_fields = 3
            with self.assertRaises(TypeError):
                time.struct_time(tuple(time.localtime()))
        finally:
            time.struct_time.n_fields = orig_fields
            time.struct_time.n_sequence_fields = orig_seq
        time.struct_time(tuple(time.localtime()))

    def test_n_unnamed_fields_exceeds_n_sequence_fields(self):
        orig_seq = time.struct_time.n_sequence_fields
        orig_unnamed = time.struct_time.n_unnamed_fields
        try:
            time.struct_time.n_sequence_fields = 2
            time.struct_time.n_unnamed_fields = 5
            with self.assertRaises(TypeError):
                time.struct_time(tuple(time.localtime()))
        finally:
            time.struct_time.n_sequence_fields = orig_seq
            time.struct_time.n_unnamed_fields = orig_unnamed
        time.struct_time(tuple(time.localtime()))


if __name__ == "__main__":
    unittest.main()
