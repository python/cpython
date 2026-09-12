import pathlib
import struct
import sysconfig
import unittest
from unittest import mock

from test.support.os_helper import temp_dir
from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("jit")
with imports_under_tool("jit"):
    import _trampoline_ehframe as ehframe


DW_EH_PE_PCREL_SDATA4 = 0x1B
DW_EH_PE_PCREL_ABSPTR = 0x10


def _fake_cie(*, version=1, augmentation=b"zR", ra_column=16,
              encoding=DW_EH_PE_PCREL_SDATA4, cie_id=0):
    """A CIE like the assembler's: code align 1, data align -8, one
    DW_CFA_def_cfa instruction, padded with DW_CFA_nop to 8 bytes."""
    body = bytes([version]) + augmentation + b"\x00"
    body += bytes([1, 0x78, ra_column, 1, encoding])
    body += bytes([0x0C, 7, 8])  # DW_CFA_def_cfa: r7 (rsp) ofs 8
    body += b"\x00" * (-(8 + len(body)) % 8)
    return struct.pack("<II", 4 + len(body), cie_id) + body


def _fake_fde(cie_total, *, field_size=4, address_range=8,
              instructions=b"\x41\x0e\x10\x86\x02"):
    """An FDE right after a CIE of cie_total bytes, padded to 8 bytes."""
    body = struct.pack("<I", cie_total + 4)  # CIE pointer, relative to itself
    # initial_location as an assembler would leave it, the parser zeroes it.
    body += (-40).to_bytes(field_size, "little", signed=True)
    body += address_range.to_bytes(field_size, "little")
    body += b"\x00"  # augmentation data length
    body += instructions
    body += b"\x00" * (-(4 + len(body)) % 8)
    return struct.pack("<I", len(body)) + body


def _fake_macho(cputype, text, eh_frame):
    # A minimal MH_OBJECT: one __TEXT segment with __text and
    # __eh_frame sections, section data right after the load command.
    segment_size = 72 + 2 * 80
    text_offset = 32 + segment_size
    eh_offset = text_offset + len(text)
    sections = b""
    for name, size, offset in (
        (b"__text", len(text), text_offset),
        (b"__eh_frame", len(eh_frame), eh_offset),
    ):
        sections += struct.pack(
            "<16s16sQQIIIIIIII", name, b"__TEXT", 0, size, offset,
            0, 0, 0, 0, 0, 0, 0,
        )
    segment = struct.pack(
        "<II16sQQQQIIII", ehframe._LC_SEGMENT_64,
        segment_size, b"__TEXT", 0, len(text) + len(eh_frame), text_offset,
        len(text) + len(eh_frame), 7, 5, 2, 0,
    )
    header = struct.pack(
        "<IIIIIIII", ehframe._MH_MAGIC_64, cputype, 0,
        1, 1, segment_size, 0, 0,
    )
    return header + segment + sections + text + eh_frame


def _fake_fat_macho(blobs, *, fat64=False):
    # The fat header and its fat_arch entries are big-endian.
    entry = struct.Struct(">IIQQII" if fat64 else ">IIIII")
    offset = 8 + entry.size * len(blobs)
    entries = b""
    body = b""
    for cputype, blob in blobs:
        fields = (cputype, 0, offset + len(body), len(blob), 0)
        if fat64:
            fields += (0,)  # reserved
        entries += entry.pack(*fields)
        body += blob
    magic = ehframe._FAT_MAGIC_64 if fat64 else ehframe._FAT_MAGIC
    return struct.pack(">II", magic, len(blobs)) + entries + body


class TestEhFrameParsing(unittest.TestCase):
    def parse(self, data, text_size=8):
        return ehframe.parse_ehframe(bytes(data), "<", text_size)

    def test_parse(self):
        """Both FDE pointer encodings: ELF sdata4 and Darwin absptr."""
        cases = [
            (DW_EH_PE_PCREL_SDATA4, 4, 16, 8),
            (DW_EH_PE_PCREL_ABSPTR, 8, 30, 20),
        ]
        for encoding, field_size, ra_column, text_size in cases:
            with self.subTest(encoding=hex(encoding)):
                cie = _fake_cie(encoding=encoding, ra_column=ra_column)
                fde = _fake_fde(
                    len(cie), field_size=field_size, address_range=text_size
                )
                result = self.parse(cie + fde, text_size)
                self.assertEqual(result.field_size, field_size)
                self.assertEqual(result.fde_pc_offset, len(cie) + 8)
                self.assertEqual(result.fde_range_offset, len(cie) + 8 + field_size)
                # Both patchable fields zeroed, everything else untouched.
                expected = bytearray(cie + fde)
                pc_offset = len(cie) + 8
                expected[pc_offset:pc_offset + 2 * field_size] = bytes(2 * field_size)
                self.assertEqual(result.data, bytes(expected))

    def test_parse_rejects_malformed(self):
        cie = _fake_cie()
        fde = _fake_fde(len(cie))
        cases = [
            ("no CIE", b"", 8),
            ("CIE_id", _fake_cie(cie_id=1) + fde, 8),
            ("bad CIE length", cie[:12], 8),
            ("version", _fake_cie(version=3) + fde, 8),
            ("augmentation", _fake_cie(augmentation=b"zPLR") + fde, 8),
            ("encoding", _fake_cie(encoding=0x1A) + fde, 8),
            ("exactly one FDE", cie + fde + fde, 8),
            ("address_range", cie + fde, 12),
            ("no FDE", cie, 8),
        ]
        for message, data, text_size in cases:
            with self.subTest(message):
                with self.assertRaisesRegex(ValueError, message):
                    self.parse(data, text_size)


