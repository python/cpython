"""Tests for peephole optimization patterns in _asm_to_dasc.py.

Each test feeds a minimal sequence of DynASM lines through the peephole
optimizer and verifies the expected output.  This catches regressions in
the pattern matching and ensures newly added patterns don't break existing
ones.

Run with:
    python -m pytest Tools/jit/test_peephole.py -v
or:
    python Tools/jit/test_peephole.py
"""

from __future__ import annotations

import sys
import os
import unittest

# Ensure the JIT tools directory is importable
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))

import _asm_to_dasc


def _optimize(lines: list[str]) -> list[str]:
    """Run peephole on the given lines and return the result.

    Wraps lines in a fake emit function header/footer so the peephole
    driver recognizes stencil boundaries correctly.
    """
    wrapped = ["static void emit_TEST(Dst_DECL) {\n"] + lines + ["}\n"]
    result = _asm_to_dasc._peephole_optimize(wrapped)
    # Strip the wrapper
    return result[1:-1]


class TestPattern6_IndexedMem(unittest.TestCase):
    """Pattern 6: fold indexed memory [base + REG*scale + disp]."""

    def test_single_indexed_load(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RCX, instruction->oparg);\n",
            "    | mov rax, qword [rbx + rcx*8 + 48]\n",
        ]
        result = _optimize(lines)
        # Should produce a computed offset, not emit_mov_imm
        self.assertEqual(len(result), 1)
        self.assertIn("* 8 + 48", result[0])

    def test_multiple_indexed_accesses(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RCX, instruction->oparg);\n",
            "    | mov rax, qword [r14 + rcx*8 + 0]\n",
            "    | mov rdx, qword [r14 + rcx*8 + 8]\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 2)
        self.assertIn("* 8 + 0", result[0])
        self.assertIn("* 8 + 8", result[1])


class TestPattern8_AluImmFold(unittest.TestCase):
    """Pattern 8: ALU OP, REG → fold immediate into instruction."""

    def test_cmp_fold_32bit(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RAX, instruction->operand0);\n",
            "    | cmp ecx, eax\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 1)
        self.assertIn("cmp ecx, (int)", result[0])

    def test_cmp_fold_64bit_with_guard(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RAX, instruction->operand0);\n",
            "    | cmp rcx, rax\n",
        ]
        result = _optimize(lines)
        # Should emit a single emit_cmp_reg_imm call
        self.assertEqual(len(result), 1)
        self.assertIn("emit_cmp_reg_imm", result[0])

    def test_cmp_fold_64bit_mem_operand(self):
        """Simple qword memory compares use the dedicated helper."""
        lines = [
            "    emit_mov_imm(Dst, JREG_RCX, (uintptr_t)&PyLong_Type);\n",
            "    | cmp qword [rax + 8], rcx\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 1)
        self.assertIn("emit_cmp_mem64_imm", result[0])
        self.assertIn("JREG_RAX", result[0])
        self.assertIn(", 8,", result[0])

    def test_cmp_mem_operand_preserves_address_dependency(self):
        """Do not fold when the compared register is also the memory base."""
        lines = [
            "    emit_mov_imm(Dst, JREG_RAX, instruction->operand0);\n",
            "    | cmp qword [rax + 8], rax\n",
        ]
        result = _optimize(lines)
        self.assertEqual(result, lines)

    def test_or_memory_operand(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RDX, instruction->operand0);\n",
            "    | or dword [rbx + 16], edx\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 1)
        self.assertIn("or dword [rbx + 16], (int)", result[0])


class TestPattern12_StoreImm(unittest.TestCase):
    """Pattern 12: store register to memory → store immediate directly."""

    def test_store_byte(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RAX, instruction->oparg);\n",
            "    | mov byte [rbx + 42], al\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 1)
        self.assertIn("(char)", result[0])

    def test_store_dword(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RCX, instruction->oparg);\n",
            "    | mov dword [r14 + 8], ecx\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 1)
        self.assertIn("(int)", result[0])

    def test_store_qword_with_guard(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RCX, instruction->operand0);\n",
            "    | mov qword [r14 + 8], rcx\n",
        ]
        result = _optimize(lines)
        found_helper = any("emit_store_mem64_imm" in r for r in result)
        self.assertTrue(
            found_helper,
            "Expected emit_store_mem64_imm helper call for qword store",
        )

    def test_store_keeps_address_dependency(self):
        """Do not fold stores when the loaded register is part of the address."""
        lines = [
            "    emit_mov_imm(Dst, JREG_RCX, instruction->operand0);\n",
            "    | mov qword [rcx + 8], rcx\n",
        ]
        result = _optimize(lines)
        self.assertEqual(result, lines)


