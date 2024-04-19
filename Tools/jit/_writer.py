"""Utilities for writing StencilGroups out to a C header file."""

import typing

import _schema
import _stencils

_PATCH_FUNCS = {
    # aarch64-apple-darwin:
    "ARM64_RELOC_GOT_LOAD_PAGE21": "patch_aarch64_21rx",
    "ARM64_RELOC_GOT_LOAD_PAGEOFF12": "patch_aarch64_12",
    "ARM64_RELOC_PAGE21": "patch_aarch64_21r",
    "ARM64_RELOC_PAGEOFF12": "patch_aarch64_12",
    "ARM64_RELOC_UNSIGNED": "patch_64",
    # x86_64-pc-windows-msvc:
    "IMAGE_REL_AMD64_REL32": "patch_x86_64_32rx",
    # aarch64-pc-windows-msvc:
    "IMAGE_REL_ARM64_BRANCH26": "patch_aarch64_26r",
    "IMAGE_REL_ARM64_PAGEBASE_REL21": "patch_aarch64_21rx",
    "IMAGE_REL_ARM64_PAGEOFFSET_12A": "patch_aarch64_12",
    "IMAGE_REL_ARM64_PAGEOFFSET_12L": "patch_aarch64_12",
    # i686-pc-windows-msvc:
    "IMAGE_REL_I386_DIR32": "patch_32",
    "IMAGE_REL_I386_REL32": "patch_x86_64_32rx",  # XXX
    # aarch64-unknown-linux-gnu:
    "R_AARCH64_ABS64": "patch_64",
    "R_AARCH64_ADR_GOT_PAGE": "patch_aarch64_21rx",
    "R_AARCH64_CALL26": "patch_aarch64_26r",
    "R_AARCH64_JUMP26": "patch_aarch64_26r",
    "R_AARCH64_LD64_GOT_LO12_NC": "patch_aarch64_12",
    "R_AARCH64_MOVW_UABS_G0_NC": "patch_aarch64_16a",
    "R_AARCH64_MOVW_UABS_G1_NC": "patch_aarch64_16b",
    "R_AARCH64_MOVW_UABS_G2_NC": "patch_aarch64_16c",
    "R_AARCH64_MOVW_UABS_G3": "patch_aarch64_16d",
    # x86_64-unknown-linux-gnu:
    "R_X86_64_64": "patch_64",
    "R_X86_64_GOTPCREL": "patch_32r",
    "R_X86_64_GOTPCRELX": "patch_x86_64_32rx",
    "R_X86_64_PC32": "patch_32r",
    "R_X86_64_REX_GOTPCRELX": "patch_x86_64_32rx",
    # x86_64-apple-darwin:
    "X86_64_RELOC_BRANCH": "patch_32r",
    "X86_64_RELOC_GOT": "patch_x86_64_32rx",
    "X86_64_RELOC_GOT_LOAD": "patch_x86_64_32rx",
    "X86_64_RELOC_SIGNED": "patch_32r",
    "X86_64_RELOC_UNSIGNED": "patch_64",
}

_HOLE_EXPRS = {
    _stencils.HoleValue.CODE: "(uintptr_t)code",
    _stencils.HoleValue.CONTINUE: "(uintptr_t)code + sizeof(code_body)",
    _stencils.HoleValue.DATA: "(uintptr_t)data",
    _stencils.HoleValue.EXECUTOR: "(uintptr_t)executor",
    # _stencils.HoleValue.GOT: "",
    _stencils.HoleValue.OPARG: "instruction->oparg",
    _stencils.HoleValue.OPERAND: "instruction->operand",
    _stencils.HoleValue.OPERAND_HI: "(instruction->operand >> 32)",
    _stencils.HoleValue.OPERAND_LO: "(instruction->operand & UINT32_MAX)",
    _stencils.HoleValue.TARGET: "instruction->target",
    _stencils.HoleValue.JUMP_TARGET: "instruction_starts[instruction->jump_target]",
    _stencils.HoleValue.ERROR_TARGET: "instruction_starts[instruction->error_target]",
    _stencils.HoleValue.EXIT_INDEX: "instruction->exit_index",
    _stencils.HoleValue.TOP: "instruction_starts[1]",
    _stencils.HoleValue.ZERO: "",
}

def _hole_to_patch(where: str, hole: _stencils.Hole) -> str:
    func = _PATCH_FUNCS[hole.kind]
    location = f"{where} + {hole.offset:#x}"
    value = _HOLE_EXPRS[hole.value]
    if hole.symbol:
        if value:
            value += " + "
        value += f"(uintptr_t)&{hole.symbol}"
    if _stencils._signed(hole.addend):
        if value:
            value += " + "
        value += f"{_stencils._signed(hole.addend):#x}"
    return f"{func}({location}, {value});"


def _dump_footer(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    yield "typedef void (*emitter)(unsigned char *code, unsigned char *data,"
    yield "                        _PyExecutorObject *executor, const _PyUOpInstruction *instruction,"
    yield "                        uintptr_t instruction_starts[]);"
    yield ""
    yield "static const emitter emitters[MAX_UOP_ID + 1] = {"
    for opname in sorted(groups):
        if opname == "trampoline":
            continue
        yield f"    [{opname}] = emit_{opname},"
    yield "};"
    yield ""
    yield "static const size_t emitted[MAX_UOP_ID + 1][2] = {"
    for opname, group in sorted(groups.items()):
        if opname == "trampoline":
            continue
        yield f"    [{opname}] = {{{len(group.code.body)}, {len(group.data.body)}}},"
    yield "};"
    yield ""
    yield f"static const size_t emitted_trampoline_code = {len(groups['trampoline'].code.body)};"
    yield f"static const size_t emitted_trampoline_data = {len(groups['trampoline'].data.body)};"
    yield ""


def _dump_stencil(opname: str, group: _stencils.StencilGroup) -> typing.Iterator[str]:
    yield "void"
    yield f"emit_{opname}(unsigned char *code, unsigned char *data,"
    yield f"     {' ' * len(opname)} _PyExecutorObject *executor, const _PyUOpInstruction *instruction,"
    yield f"     {' ' * len(opname)} uintptr_t instruction_starts[])"
    yield "{"
    for part, stencil in [("code", group.code), ("data", group.data)]:
        for line in stencil.disassembly:
            yield f"    // {line}"
        if stencil.body:
            yield f"    const unsigned char {part}_body[{len(stencil.body)}] = {{"
            for i in range(0, len(stencil.body), 8):
                row = " ".join(f"{byte:#04x}," for byte in stencil.body[i : i + 8])
                yield f"        {row}"
            yield "    };"
    for part, stencil in [("data", group.data), ("code", group.code)]:
        if stencil.body:
            yield f"    memcpy({part}, {part}_body, sizeof({part}_body));"
        for hole in stencil.holes:
            yield f"    {_hole_to_patch(part, hole)}"
    yield "}"
    yield ""


def dump(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    """Yield a JIT compiler line-by-line as a C header file."""
    for opname, group in sorted(groups.items()):
        yield from _dump_stencil(opname, group)
    yield from _dump_footer(groups)
