"""Utilities for writing StencilGroups out to a C header file."""
import typing

import _schema
import _stencils


def _dump_header() -> typing.Iterator[str]:
    yield "typedef enum {"
    for kind in typing.get_args(_schema.HoleKind):
        yield f"    HoleKind_{kind},"
    yield "} HoleKind;"
    yield ""
    yield "typedef enum {"
    for value in _stencils.HoleValue:
        yield f"    HoleValue_{value.name},"
    yield "} HoleValue;"
    yield ""
    yield "typedef struct {"
    yield "    const size_t offset;"
    yield "    const HoleKind kind;"
    yield "    const HoleValue value;"
    yield "    const void *symbol;"
    yield "    const uint64_t addend;"
    yield "} Hole;"
    yield ""
    yield "typedef struct {"
    yield "    const size_t body_size;"
    yield "    const unsigned char * const body;"
    yield "    const size_t holes_size;"
    yield "    const Hole * const holes;"
    yield "} Stencil;"
    yield ""
    yield "typedef struct {"
    yield "    const Stencil code;"
    yield "    const Stencil data;"
    yield "} StencilGroup;"
    yield ""


def _dump_footer(opnames: typing.Iterable[str]) -> typing.Iterator[str]:
    yield "#define INIT_STENCIL(STENCIL) {                         \\"
    yield "    .body_size = Py_ARRAY_LENGTH(STENCIL##_body) - 1,   \\"
    yield "    .body = STENCIL##_body,                             \\"
    yield "    .holes_size = Py_ARRAY_LENGTH(STENCIL##_holes) - 1, \\"
    yield "    .holes = STENCIL##_holes,                           \\"
    yield "}"
    yield ""
    yield "#define INIT_STENCIL_GROUP(OP) {     \\"
    yield "    .code = INIT_STENCIL(OP##_code), \\"
    yield "    .data = INIT_STENCIL(OP##_data), \\"
    yield "}"
    yield ""
    yield "static const StencilGroup stencil_groups[512] = {"
    for opname in opnames:
        yield f"    [{opname}] = INIT_STENCIL_GROUP({opname}),"
    yield "};"
    yield ""
    yield "#define GET_PATCHES() { \\"
    for value in _stencils.HoleValue:
        yield f"    [HoleValue_{value.name}] = (uintptr_t)0xBADBADBADBADBADB, \\"
    yield "}"


def _dump_stencil(opname: str, group: _stencils.StencilGroup) -> typing.Iterator[str]:
    yield f"// {opname}"
    for part, stencil in [("code", group.code), ("data", group.data)]:
        for line in stencil.disassembly:
            yield f"// {line}"
        if stencil.body:
            size = len(stencil.body) + 1
            yield f"static const unsigned char {opname}_{part}_body[{size}] = {{"
            for i in range(0, len(stencil.body), 8):
                row = " ".join(f"{byte:#04x}," for byte in stencil.body[i : i + 8])
                yield f"    {row}"
            yield "};"
        else:
            yield f"static const unsigned char {opname}_{part}_body[1];"
        if stencil.holes:
            size = len(stencil.holes) + 1
            yield f"static const Hole {opname}_{part}_holes[{size}] = {{"
            for hole in stencil.holes:
                yield f"    {hole.as_c()},"
            yield "};"
        else:
            yield f"static const Hole {opname}_{part}_holes[1];"
    yield ""


def dump(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    """Yield a JIT compiler line-by-line as a C header file."""
    yield from _dump_header()
    for opname, group in groups.items():
        yield from _dump_stencil(opname, group)
    yield from _dump_footer(groups)