class TestPattern15_TwoMovAdd(unittest.TestCase):
    """Pattern 15: combine two immediate loads followed by add."""

    def test_load_small_int_style_fold(self):
        lines = [
            "    emit_mov_imm(Dst, JREG_RAX, ((uint16_t)((uintptr_t)(instruction->oparg))) << 5);\n",
            "    emit_mov_imm(Dst, JREG_RDI, ((uintptr_t)&_PyRuntime + 14280));\n",
            "    | add rdi, rax\n",
        ]
        result = _optimize(lines)
        self.assertEqual(len(result), 1)
        self.assertIn("JREG_RDI", result[0])
        self.assertIn("&_PyRuntime + 14280", result[0])
        self.assertIn("<< 5", result[0])


class TestPattern19_InverseMovRestore(unittest.TestCase):
    def test_drop_redundant_inverse_restore(self):
        lines = [
            "    | mov rbx, rdi\n",
            "    | mov rdi, rbx\n",
        ]
        result = _optimize(lines)
        self.assertEqual(result, ["    | mov rbx, rdi\n"])


class TestPattern13_DeadNullCheck(unittest.TestCase):
    """Pattern 13: Remove dead NULL check after PyStackRef tag creation.

    The movzx at [rax + 6] already dereferences rax, proving non-NULL.
    The subsequent cmp rdi, 1 / je =>L(N) is therefore dead code.
    """

    def test_dead_null_check_removed(self):
        lines = [
            "    | movzx edi, word [rax + 6]\n",
            "    | and edi, 1\n",
            "    | or rdi, rax\n",
            "    | cmp rdi, 1\n",
            "    | je =>L(3)\n",
        ]
        result = _optimize(lines)
        # cmp + je should be removed, leaving 3 lines
        self.assertEqual(len(result), 3)
        self.assertIn("movzx edi, word [rax + 6]", result[0])
        self.assertIn("and edi, 1", result[1])
        self.assertIn("or rdi, rax", result[2])

    def test_no_false_positive_different_reg(self):
        """Don't remove check when movzx uses a different register."""
        lines = [
            "    | movzx edi, word [rcx + 6]\n",
            "    | and edi, 1\n",
            "    | or rdi, rax\n",
            "    | cmp rdi, 1\n",
            "    | je =>L(3)\n",
        ]
        result = _optimize(lines)
        # Should NOT remove: movzx dereferences rcx, not rax
        self.assertEqual(len(result), 5)


class TestSP0_PreserveFlagsMovImm(unittest.TestCase):
    """SP0: rewrite flag-sensitive immediate loads to preserve flags."""

    def test_preserve_flags_before_setcc(self):
        lines = [
            "    | ucomisd xmm1, xmm0\n",
            "    emit_mov_imm(Dst, JREG_RDI, (uintptr_t)(instruction->oparg));\n",
            "    | setae cl\n",
        ]
        result = _optimize(lines)
        self.assertIn("emit_mov_imm_preserve_flags", result[1])

    def test_preserve_flags_before_cmov(self):
        lines = [
            "    | test cx, ax\n",
            "    emit_mov_imm(Dst, JREG_RAX, (uintptr_t)&_Py_FalseStruct);\n",
            "    emit_mov_imm(Dst, JREG_RDI, (uintptr_t)&_Py_TrueStruct);\n",
            "    | cmove rdi, rax\n",
        ]
        result = _optimize(lines)
        self.assertIn("emit_mov_imm_preserve_flags", result[1])
        self.assertIn("emit_mov_imm_preserve_flags", result[2])


class TestSP1_StoreReloadElim(unittest.TestCase):
    """SP1: Eliminate redundant stackpointer reload on hot path."""

    def test_store_reload_elimination(self):
        lines = [
            "    | mov qword [r13 + 64], r14\n",
            "    | test eax, eax\n",
            "    | je =>L(5)\n",
            "    |=>L(6):\n",
            "    | mov r14, qword [r13 + 64]\n",
        ]
        result = _optimize(lines)
        # The reload line should be eliminated
        reload_lines = [r for r in result if "mov r14, qword [r13 + 64]" in r]
        self.assertEqual(len(reload_lines), 0, "Reload should be eliminated")
        # store + test + jcc (label removed by dead label elimination
        # since L(6) has no jump references after reload removal)
        self.assertEqual(len(result), 3)


