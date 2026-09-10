"""Generate trampoline_ehframe.h from a compiled perf trampoline object.

Copies the assembler-generated .eh_frame of the trampoline into a C header,
one block per architecture (a fat Mach-O file yields several).  The FDE's
initial_location and address_range are left zeroed for Python/jit_unwind.c
to patch at runtime.
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
from dataclasses import dataclass

# ELF constants, see <elf.h>.
_ELF_MAGIC = b"\x7fELF"
_ELF64_HEADER_SIZE = 64
_ELF64_SECTION_HEADER_SIZE = 64
_ELFCLASS64 = 2
_ELFDATA2LSB = 1
_ELFDATA2MSB = 2
_SHT_NOBITS = 8
_EM_X86_64 = 62
_EM_AARCH64 = 183

# Mach-O constants, see llvm/BinaryFormat/MachO.h.
_MH_MAGIC_64 = 0xFEEDFACF
_MH_CIGAM_64 = 0xCFFAEDFE
_MACHO64_HEADER_SIZE = 32
_MACHO64_SEGMENT_COMMAND_SIZE = 72
_MACHO64_SECTION_SIZE = 80
_FAT_MAGIC = 0xCAFEBABE
_FAT_MAGIC_64 = 0xCAFEBABF
_FAT_HEADER_SIZE = 8
_LC_SEGMENT_64 = 0x19
_CPU_ARCH_ABI64 = 0x01000000
_CPU_TYPE_X86_64 = 7 | _CPU_ARCH_ABI64
_CPU_TYPE_ARM64 = 12 | _CPU_ARCH_ABI64

# DWARF exception header pointer encodings, see
# Include/internal/pycore_jit_unwind.h.
_DW_EH_PE_absptr = 0x00
_DW_EH_PE_sdata4 = 0x0B
_DW_EH_PE_pcrel = 0x10

# Smallest CIE the parser accepts, in bytes after the length field: CIE_id,
# version, "zR\0", code and data alignment factors, return address column,
# augmentation data length and the FDE pointer encoding.
_CIE_MIN_LENGTH = 4 + 1 + 3 + 1 + 1 + 1 + 1 + 1

# Accepted FDE pointer encodings and the width of the initial_location and
# address_range fields they imply.  GNU and LLVM ELF assemblers emit
# pcrel|sdata4, Darwin assemblers emit pcrel|absptr.
_FDE_FIELD_SIZES = {
    _DW_EH_PE_pcrel | _DW_EH_PE_sdata4: 4,
    _DW_EH_PE_pcrel | _DW_EH_PE_absptr: 8,
}

# Compiler macro that selects each slice's block in the generated header.
_ELF_ARCH_MACROS = {
    _EM_X86_64: "__x86_64__",
    _EM_AARCH64: "__aarch64__",
}
_MACHO_ARCH_MACROS = {
    _CPU_TYPE_X86_64: "__x86_64__",
    _CPU_TYPE_ARM64: "__aarch64__",
}

# The sections the generator needs, and their Mach-O names.
_WANTED_SECTIONS = (".eh_frame", ".text")
_MACHO_SECTION_NAMES = {
    "__eh_frame": ".eh_frame",
    "__text": ".text",
}


@dataclass
class ObjectSlice:
    """One architecture's worth of an object file."""

    source: str
    arch_macro: str
    endian: str
    sections: dict[str, bytes]


@dataclass
class EhFrame:
    """Parsed .eh_frame with the FDE fields zeroed for runtime patching."""

    data: bytes
    fde_pc_offset: int
    fde_range_offset: int
    field_size: int


