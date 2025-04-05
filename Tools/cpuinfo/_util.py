from __future__ import annotations

__all__ = ["next_block", "make_enum_member"]


def next_block(w: int) -> int:
    """Compute the smallest multiple of 4 strictly larger than *w*."""
    return ((w + 3) & ~0x03) if (w % 4) else (w + 4)


_MASKSIZE: Final[int] = next_block(len("= 0x00000000,"))


def make_enum_member(key: str, bit: int, name_maxsize: int) -> str:
    member_name = key.ljust(name_maxsize)
    member_mask = format(1 << bit, "008x")
    member_mask = f"= 0x{member_mask},".ljust(_MASKSIZE)
    return f"{member_name}{member_mask} // bit = {bit}"