class TestSP3_InvertedStoreReload(unittest.TestCase):
    """SP3: Deferred stackpointer store when hot branch is inverted."""

    def test_inverted_store_reload(self):
        """Hot path jumps to merge, cold path falls through."""
        lines = [
            "    | mov qword [r13 + 64], r14\n",
            "    | test dil, 1\n",
            "    | jne =>L(2)\n",
            "    | jmp =>L(7)\n",
            "    |=>L(1):\n",
            "    | add rsp, 8\n",
            "    |=>L(2):\n",
            "    | mov r14, qword [r13 + 64]\n",
            "    | xor edi, edi\n",
        ]
        result = _optimize(lines)
        # Hot path: test + jne L(2) → L(2): → xor (no store/reload)
        reload_before_merge = False
        for i, r in enumerate(result):
            if "mov r14, qword [r13 + 64]" in r:
                # Reload should appear BEFORE the merge label
                for j in range(i + 1, len(result)):
                    if "=>L(2):" in result[j]:
                        reload_before_merge = True
                        break
        self.assertTrue(
            reload_before_merge, "Reload should be moved before merge label"
        )
        # Store should appear after jne (deferred to cold path)
        store_idx = None
        jne_idx = None
        for i, r in enumerate(result):
            if "jne" in r:
                jne_idx = i
            if "mov qword [r13 + 64], r14" in r:
                store_idx = i
        self.assertIsNotNone(store_idx)
        self.assertIsNotNone(jne_idx)
        self.assertGreater(
            store_idx, jne_idx, "Store should be deferred past the jne"
        )


class TestStatistics(unittest.TestCase):
    """Test the peephole statistics tracking."""

    def test_stats_increment(self):
        _asm_to_dasc.reset_peephole_stats()
        lines = [
            "    emit_mov_imm(Dst, JREG_RAX, instruction->oparg);\n",
            "    | cmp ecx, eax\n",
        ]
        _optimize(lines)
        stats = _asm_to_dasc.get_peephole_stats()
        self.assertGreater(stats["P8_alu_imm_fold"], 0)

    def test_stats_reset(self):
        _asm_to_dasc.reset_peephole_stats()
        stats = _asm_to_dasc.get_peephole_stats()
        self.assertEqual(sum(stats.values()), 0)

    def test_new_stats_registered(self):
        stats = _asm_to_dasc.get_peephole_stats()
        for key in (
            "P16_two_mov_add",
            "SP0_preserve_flags_mov_imm",
            "LLVM_fold_marker",
        ):
            self.assertIn(key, stats)


