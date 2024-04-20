"""Utilities for writing StencilGroups out to a C header file."""

import typing

import _stencils


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
            yield f"    {hole.as_c(part)}"
    yield "}"
    yield ""


def dump(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    """Yield a JIT compiler line-by-line as a C header file."""
    for opname, group in sorted(groups.items()):
        yield from _dump_stencil(opname, group)
    yield from _dump_footer(groups)
