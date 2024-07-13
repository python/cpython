"""Schema for the JSON produced by llvm-readobj --elf-output-style=JSON."""

import typing

HoleKind: typing.TypeAlias = typing.Literal[
    "ARM64_RELOC_BRANCH26",
    "ARM64_RELOC_GOT_LOAD_PAGE21",
    "ARM64_RELOC_GOT_LOAD_PAGEOFF12",
    "ARM64_RELOC_PAGE21",
    "ARM64_RELOC_PAGEOFF12",
    "ARM64_RELOC_UNSIGNED",
    "IMAGE_REL_AMD64_REL32",
    "IMAGE_REL_ARM64_BRANCH26",
    "IMAGE_REL_ARM64_PAGEBASE_REL21",
    "IMAGE_REL_ARM64_PAGEOFFSET_12A",
    "IMAGE_REL_ARM64_PAGEOFFSET_12L",
    "IMAGE_REL_I386_DIR32",
    "IMAGE_REL_I386_REL32",
    "R_AARCH64_ABS64",
    "R_AARCH64_ADR_GOT_PAGE",
    "R_AARCH64_ADR_PREL_PG_HI21",
    "R_AARCH64_CALL26",
    "R_AARCH64_JUMP26",
    "R_AARCH64_ADD_ABS_LO12_NC",
    "R_AARCH64_LD64_GOT_LO12_NC",
    "R_AARCH64_MOVW_UABS_G0_NC",
    "R_AARCH64_MOVW_UABS_G1_NC",
    "R_AARCH64_MOVW_UABS_G2_NC",
    "R_AARCH64_MOVW_UABS_G3",
    "R_X86_64_64",
    "R_X86_64_GOTPCREL",
    "R_X86_64_GOTPCRELX",
    "R_X86_64_PC32",
    "R_X86_64_REX_GOTPCRELX",
    "X86_64_RELOC_BRANCH",
    "X86_64_RELOC_GOT",
    "X86_64_RELOC_GOT_LOAD",
    "X86_64_RELOC_SIGNED",
    "X86_64_RELOC_UNSIGNED",
]


class COFFRelocation(typing.TypedDict):
    """A COFF object file relocation record."""

    Type: dict[typing.Literal["Value"], HoleKind]
    Symbol: str
    Offset: int


class ELFRelocation(typing.TypedDict):
    """An ELF object file relocation record."""

    Addend: int
    Offset: int
    Symbol: dict[typing.Literal["Value"], str]
    Type: dict[typing.Literal["Value"], HoleKind]


class MachORelocation(typing.TypedDict):
    """A Mach-O object file relocation record."""

    Offset: int
    Section: typing.NotRequired[dict[typing.Literal["Value"], str]]
    Symbol: typing.NotRequired[dict[typing.Literal["Value"], str]]
    Type: dict[typing.Literal["Value"], HoleKind]


class _COFFSymbol(typing.TypedDict):
    Name: str
    Value: int


class _ELFSymbol(typing.TypedDict):
    Name: dict[typing.Literal["Name"], str]
    Value: int


class _MachOSymbol(typing.TypedDict):
    Name: dict[typing.Literal["Name"], str]
    Value: int


class COFFSection(typing.TypedDict):
    """A COFF object file section."""

    Characteristics: dict[
        typing.Literal["Flags"], list[dict[typing.Literal["Name"], str]]
    ]
    Number: int
    RawDataSize: int
    Relocations: list[dict[typing.Literal["Relocation"], COFFRelocation]]
    SectionData: typing.NotRequired[dict[typing.Literal["Bytes"], list[int]]]
    Symbols: list[dict[typing.Literal["Symbol"], _COFFSymbol]]


class ELFSection(typing.TypedDict):
    """An ELF object file section."""

    Flags: dict[typing.Literal["Flags"], list[dict[typing.Literal["Name"], str]]]
    Index: int
    Info: int
    Relocations: list[dict[typing.Literal["Relocation"], ELFRelocation]]
    SectionData: dict[typing.Literal["Bytes"], list[int]]
    Symbols: list[dict[typing.Literal["Symbol"], _ELFSymbol]]
    Type: dict[typing.Literal["Name"], str]


class MachOSection(typing.TypedDict):
    """A Mach-O object file section."""

    Address: int
    Attributes: dict[typing.Literal["Flags"], list[dict[typing.Literal["Name"], str]]]
    Index: int
    Name: dict[typing.Literal["Value"], str]
    Relocations: typing.NotRequired[
        list[dict[typing.Literal["Relocation"], MachORelocation]]
    ]
    SectionData: typing.NotRequired[dict[typing.Literal["Bytes"], list[int]]]
    Symbols: typing.NotRequired[list[dict[typing.Literal["Symbol"], _MachOSymbol]]]