class TestLineClassification(unittest.TestCase):
    """Tests for the structured line classification (parse_line) infrastructure.

    Verifies that parse_line() correctly classifies DynASM output lines into
    typed Line objects (Asm, CCall, Label, Section, FuncDef, Blank, CCode)
    with structured operands (Reg, Mem, Imm).
    """

    def test_asm_mov(self):
        line = _asm_to_dasc.parse_line("    | mov rax, rbx")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "mov")
        self.assertIsInstance(line.dst, _asm_to_dasc.Reg)
        self.assertEqual(line.dst.name, "rax")
        self.assertIsInstance(line.src, _asm_to_dasc.Reg)
        self.assertEqual(line.src.name, "rbx")

    def test_asm_mov_memory(self):
        line = _asm_to_dasc.parse_line("    | mov qword [r13 + 64], r14")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "mov")
        self.assertIsInstance(line.dst, _asm_to_dasc.Mem)
        self.assertEqual(line.dst.size, "qword")
        self.assertEqual(line.dst.base, "r13")
        self.assertEqual(line.dst.offset, 64)
        self.assertIsInstance(line.src, _asm_to_dasc.Reg)
        self.assertEqual(line.src.name, "r14")

    def test_asm_jmp(self):
        line = _asm_to_dasc.parse_line("    | jmp =>L(3)")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "jmp")
        self.assertEqual(line.target, "=>L(3)")
        # jmp is an unconditional jump, not a conditional branch
        self.assertTrue(_asm_to_dasc.is_jump(line))
        self.assertFalse(_asm_to_dasc.is_branch(line))
        self.assertEqual(_asm_to_dasc.branch_target(line), "=>L(3)")

    def test_asm_jcc(self):
        line = _asm_to_dasc.parse_line("    | jne =>L(1)")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "jne")
        self.assertTrue(_asm_to_dasc.is_branch(line))
        self.assertFalse(_asm_to_dasc.is_jump(line))
        self.assertEqual(_asm_to_dasc.branch_target(line), "=>L(1)")

    def test_asm_call(self):
        line = _asm_to_dasc.parse_line("    | call rax")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "call")
        self.assertTrue(_asm_to_dasc.is_call(line))

    def test_asm_no_operands(self):
        line = _asm_to_dasc.parse_line("    | ret")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "ret")
        self.assertIsNone(line.dst)
        self.assertIsNone(line.src)

    def test_c_call_mov_imm(self):
        line = _asm_to_dasc.parse_line(
            "    emit_mov_imm(Dst, JREG_R8, (uintptr_t)&PyFloat_Type);"
        )
        self.assertIsInstance(line, _asm_to_dasc.CCall)
        self.assertEqual(line.kind, _asm_to_dasc.CCallKind.MOV_IMM)
        self.assertIn("JREG_R8", line.args)

    def test_c_call_mov_imm_preserve_flags(self):
        line = _asm_to_dasc.parse_line(
            "    emit_mov_imm_preserve_flags(Dst, JREG_R8, 0);"
        )
        self.assertIsInstance(line, _asm_to_dasc.CCall)
        self.assertEqual(line.kind, _asm_to_dasc.CCallKind.MOV_IMM)
        self.assertIn("JREG_R8", line.args)

    def test_c_call_ext(self):
        line = _asm_to_dasc.parse_line(
            "    emit_call_ext(Dst, (void *)&PyFloat_FromDouble);"
        )
        self.assertIsInstance(line, _asm_to_dasc.CCall)
        self.assertEqual(line.kind, _asm_to_dasc.CCallKind.CALL_EXT)
        self.assertTrue(_asm_to_dasc.is_call(line))

    def test_c_call_cmp(self):
        line = _asm_to_dasc.parse_line(
            "    emit_cmp_reg_imm(Dst, JREG_R8, (uintptr_t)&PyFloat_Type);"
        )
        self.assertIsInstance(line, _asm_to_dasc.CCall)
        self.assertEqual(line.kind, _asm_to_dasc.CCallKind.CMP_REG_IMM)

    def test_c_call_cmp_mem64(self):
        line = _asm_to_dasc.parse_line(
            "    emit_cmp_mem64_imm(Dst, JREG_RAX, 8, JREG_RCX, (uintptr_t)&PyFloat_Type);"
        )
        self.assertIsInstance(line, _asm_to_dasc.CCall)
        self.assertEqual(line.kind, _asm_to_dasc.CCallKind.CMP_MEM64_IMM)

    def test_label(self):
        line = _asm_to_dasc.parse_line("    |=>L(3):")
        self.assertIsInstance(line, _asm_to_dasc.Label)
        self.assertEqual(line.name, "L(3)")

    def test_label_uop(self):
        line = _asm_to_dasc.parse_line("    |=>uop_label:")
        self.assertIsInstance(line, _asm_to_dasc.Label)
        self.assertEqual(line.name, "uop_label")

    def test_section_code(self):
        line = _asm_to_dasc.parse_line("    |.code")
        self.assertIsInstance(line, _asm_to_dasc.Section)
        self.assertEqual(line.name, "code")

    def test_section_cold(self):
        line = _asm_to_dasc.parse_line("    |.cold")
        self.assertIsInstance(line, _asm_to_dasc.Section)
        self.assertEqual(line.name, "cold")

    def test_func_def(self):
        line = _asm_to_dasc.parse_line(
            "static void emit__BINARY_OP_ADD_FLOAT("
        )
        self.assertIsInstance(line, _asm_to_dasc.FuncDef)

    def test_c_code(self):
        line = _asm_to_dasc.parse_line("    if (!elide_prologue) {")
        self.assertIsInstance(line, _asm_to_dasc.CCode)

    def test_c_code_brace(self):
        line = _asm_to_dasc.parse_line("    }")
        self.assertIsInstance(line, _asm_to_dasc.CCode)

    def test_blank(self):
        line = _asm_to_dasc.parse_line("")
        self.assertIsInstance(line, _asm_to_dasc.Blank)

    def test_comment(self):
        line = _asm_to_dasc.parse_line("    // cold path")
        self.assertIsInstance(line, _asm_to_dasc.Blank)

    def test_store_sp(self):
        line = _asm_to_dasc.parse_line("    | mov qword [r13 + 64], r14")
        self.assertTrue(_asm_to_dasc.is_store_sp(line))

    def test_reload_sp(self):
        line = _asm_to_dasc.parse_line("    | mov r14, qword [r13 + 64]")
        self.assertTrue(_asm_to_dasc.is_reload_sp(line))

    def test_uses_reg(self):
        line = _asm_to_dasc.parse_line("    | mov rax, rbx")
        self.assertTrue(_asm_to_dasc.uses_reg(line, 0))  # rax
        self.assertTrue(_asm_to_dasc.uses_reg(line, 3))  # rbx
        self.assertFalse(_asm_to_dasc.uses_reg(line, 1))  # rcx

    def test_parse_lines_batch(self):
        lines = [
            "    | mov rax, rbx",
            "    emit_mov_imm(Dst, JREG_R8, 42);",
            "    |=>L(1):",
            "    |.cold",
        ]
        parsed = _asm_to_dasc.parse_lines(lines)
        self.assertEqual(len(parsed), 4)
        self.assertIsInstance(parsed[0], _asm_to_dasc.Asm)
        self.assertIsInstance(parsed[1], _asm_to_dasc.CCall)
        self.assertIsInstance(parsed[2], _asm_to_dasc.Label)
        self.assertIsInstance(parsed[3], _asm_to_dasc.Section)

    def test_split_operands_memory(self):
        parts = _asm_to_dasc._split_operands("qword [r13 + 64], r14")
        self.assertEqual(len(parts), 2)
        self.assertEqual(parts[0].strip(), "qword [r13 + 64]")
        self.assertEqual(parts[1].strip(), "r14")

    def test_split_operands_single(self):
        parts = _asm_to_dasc._split_operands("rax")
        self.assertEqual(len(parts), 1)
        self.assertEqual(parts[0].strip(), "rax")

    def test_split_operands_complex(self):
        parts = _asm_to_dasc._split_operands("qword [rax + rbx*8 + 16], rcx")
        self.assertEqual(len(parts), 2)
        self.assertIn("[rax + rbx*8 + 16]", parts[0])

    def test_c_call_alu_reg_imm(self):
        """ALU reg-imm helpers (test/and/or/xor) are detected as CCall."""
        for op in ("test", "and", "or", "xor"):
            line = _asm_to_dasc.parse_line(
                f"    emit_{op}_reg_imm(Dst, JREG_RAX, JREG_R8, val);"
            )
            self.assertIsInstance(line, _asm_to_dasc.CCall)
            self.assertEqual(line.kind, _asm_to_dasc.CCallKind.ALU_REG_IMM)

    def test_indexed_mem_operand(self):
        """Indexed memory operand [base + idx*scale + disp] is parsed."""
        line = _asm_to_dasc.parse_line(
            "    | mov rax, qword [rbx + rcx*8 + 48]"
        )
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertIsInstance(line.src, _asm_to_dasc.Mem)
        self.assertEqual(line.src.base, "rbx")
        self.assertEqual(line.src.index, "rcx")
        self.assertEqual(line.src.scale, 8)
        self.assertEqual(line.src.offset, 48)

    def test_lea_scale_only(self):
        """LEA with scale-only addressing [idx*scale+0] is parsed."""
        line = _asm_to_dasc.parse_line("    | lea rax, [rcx*8+0]")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "lea")
        self.assertIsInstance(line.src, _asm_to_dasc.Mem)
        self.assertIsNone(line.src.base)
        self.assertEqual(line.src.index, "rcx")
        self.assertEqual(line.src.scale, 8)
        self.assertEqual(line.src.offset, 0)

    def test_lea_base_scale(self):
        """LEA with base+index*scale [rax + rcx*8] is parsed."""
        line = _asm_to_dasc.parse_line("    | lea rax, [rax + rcx*8]")
        self.assertIsInstance(line, _asm_to_dasc.Asm)
        self.assertEqual(line.mnemonic, "lea")
        self.assertIsInstance(line.src, _asm_to_dasc.Mem)
        self.assertEqual(line.src.base, "rax")
        self.assertEqual(line.src.index, "rcx")
        self.assertEqual(line.src.scale, 8)


