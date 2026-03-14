from __future__ import annotations

import pathlib
import re
import tempfile
import unittest

import _optimizers


def _run_optimizer(text: str) -> str:
    with tempfile.TemporaryDirectory() as tmp:
        path = pathlib.Path(tmp) / "test.s"
        path.write_text(text)
        _optimizers.OptimizerX86ELF(
            path,
            label_prefix=".L",
            symbol_prefix="",
            re_global=re.compile(
                r"\s*\.globl\s+(?P<label>[\w.\"$?@]+)(\s+.*)?"
            ),
        ).run()
        return path.read_text()


def _strip_annotated_input(text: str) -> str:
    return "\n".join(
        line for line in text.splitlines() if not line.startswith("#")
    )


class TestHotColdLayout(unittest.TestCase):
    def test_refcount_gate_stays_hot(self):
        text = """
            .text
            .globl entry
        entry:
            test al, 1
            je .Ldecref
        .Lhot:
            jmp _JIT_CONTINUE
        .Ldecref:
            dec dword [rbx]
            je .Lcold
            jmp .Lhot
        .Lcold:
            callq helper
            jmp .Lhot
        """
        optimized = _run_optimizer(text)
        hot, _, cold = optimized.partition(
            '.section\t.text.cold,"ax",@progbits'
        )
        self.assertIn(".Ldecref:", hot)
        self.assertIn(".Lcold:", cold)

    def test_cold_join_block_can_leave_hot_section(self):
        text = """
            .text
            .globl entry
        entry:
            test al, 1
            je .Lfast
            jmp .Lcoldstart
        .Ljoin:
            test rbp, rbp
            je .Lerr
        .Lfast:
            jmp _JIT_CONTINUE
        .Lcoldstart:
            callq helper
            jmp .Ljoin
        .Lerr:
            jmp slowerror
        """
        optimized = _run_optimizer(text)
        hot, _, cold = optimized.partition(
            '.section\t.text.cold,"ax",@progbits'
        )
        hot = _strip_annotated_input(hot)
        self.assertIn(".Ljoin:", cold)
        self.assertNotIn(".Ljoin:", hot)


if __name__ == "__main__":
    unittest.main()