def _elf_slice(data: bytes, source: str) -> ObjectSlice:
    """Parse an ELF64 relocatable object."""
    if len(data) < _ELF64_HEADER_SIZE:
        raise ValueError(f"{source}: truncated ELF header")
    if data[4] != _ELFCLASS64:
        raise ValueError(f"{source}: not an ELF64 object (class={data[4]})")
    if data[5] == _ELFDATA2LSB:
        endian = "<"
    elif data[5] == _ELFDATA2MSB:
        endian = ">"
    else:
        raise ValueError(f"{source}: unknown ELF byte order ({data[5]})")

    e_machine: int = struct.unpack_from(f"{endian}H", data, 18)[0]
    try:
        arch_macro = _ELF_ARCH_MACROS[e_machine]
    except KeyError:
        raise ValueError(
            f"{source}: unsupported ELF machine type {e_machine}"
        ) from None

    e_shoff: int = struct.unpack_from(f"{endian}Q", data, 40)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(f"{endian}HHH", data, 58)
    if e_shoff == 0 or e_shnum == 0:
        raise ValueError(f"{source}: no section headers")
    if e_shentsize < _ELF64_SECTION_HEADER_SIZE:
        raise ValueError(f"{source}: bad section header size {e_shentsize}")
    if e_shoff + e_shnum * e_shentsize > len(data):
        raise ValueError(f"{source}: section headers extend beyond the end of the file")
    if e_shstrndx >= e_shnum:
        raise ValueError(f"{source}: bad section name string table index")

    def section_header(index: int) -> tuple[int, int, int, int]:
        base = e_shoff + index * e_shentsize
        sh_name, sh_type = struct.unpack_from(f"{endian}II", data, base)
        sh_offset, sh_size = struct.unpack_from(f"{endian}QQ", data, base + 24)
        return sh_name, sh_type, sh_offset, sh_size

    def section_bytes(name: str, sh_type: int, sh_offset: int, sh_size: int) -> bytes:
        if sh_type == _SHT_NOBITS:
            raise ValueError(f"{source}: section {name} has no contents in the file")
        if sh_offset + sh_size > len(data):
            raise ValueError(
                f"{source}: section {name} extends past the end of the file"
            )
        return data[sh_offset : sh_offset + sh_size]

    _, shstr_type, shstr_offset, shstr_size = section_header(e_shstrndx)
    shstrtab = section_bytes(".shstrtab", shstr_type, shstr_offset, shstr_size)

    sections: dict[str, bytes] = {}
    for index in range(e_shnum):
        sh_name, sh_type, sh_offset, sh_size = section_header(index)
        end = shstrtab.find(b"\x00", sh_name)
        if end < 0:
            continue
        name = shstrtab[sh_name:end].decode("ascii", errors="replace")
        if name in _WANTED_SECTIONS:
            if name in sections:
                raise ValueError(f"{source}: more than one {name} section")
            sections[name] = section_bytes(name, sh_type, sh_offset, sh_size)

    return ObjectSlice(source, arch_macro, endian, sections)


def _macho_slice(data: bytes, source: str) -> ObjectSlice:
    """Parse a thin Mach-O 64-bit object."""
    if len(data) < _MACHO64_HEADER_SIZE:
        raise ValueError(f"{source}: truncated Mach-O header")
    magic: int = struct.unpack_from("<I", data, 0)[0]
    if magic == _MH_MAGIC_64:
        endian = "<"
    elif magic == _MH_CIGAM_64:
        endian = ">"
    else:
        raise ValueError(f"{source}: not a 64-bit Mach-O object")

    cputype: int = struct.unpack_from(f"{endian}I", data, 4)[0]
    try:
        arch_macro = _MACHO_ARCH_MACROS[cputype]
    except KeyError:
        raise ValueError(
            f"{source}: unsupported Mach-O CPU type {cputype:#x}"
        ) from None

    # mach_header_64: magic, cputype, cpusubtype, filetype, ncmds,
    # sizeofcmds, flags, reserved (8 x uint32, 32 bytes).
    ncmds, sizeofcmds = struct.unpack_from(f"{endian}II", data, 16)
    commands_end = _MACHO64_HEADER_SIZE + sizeofcmds
    if commands_end > len(data):
        raise ValueError(f"{source}: load commands extend beyond the end of the file")

    sections: dict[str, bytes] = {}
    offset = _MACHO64_HEADER_SIZE
    for _ in range(ncmds):
        if offset + 8 > commands_end:
            raise ValueError(f"{source}: truncated load commands")
        cmd, cmdsize = struct.unpack_from(f"{endian}II", data, offset)
        if cmdsize < 8 or offset + cmdsize > commands_end:
            raise ValueError(f"{source}: bad load command size {cmdsize}")
        if cmd == _LC_SEGMENT_64:
            # segment_command_64: cmd, cmdsize, segname[16], vmaddr, vmsize,
            # fileoff, filesize, maxprot, initprot, nsects, flags (72 bytes),
            # followed by nsects section_64 entries.
            if cmdsize < _MACHO64_SEGMENT_COMMAND_SIZE:
                raise ValueError(f"{source}: truncated segment command")
            nsects: int = struct.unpack_from(f"{endian}I", data, offset + 64)[0]
            if _MACHO64_SEGMENT_COMMAND_SIZE + nsects * _MACHO64_SECTION_SIZE > cmdsize:
                raise ValueError(
                    f"{source}: section table extends beyond its segment command"
                )
            sect = offset + _MACHO64_SEGMENT_COMMAND_SIZE
            for _ in range(nsects):
                # section_64: sectname[16], segname[16], addr, size, offset,
                # align, reloff, nreloc, flags, reserved1-3 (80 bytes).
                raw_name = data[sect : sect + 16].split(b"\x00", 1)[0]
                segname = data[sect + 16 : sect + 32].split(b"\x00", 1)[0]
                name = _MACHO_SECTION_NAMES.get(
                    raw_name.decode("ascii", errors="replace")
                )
                if name is not None and segname == b"__TEXT":
                    if name in sections:
                        raise ValueError(f"{source}: more than one {name} section")
                    size: int = struct.unpack_from(f"{endian}Q", data, sect + 40)[0]
                    file_offset: int = struct.unpack_from(
                        f"{endian}I", data, sect + 48
                    )[0]
                    if file_offset + size > len(data):
                        raise ValueError(
                            f"{source}: section {name} extends past the end of the file"
                        )
                    sections[name] = data[file_offset : file_offset + size]
                sect += _MACHO64_SECTION_SIZE
        offset += cmdsize

    return ObjectSlice(source, arch_macro, endian, sections)