class TestCanonicalFrameStripping(unittest.TestCase):
    def test_strip_standard_entry_frame(self):
        assembly = """
_JIT_ENTRY:
    push rbp
    sub rsp, 32
    mov qword ptr [rsp + 8], rdi
    add rsp, 32
    pop rbp
    jmp _JIT_CONTINUE
"""
        stencil = _asm_to_dasc.convert_stencil("TEST", assembly)
        self.assertEqual(stencil.frame_size, 32)
        self.assertEqual(
            stencil.lines,
            [
                "    |=>uop_label:",
                "    | mov qword [rsp + 8], rdi",
                "    | jmp =>continue_label",
            ],
        )

    def test_strip_entry_frame_with_rbp_setup(self):
        assembly = """
_JIT_ENTRY:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    mov qword ptr [rsp + 32], rdx
    add rsp, 48
    pop rbp
    jmp _JIT_CONTINUE
"""
        stencil = _asm_to_dasc.convert_stencil("TEST", assembly)
        self.assertEqual(stencil.frame_size, 48)
        self.assertEqual(
            stencil.lines,
            [
                "    |=>uop_label:",
                "    | mov qword [rsp + 32], rdx",
                "    | jmp =>continue_label",
            ],
        )

    def test_keep_non_entry_rbp_save(self):
        assembly = """
_JIT_ENTRY:
    cmp edi, 0
    jne .Lcold
    jmp _JIT_CONTINUE
.Lcold:
    push rbp
    sub rsp, 16
    mov rbp, rsi
    mov qword ptr [rsp + 8], rdx
    add rsp, 16
    pop rbp
    jmp _JIT_CONTINUE
"""
        stencil = _asm_to_dasc.convert_stencil("TEST", assembly)
        self.assertEqual(stencil.frame_size, 0)
        self.assertIn("    | push rbp", stencil.lines)
        self.assertIn("    | add rsp, 16", stencil.lines)

    def test_frame_anchor_stripping_keeps_cache_move(self):
        assembly = """
_JIT_ENTRY:
    push rbp
    mov rbp, rsp
    sub rsp, 144
    mov rbp, rsi
    mov rbx, rdi
    nop # @@JIT_FRAME_ANCHOR@@
    mov qword ptr [r14], rbx
    jmp _JIT_CONTINUE
"""
        stencil = _asm_to_dasc.convert_stencil("TEST", assembly)
        self.assertEqual(stencil.frame_size, 144)
        self.assertIn("    | mov rbx, rdi", stencil.lines)
        self.assertIn("    | mov qword [r14], rbx", stencil.lines)

    def test_frame_anchor_stripping_removes_marker(self):
        assembly = """
_JIT_ENTRY:
    push rbp
    mov rbp, rsp
    sub rsp, 144
    mov qword ptr [rsp + 8], rdx
    nop # @@JIT_FRAME_ANCHOR@@
    mov qword ptr [r14], rbx
    jmp _JIT_CONTINUE
"""
        stencil = _asm_to_dasc.convert_stencil("TEST", assembly)
        self.assertEqual(stencil.frame_size, 144)
        self.assertNotIn("JIT_FRAME_ANCHOR", "\n".join(stencil.lines))


class TestDeadFrameAnchorElimination(unittest.TestCase):
    def test_remove_dead_frame_anchor_lea(self):
        result = _optimize(
            [
                "    | lea rax, [rbp - 144]\n",
                "    | mov rax, rdi\n",
            ]
        )
        self.assertEqual(result, ["    | mov rax, rdi\n"])

    def test_keep_live_frame_anchor_lea(self):
        result = _optimize(
            [
                "    | lea rax, [rbp - 144]\n",
                "    | add rdi, rax\n",
            ]
        )
        self.assertEqual(
            result,
            [
                "    | lea rax, [rbp - 144]\n",
                "    | add rdi, rax\n",
            ],
        )


if __name__ == "__main__":
    unittest.main()
