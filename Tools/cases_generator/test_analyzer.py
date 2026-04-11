"""Tests for analyzer.py — specifically the add_macro() recording-uop placement rules.

Run with:
    cd Tools/cases_generator
    python -m pytest test_analyzer.py -v
or:
    python test_analyzer.py
"""

import sys
import os
import unittest
from typing import Any

# The cases_generator directory is not on sys.path when invoked from the repo
# root, so add it explicitly.
sys.path.insert(0, os.path.dirname(__file__))

import parsing
from analyzer import analyze_forest


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _parse(src: str) -> list[parsing.AstNode]:
    """Parse a raw DSL string (no BEGIN/END markers needed) into an AST forest."""
    psr = parsing.Parser(src, filename="<test>")
    nodes: list[parsing.AstNode] = []
    while node := psr.definition():
        nodes.append(node)  # type: ignore[arg-type]
    return nodes


def _analyze(src: str) -> Any:
    """Parse *src* and run analyze_forest(); return the Analysis object."""
    return analyze_forest(_parse(src))


# ---------------------------------------------------------------------------
# Shared DSL fragments
# ---------------------------------------------------------------------------

# A minimal specializing op (tier == 1 because of the "specializing" annotation).
_SPECIALIZE_OP = """\
specializing op(_SPECIALIZE_DUMMY, (counter/1, value -- value)) {
}
"""

# A minimal recording op: uses RECORD_VALUE → records_value == True.
_RECORD_OP = """\
op(_RECORD_DUMMY, (value -- value)) {
    RECORD_VALUE(PyStackRef_AsPyObjectBorrow(value));
}
"""

# A plain (non-specializing, non-recording) worker op.
_WORKER_OP = """\
op(_WORKER_DUMMY, (value -- res)) {
    res = value;
}
"""


# ---------------------------------------------------------------------------
# Test class
# ---------------------------------------------------------------------------

class TestAnalyzer(unittest.TestCase):

    def test_recording_uop_position(self) -> None:
        """Recording uops must be first, or immediately follow a specializing uop.

        Case 1 — VALID: recording uop directly after specializing uop.
        Case 2 — VALID: recording uop after specializing uop with a cache effect
                         (unused/1) between them; cache effects are transparent.
        Case 3 — INVALID: recording uop after a plain (non-specializing) worker uop.
        """

        # ------------------------------------------------------------------
        # Case 1: _SPECIALIZE_DUMMY + _RECORD_DUMMY  (no cache between them)
        # ------------------------------------------------------------------
        src_valid_direct = (
            _SPECIALIZE_OP
            + _RECORD_OP
            + _WORKER_OP
            + "macro(VALID_DIRECT) = _SPECIALIZE_DUMMY + _RECORD_DUMMY + _WORKER_DUMMY;\n"
        )
        # Must not raise — the recording uop follows the specializing uop directly.
        try:
            _analyze(src_valid_direct)
        except SyntaxError as exc:
            self.fail(
                f"Case 1 (valid: recording after specializing) raised unexpectedly: {exc}"
            )

        # ------------------------------------------------------------------
        # Case 2: _SPECIALIZE_DUMMY + unused/1 + _RECORD_DUMMY
        #         A CacheEffect between them must be transparent.
        # ------------------------------------------------------------------
        src_valid_with_cache = (
            _SPECIALIZE_OP
            + _RECORD_OP
            + _WORKER_OP
            + "macro(VALID_CACHE) = _SPECIALIZE_DUMMY + unused/1 + _RECORD_DUMMY + _WORKER_DUMMY;\n"
        )
        try:
            _analyze(src_valid_with_cache)
        except SyntaxError as exc:
            self.fail(
                f"Case 2 (valid: recording after specializing + cache) raised unexpectedly: {exc}"
            )

        # ------------------------------------------------------------------
        # Case 3: _WORKER_DUMMY + _RECORD_DUMMY
        #         A recording uop after a non-specializing uop must be rejected.
        # ------------------------------------------------------------------
        src_invalid = (
            _SPECIALIZE_OP
            + _RECORD_OP
            + _WORKER_OP
            + "macro(INVALID) = _WORKER_DUMMY + _RECORD_DUMMY;\n"
        )
        with self.assertRaises(SyntaxError) as ctx:
            _analyze(src_invalid)

        # Confirm the error message is the one we emit, not some unrelated error.
        self.assertIn(
            "Recording uop",
            str(ctx.exception),
            msg="Case 3: SyntaxError message should mention 'Recording uop'",
        )
        self.assertIn(
            "_RECORD_DUMMY",
            str(ctx.exception),
            msg="Case 3: SyntaxError message should name the offending uop",
        )


if __name__ == "__main__":
    unittest.main()