def _fat_slices(data: bytes, source: str) -> list[ObjectSlice]:
    """Split a universal (fat) Mach-O file into its thin slices."""
    # The fat header and its fat_arch entries are always big-endian.
    magic, nfat_arch = struct.unpack_from(">II", data, 0)
    if magic == _FAT_MAGIC:
        # fat_arch: cputype, cpusubtype, offset, size, align (20 bytes).
        entry_format, entry_size = ">IIIII", 20
    elif magic == _FAT_MAGIC_64:
        # fat_arch_64: cputype, cpusubtype, offset, size, align, reserved.
        entry_format, entry_size = ">IIQQII", 32
    else:
        raise ValueError(f"{source}: not a fat Mach-O file")
    if nfat_arch == 0:
        raise ValueError(f"{source}: fat Mach-O file with no architectures")
    if _FAT_HEADER_SIZE + nfat_arch * entry_size > len(data):
        raise ValueError(f"{source}: truncated fat header")

    slices: list[ObjectSlice] = []
    for index in range(nfat_arch):
        entry = struct.unpack_from(
            entry_format, data, _FAT_HEADER_SIZE + index * entry_size
        )
        cputype, _, offset, size = entry[:4]
        thin = data[offset : offset + size]
        if len(thin) != size:
            raise ValueError(f"{source}: fat slice {index} is truncated")
        obj_slice = _macho_slice(thin, f"{source} (slice {cputype:#x})")
        if _MACHO_ARCH_MACROS.get(cputype) != obj_slice.arch_macro:
            raise ValueError(
                f"{source}: fat slice {index} CPU type {cputype:#x} does not match "
                "its Mach-O header"
            )
        slices.append(obj_slice)
    return slices


def load_object(path: str) -> list[ObjectSlice]:
    """Return the ObjectSlices (one per architecture) of an object file."""
    with open(path, "rb") as f:
        data = f.read()
    source = os.path.basename(path)
    if data[:4] == _ELF_MAGIC:
        return [_elf_slice(data, source)]
    if len(data) < _FAT_HEADER_SIZE:
        raise ValueError(f"{source}: file too short to be an object file")
    magic_be: int = struct.unpack_from(">I", data, 0)[0]
    if magic_be in (_FAT_MAGIC, _FAT_MAGIC_64):
        return _fat_slices(data, source)
    magic_le: int = struct.unpack_from("<I", data, 0)[0]
    if magic_le in (_MH_MAGIC_64, _MH_CIGAM_64):
        return [_macho_slice(data, source)]
    raise ValueError(f"{source}: not an ELF64, Mach-O 64 or fat Mach-O object")


