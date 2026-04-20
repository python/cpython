"""Minimal DWARF .eh_frame parser for the JIT shim's CFI extraction.

Reads a compiled object's .eh_frame section and returns the bytes we
need to splice into the runtime-emitted EH frame in Python/jit_unwind.c:
the CIE's initial CFI, the FDE's CFI, and the CIE's alignment factors
and RA column.
"""

import _stencils


def _read_uleb128(data: bytes, pos: int) -> tuple[int, int]:
    result = shift = n = 0
    while True:
        b = data[pos + n]
        n += 1
        result |= (b & 0x7F) << shift
        if not (b & 0x80):
            return result, n
        shift += 7


def _read_sleb128(data: bytes, pos: int) -> tuple[int, int]:
    result = shift = n = 0
    while True:
        b = data[pos + n]
        n += 1
        result |= (b & 0x7F) << shift
        shift += 7
        if not (b & 0x80):
            if b & 0x40:
                result -= 1 << shift
            return result, n


def parse(data: bytes) -> _stencils.ShimCfi:
    """
    Parse a compiled .eh_frame section and return the shim's CFI bytes.

    Extracts the CIE's initial CFI (state at function entry) and the FDE's
    CFI (prologue transitions) as raw bytes, plus the CIE's CAF/DAF/RA
    column so jit_unwind.c can emit a matching synthetic CIE at runtime.

    Assumptions (verified as they're read):
    - The object has exactly one CIE covering one FDE (the shim).
    - FDE pointer encoding is DW_EH_PE_absptr or sdata4 (compilers always
      pick one of these for an unlinked object); we skip the PC+range
      fields either way since we re-emit them at runtime.
    """
    pos = 0
    cie_init_cfi: bytes | None = None
    code_align = data_align = ra_column = None
    fde_cfi: bytes | None = None
    cie_has_z = False
    # sdata4 default; adjusted below if the CIE's R augmentation says absptr.
    fde_ptr_len = 4
    while pos < len(data):
        length = int.from_bytes(data[pos : pos + 4], "little")
        if length == 0:
            break
        entry_end = pos + 4 + length
        pos += 4
        cie_id = int.from_bytes(data[pos : pos + 4], "little")
        pos += 4
        if cie_id == 0:
            assert data[pos] == 1, "only DWARF CIE version 1 supported"
            pos += 1
            aug_end = data.index(0, pos)
            augmentation = data[pos:aug_end].decode("ascii")
            pos = aug_end + 1
            code_align, n = _read_uleb128(data, pos)
            pos += n
            data_align, n = _read_sleb128(data, pos)
            pos += n
            ra_column = data[pos]
            pos += 1
            if "z" in augmentation:
                cie_has_z = True
                aug_data_len, n = _read_uleb128(data, pos)
                pos += n
                aug_data_start = pos
                for ch in augmentation[1:]:  # skip 'z'
                    if ch == "R":
                        enc = data[pos]
                        if enc == 0x00:  # DW_EH_PE_absptr
                            fde_ptr_len = 8
                        pos += 1
                    elif ch in ("L", "P"):
                        raise AssertionError(
                            f"shim .eh_frame has augmentation {ch!r}; unsupported"
                        )
                pos = aug_data_start + aug_data_len
            # Trailing 0-bytes are DW_CFA_nop alignment padding; strip
            # them so the splice at runtime re-aligns without double-pad.
            cie_init_cfi = bytes(data[pos:entry_end]).rstrip(b"\x00")
        else:
            assert cie_has_z, "FDE before CIE"
            pos += fde_ptr_len  # pc_start (we re-emit with absolute encoding)
            pos += fde_ptr_len  # pc_range
            aug_data_len, n = _read_uleb128(data, pos)
            pos += n
            pos += aug_data_len
            fde_cfi = bytes(data[pos:entry_end]).rstrip(b"\x00")  # nop padding
        pos = entry_end
    assert cie_init_cfi is not None and fde_cfi is not None, "no CIE/FDE in .eh_frame"
    return _stencils.ShimCfi(
        cie_init_cfi=cie_init_cfi,
        fde_cfi=fde_cfi,
        code_align=code_align,
        data_align=data_align,
        ra_column=ra_column,
    )
