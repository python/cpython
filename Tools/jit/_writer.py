"""Utilities for writing StencilGroups out to a C header file."""

import typing

import _schema
import _stencils

_PATCH_REMAP = {
    # aarch64-apple-darwin:
    "ARM64_RELOC_GOT_LOAD_PAGE21": "aarch64_21x",
    "ARM64_RELOC_GOT_LOAD_PAGEOFF12": "aarch64_12",
    "ARM64_RELOC_PAGE21": "aarch64_21",
    "ARM64_RELOC_PAGEOFF12": "aarch64_12",
    "ARM64_RELOC_UNSIGNED": "64",
    # x86_64-pc-windows-msvc:
    "IMAGE_REL_AMD64_REL32": "x86_64_32x",
    # aarch64-pc-windows-msvc:
    "IMAGE_REL_ARM64_BRANCH26": "aarch64_26",
    "IMAGE_REL_ARM64_PAGEBASE_REL21": "aarch64_21x",
    "IMAGE_REL_ARM64_PAGEOFFSET_12A": "aarch64_12",
    "IMAGE_REL_ARM64_PAGEOFFSET_12L": "aarch64_12",
    # i686-pc-windows-msvc:
    "IMAGE_REL_I386_DIR32": "32",
    "IMAGE_REL_I386_REL32": "x86_64_32x",  # XXX
    # aarch64-unknown-linux-gnu:
    "R_AARCH64_ABS64": "64",
    "R_AARCH64_ADR_GOT_PAGE": "aarch64_21x",
    "R_AARCH64_CALL26": "aarch64_26",
    "R_AARCH64_JUMP26": "aarch64_26",
    "R_AARCH64_LD64_GOT_LO12_NC": "aarch64_12",
    "R_AARCH64_MOVW_UABS_G0_NC": "aarch64_16a",
    "R_AARCH64_MOVW_UABS_G1_NC": "aarch64_16b",
    "R_AARCH64_MOVW_UABS_G2_NC": "aarch64_16c",
    "R_AARCH64_MOVW_UABS_G3": "aarch64_16d",
    # x86_64-unknown-linux-gnu:
    "R_X86_64_64": "64",
    "R_X86_64_GOTPCREL": "32r",
    "R_X86_64_GOTPCRELX": "x86_64_32x",
    "R_X86_64_PC32": "32r",
    "R_X86_64_REX_GOTPCRELX": "x86_64_32x",
    # x86_64-apple-darwin:
    "X86_64_RELOC_BRANCH": "32r",
    "X86_64_RELOC_GOT": "x86_64_32x",
    "X86_64_RELOC_GOT_LOAD": "x86_64_32x",
    "X86_64_RELOC_SIGNED": "32r",
    "X86_64_RELOC_UNSIGNED": "64",
}

def _dump_header() -> typing.Iterator[str]:
    yield "typedef void (*emitter)(uintptr_t patches[]);"
    # yield "typedef enum {"
    # for kind in typing.get_args(_schema.HoleKind):
    #     yield f"    HoleKind_{kind},"
    # yield "} HoleKind;"
    # yield ""
    yield "typedef enum {"
    for value in _stencils.HoleValue:
        yield f"    HoleValue_{value.name},"
    yield "} HoleValue;"
    yield ""
    # yield "typedef struct {"
    # yield "    const size_t offset;"
    # yield "    const HoleKind kind;"
    # yield "    const HoleValue value;"
    # yield "    const void *symbol;"
    # yield "    const uint64_t addend;"
    # yield "} Hole;"
    # yield ""
    # yield "typedef struct {"
    # yield "    const size_t body_size;"
    # yield "    const unsigned char * const body;"
    # yield "    const size_t holes_size;"
    # yield "    const Hole * const holes;"
    # yield "} Stencil;"
    # yield ""
    # yield "typedef struct {"
    # yield "    const Stencil code;"
    # yield "    const Stencil data;"
    # yield "} StencilGroup;"
    # yield ""


def _dump_footer(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    # yield "#define INIT_STENCIL(STENCIL) {                         \\"
    # yield "    .body_size = Py_ARRAY_LENGTH(STENCIL##_body) - 1,   \\"
    # yield "    .body = STENCIL##_body,                             \\"
    # yield "    .holes_size = Py_ARRAY_LENGTH(STENCIL##_holes) - 1, \\"
    # yield "    .holes = STENCIL##_holes,                           \\"
    # yield "}"
    # yield ""
    # yield "#define INIT_STENCIL_GROUP(OP) {     \\"
    # yield "    .code = INIT_STENCIL(OP##_code), \\"
    # yield "    .data = INIT_STENCIL(OP##_data), \\"
    # yield "}"
    # yield ""
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
    yield "#define GET_PATCHES() { \\"
    for value in _stencils.HoleValue:
        yield f"    [HoleValue_{value.name}] = (uintptr_t)0xBADBADBADBADBADB, \\"
    yield "}"


def _dump_stencil(opname: str, group: _stencils.StencilGroup) -> typing.Iterator[str]:
    yield f"void emit_{opname}(uintptr_t patches[]) {{"
    yield f"    unsigned char *location;"
    for part, stencil in [("data", group.data), ("code", group.code)]:
        for line in stencil.disassembly:
            yield f"    // {line}"
        if stencil.body:
            yield f"    const unsigned char {part}[{len(stencil.body)}] = {{"
            for i in range(0, len(stencil.body), 8):
                row = " ".join(f"{byte:#04x}," for byte in stencil.body[i : i + 8])
                yield f"        {row}"
            yield "    };"
            yield f"    location = (unsigned char *)patches[HoleValue_{part.upper()}];"
            yield f"    memcpy(location, {part}, sizeof({part}));"
        for hole in stencil.holes:
            yield f"    patch_{_PATCH_REMAP[hole.kind]}(location + {hole.offset}, patches[HoleValue_{hole.value.name}]{f' + (uintptr_t)&{hole.symbol}' if hole.symbol else ''}{f' + {_stencils._signed(hole.addend):#x}' if _stencils._signed(hole.addend) else ''});"
    yield "}"
    yield ""


def dump(groups: dict[str, _stencils.StencilGroup]) -> typing.Iterator[str]:
    """Yield a JIT compiler line-by-line as a C header file."""
    yield from _dump_header()
    for opname, group in groups.items():
        yield from _dump_stencil(opname, group)
    yield from _dump_footer(groups)