def _read_uleb128(data: bytearray, pos: int, limit: int) -> tuple[int, int]:
    """Decode an unsigned LEB128 at pos; return (value, position after it)."""
    value = 0
    shift = 0
    while True:
        if pos >= limit:
            raise ValueError("truncated LEB128 in CIE")
        byte = data[pos]
        pos += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if not byte & 0x80:
            return value, pos


def parse_ehframe(eh_frame: bytes, endian: str, text_size: int) -> EhFrame:
    """Validate a one-CIE, one-FDE .eh_frame and zero the FDE's fields.

    text_size is the size of the object's .text section; it must equal the
    FDE's address_range, which catches a misplaced .cfi_endproc.
    """
    data = bytearray(eh_frame)
    if len(data) < 8:
        raise ValueError("no CIE found in .eh_frame")

    # CIE header: length, CIE_id (0), version, augmentation string.
    cie_length, cie_id = struct.unpack_from(f"{endian}II", data, 0)
    if cie_id != 0:
        raise ValueError(f"expected a CIE at offset 0, got CIE_id={cie_id:#x}")
    cie_total: int = 4 + cie_length
    if cie_length < _CIE_MIN_LENGTH or cie_total > len(data):
        raise ValueError(f"bad CIE length {cie_length}")

    version = data[8]
    if version != 1:
        raise ValueError(f"unexpected CIE version {version}")

    null_pos = data.find(0, 9, cie_total)
    if null_pos < 0:
        raise ValueError("CIE augmentation string not null-terminated")
    augmentation = data[9:null_pos].decode("ascii", errors="replace")
    if augmentation != "zR":
        raise ValueError(f"CIE augmentation {augmentation!r} is not 'zR'")

    # After the augmentation string: code alignment factor (ULEB128), data
    # alignment factor (SLEB128, skipped the same way), return address column
    # (one byte in version 1), augmentation data length (ULEB128), then the
    # augmentation data, which for "zR" is exactly the FDE pointer encoding.
    pos = null_pos + 1
    _, pos = _read_uleb128(data, pos, cie_total)
    _, pos = _read_uleb128(data, pos, cie_total)
    pos += 1
    aug_length, pos = _read_uleb128(data, pos, cie_total)
    if aug_length != 1 or pos + aug_length > cie_total:
        raise ValueError(f"CIE augmentation data length {aug_length} is not 1")
    fde_ptr_enc = data[pos]
    try:
        field_size = _FDE_FIELD_SIZES[fde_ptr_enc]
    except KeyError:
        raise ValueError(f"unsupported FDE pointer encoding {fde_ptr_enc:#x}") from None

    # FDE: length, CIE pointer, initial_location, address_range, augmentation
    # data length (0 for a "zR" CIE), then the CFI instructions.
    fde_start = cie_total
    fde_min_length = 4 + 2 * field_size + 1
    if fde_start + 4 + fde_min_length > len(data):
        raise ValueError("no FDE after the CIE")
    fde_length, fde_cie_ptr = struct.unpack_from(f"{endian}II", data, fde_start)
    if fde_length < fde_min_length:
        raise ValueError(f"FDE too short ({fde_length} bytes)")
    if fde_cie_ptr != fde_start + 4:
        raise ValueError(f"FDE CIE pointer {fde_cie_ptr} does not point at the CIE")
    if fde_start + 4 + fde_length != len(data):
        raise ValueError("expected exactly one FDE ending at the end of .eh_frame")

    fde_pc_offset = fde_start + 8
    fde_range_offset = fde_pc_offset + field_size
    fde_aug_length = data[fde_range_offset + field_size]
    if fde_aug_length != 0:
        raise ValueError(f"FDE augmentation data length {fde_aug_length} is not 0")
    address_range = int.from_bytes(
        data[fde_range_offset : fde_range_offset + field_size],
        "little" if endian == "<" else "big",
    )
    if address_range != text_size:
        raise ValueError(
            f"FDE address_range {address_range} != .text size {text_size}; "
            "check the .cfi_startproc/.cfi_endproc placement"
        )

    # Zero the placeholders. The runtime fills in the real values.
    data[fde_pc_offset : fde_range_offset + field_size] = bytes(2 * field_size)
    return EhFrame(bytes(data), fde_pc_offset, fde_range_offset, field_size)