class TestObjectLoading(unittest.TestCase):
    def load_object(self, data):
        with temp_dir() as tmp:
            path = pathlib.Path(tmp) / "trampoline.o"
            path.write_bytes(data)
            return ehframe.load_object(path)

    def test_macho_thin(self):
        text = b"\xc0\x03\x5f\xd6"
        data = _fake_macho(ehframe._CPU_TYPE_ARM64, text, b"arm64 eh_frame")
        (obj,) = self.load_object(data)
        self.assertEqual(obj.arch_macro, "__aarch64__")
        self.assertEqual(obj.sections, {".text": text, ".eh_frame": b"arm64 eh_frame"})

    def test_macho_fat(self):
        x86 = _fake_macho(ehframe._CPU_TYPE_X86_64, b"\x55\xc3", b"x86 eh_frame")
        arm = _fake_macho(
            ehframe._CPU_TYPE_ARM64, b"\xc0\x03\x5f\xd6", b"arm64 eh_frame"
        )
        blobs = [
            (ehframe._CPU_TYPE_X86_64, x86),
            (ehframe._CPU_TYPE_ARM64, arm),
        ]
        for fat64 in (False, True):
            with self.subTest(fat64=fat64):
                slices = self.load_object(_fake_fat_macho(blobs, fat64=fat64))
                self.assertEqual([s.arch_macro for s in slices],
                                 ["__x86_64__", "__aarch64__"])
                for obj, (_, blob) in zip(slices, blobs, strict=True):
                    with self.subTest(arch=obj.arch_macro):
                        (thin,) = self.load_object(blob)
                        self.assertEqual(obj.sections, thin.sections)

    def test_macho_rejects_malformed_sections(self):
        data = _fake_macho(ehframe._CPU_TYPE_ARM64, b"text", b"eh_frame")
        cases = [
            ("truncated segment command", 32 + 4, "I", 16),
            ("section table extends", 32 + 64, "I", 3),
            ("section .text extends", 32 + 72 + 48, "I", len(data)),
        ]
        for message, offset, fmt, value in cases:
            with self.subTest(message):
                malformed = bytearray(data)
                struct.pack_into("<" + fmt, malformed, offset, value)
                with self.assertRaisesRegex(ValueError, message):
                    self.load_object(malformed)


class TestHeaderGeneration(unittest.TestCase):
    def _build_trampoline_objects(self):
        """The object(s) the Makefile fed to the generator."""
        builddir = pathlib.Path(sysconfig.get_config_var("abs_builddir") or ".")
        universal2 = builddir / "Python" / "asm_trampoline_universal2.o"
        if universal2.exists():
            return [universal2]
        return sorted(
            path for path in (builddir / "Python").glob("asm_trampoline_*.o")
            if "apple-darwin" not in path.name
        )

    def test_failed_replace_preserves_header(self):
        cie = _fake_cie()
        obj = ehframe.ObjectSlice(
            "trampoline.o", "__x86_64__", "<",
            {".text": bytes(8), ".eh_frame": cie + _fake_fde(len(cie))},
        )
        entries = [(obj, ehframe.build_ehframe(obj))]
        with temp_dir() as tmp:
            path = pathlib.Path(tmp) / "trampoline_ehframe.h"
            path.write_text("previous header")
            with (
                mock.patch.object(pathlib.Path, "replace", side_effect=OSError),
                self.assertRaises(OSError),
            ):
                ehframe.write_header(entries, path)
            self.assertEqual(path.read_text(), "previous header")
            self.assertEqual(list(path.parent.iterdir()), [path])

    def test_generated_header_is_current(self):
        """The header in the build directory matches a fresh generation."""
        objects = self._build_trampoline_objects()
        builddir = pathlib.Path(sysconfig.get_config_var("abs_builddir") or ".")
        header = builddir / "trampoline_ehframe.h"
        if not objects or not header.exists():
            self.skipTest("trampoline object or generated header not found")
        current = header.read_text()
        with temp_dir() as tmp:
            fresh_path = pathlib.Path(tmp) / "trampoline_ehframe.h"
            ehframe.generate(objects, fresh_path)
            fresh = fresh_path.read_text()
        self.assertEqual(current, fresh)


if __name__ == "__main__":
    unittest.main()
