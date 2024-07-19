"""Utilities for writing StencilGroups out to a C header file."""

import itertools
import typing

import _stencils


def _dump_footer(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    yield "typedef struct {"
    yield "    void (*emit)("
    yield "        unsigned char *code, unsigned char *data, _PyExecutorObject *executor,"
    yield "        const _PyUOpInstruction *instruction, uintptr_t instruction_starts[]);"
    yield "    size_t code_size;"
    yield "    size_t data_size;"
    yield "} StencilGroup;"
    yield ""
    yield f"static const StencilGroup trampoline = {groups['trampoline'].as_c('trampoline')};"
    yield ""
    yield "static const StencilGroup stencil_groups[MAX_UOP_ID + 1] = {"
    for opname, group in sorted(groups.items()):
        if opname == "trampoline":
            continue
        yield f"    [{opname}] = {group.as_c(opname)},"
    yield "};"


def _dump_stencil(opname: str, group: _stencils.StencilGroup) -> typing.Iterator[str]:
    yield "void"
    yield f"emit_{opname}("
    yield "    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,"
    yield "    const _PyUOpInstruction *instruction, uintptr_t instruction_starts[])"
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
    # Data is written first (so relaxations in the code work properly):
    for part, stencil in [("data", group.data), ("code", group.code)]:
        if stencil.body:
            yield f"    memcpy({part}, {part}_body, sizeof({part}_body));"
        skip = False
        stencil.holes.sort(key=lambda hole: hole.offset)
        for hole, pair in itertools.zip_longest(stencil.holes, stencil.holes[1:]):
            if skip:
                skip = False
                continue
            if pair and (folded := hole.fold(pair)):
                skip = True
                hole = folded
            yield f"    {hole.as_c(part)}"
    yield "}"
    yield ""


def dump(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    """Yield a JIT compiler line-by-line as a C header file."""
    for opname, group in sorted(groups.items()):
        yield from _dump_stencil(opname, group)
    yield from _dump_footer(groups)