def build_ehframe(obj_slice: ObjectSlice) -> EhFrame:
    """Extract and validate the .eh_frame of one object slice."""
    sections = obj_slice.sections
    if ".eh_frame" not in sections:
        raise ValueError(
            f"{obj_slice.source}: no .eh_frame section; does the assembly "
            "have .cfi_startproc/.cfi_endproc directives?"
        )
    if ".text" not in sections:
        raise ValueError(f"{obj_slice.source}: no .text section")
    try:
        return parse_ehframe(
            sections[".eh_frame"], obj_slice.endian, len(sections[".text"])
        )
    except ValueError as exc:
        raise ValueError(f"{obj_slice.source}: {exc}") from None


def _format_bytes(data: bytes, per_line: int = 12) -> str:
    lines = []
    for start in range(0, len(data), per_line):
        chunk = data[start : start + per_line]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    return "\n".join(lines)


def write_header(entries: list[tuple[ObjectSlice, EhFrame]], output_path: str) -> None:
    """Write the C header; entries is a list of (ObjectSlice, EhFrame)."""
    if not entries:
        raise ValueError("no architectures to write")
    sources = sorted({obj_slice.source.split(" (")[0] for obj_slice, _ in entries})
    lines = [
        "/* Auto-generated by Tools/jit/_trampoline_ehframe.py"
        f" from {', '.join(sources)}. Do not edit. */",
        "",
        "#include <stdint.h>",
        "",
        "/* .eh_frame of the perf trampoline. The FDE's initial_location and",
        " * address_range are zeroed placeholders patched by Python/jit_unwind.c. */",
    ]
    keyword = "#if"
    for obj_slice, eh_frame in sorted(entries, key=lambda e: e[0].arch_macro):
        lines += [
            f"{keyword} defined({obj_slice.arch_macro})",
            f"/* From {obj_slice.source}. */",
            "static const uint8_t _trampoline_ehframe[] = {",
            _format_bytes(eh_frame.data),
            "};",
            f"#define TRAMPOLINE_EHFRAME_FDE_PC_OFFSET {eh_frame.fde_pc_offset}",
            f"#define TRAMPOLINE_EHFRAME_FDE_RANGE_OFFSET {eh_frame.fde_range_offset}",
            f"#define TRAMPOLINE_EHFRAME_FDE_FIELD_SIZE {eh_frame.field_size}",
        ]
        keyword = "#elif"
    lines += [
        "#else",
        '#  error "trampoline_ehframe.h was not generated for this architecture"',
        "#endif",
        "",
        "#define TRAMPOLINE_EHFRAME_SIZE sizeof(_trampoline_ehframe)",
        "",
    ]
    output = "\n".join(lines)

    tmp_path = output_path + ".tmp"
    try:
        with open(tmp_path, "w") as f:
            f.write(output)
        os.replace(tmp_path, output_path)
    except BaseException:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)
        raise


def generate(
    object_paths: list[str], output_path: str
) -> list[tuple[ObjectSlice, EhFrame]]:
    """Generate the header from the given objects; return what was written."""
    entries: list[tuple[ObjectSlice, EhFrame]] = []
    seen: dict[str, str] = {}
    for path in object_paths:
        for obj_slice in load_object(path):
            if obj_slice.arch_macro in seen:
                raise ValueError(
                    f"{obj_slice.source} and {seen[obj_slice.arch_macro]} "
                    f"are both {obj_slice.arch_macro}"
                )
            seen[obj_slice.arch_macro] = obj_slice.source
            entries.append((obj_slice, build_ehframe(obj_slice)))
    write_header(entries, output_path)
    return entries


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate trampoline_ehframe.h from trampoline objects"
    )
    parser.add_argument(
        "objects",
        nargs="+",
        metavar="OBJECT",
        help="compiled trampoline object (ELF64, Mach-O 64, or fat Mach-O)",
    )
    parser.add_argument(
        "-o", "--output", required=True, help="path of the C header to write"
    )
    args = parser.parse_args()
    try:
        entries = generate(args.objects, args.output)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
    for obj_slice, eh_frame in entries:
        print(
            f"Generated {args.output}: {obj_slice.arch_macro} from "
            f"{obj_slice.source}, {len(eh_frame.data)} bytes"
        )


if __name__ == "__main__":
    main()
