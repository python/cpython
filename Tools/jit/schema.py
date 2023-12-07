"""Schema for the JSON produced by llvm-readobj --elf-output-style=JSON."""

# pylint: disable = missing-class-docstring

import typing

HoleKind: typing.TypeAlias = typing.Literal[
    "ARM64_RELOC_GOT_LOAD_PAGE21",
    "ARM64_RELOC_GOT_LOAD_PAGEOFF12",
    "ARM64_RELOC_UNSIGNED",
    "IMAGE_REL_AMD64_ADDR64",
    "IMAGE_REL_I386_DIR32",
    "R_AARCH64_ABS64",
    "R_AARCH64_CALL26",
    "R_AARCH64_JUMP26",
    "R_AARCH64_MOVW_UABS_G0_NC",
    "R_AARCH64_MOVW_UABS_G1_NC",
    "R_AARCH64_MOVW_UABS_G2_NC",
    "R_AARCH64_MOVW_UABS_G3",
    "R_X86_64_64",
    "X86_64_RELOC_UNSIGNED",
]


class RelocationType(typing.TypedDict):
    Value: HoleKind
    RawValue: int


class _Value(typing.TypedDict):
    Value: str
    RawValue: int


class Flag(typing.TypedDict):
    Name: str
    Value: int


class Flags(typing.TypedDict):
    RawFlags: int
    Flags: list[Flag]


class SectionData(typing.TypedDict):
    Offset: int
    Bytes: list[int]


class _Name(typing.TypedDict):
    Value: str
    Offset: int
    Bytes: list[int]


class ELFRelocation(typing.TypedDict):
    Offset: int
    Type: RelocationType
    Symbol: _Value
    Addend: int


class COFFRelocation(typing.TypedDict):
    Offset: int
    Type: RelocationType
    Symbol: str
    SymbolIndex: int


class MachORelocation(typing.TypedDict):
    Offset: int
    PCRel: int
    Length: int
    Type: RelocationType
    Symbol: _Value  # XXX
    Section: _Value  # XXX


class COFFAuxSectionDef(typing.TypedDict):
    Length: int
    RelocationCount: int
    LineNumberCount: int
    Checksum: int
    Number: int
    Selection: int


class COFFSymbol(typing.TypedDict):
    Name: str
    Value: int
    Section: _Value
    BaseType: _Value
    ComplexType: _Value
    StorageClass: int
    AuxSymbolCount: int
    AuxSectionDef: COFFAuxSectionDef


class ELFSymbol(typing.TypedDict):
    Name: _Value
    Value: int
    Size: int
    Binding: _Value
    Type: _Value
    Other: int
    Section: _Value


class MachOSymbol(typing.TypedDict):
    Name: _Value
    Type: _Value
    Section: _Value
    RefType: _Value
    Flags: Flags
    Value: int


class ELFSection(typing.TypedDict):
    Index: int
    Name: _Value
    Type: _Value
    Flags: Flags
    Address: int
    Offset: int
    Size: int
    Link: int
    Info: int
    AddressAlignment: int
    EntrySize: int
    Relocations: list[dict[typing.Literal["Relocation"], ELFRelocation]]
    Symbols: list[dict[typing.Literal["Symbol"], ELFSymbol]]
    SectionData: SectionData


class COFFSection(typing.TypedDict):
    Number: int
    Name: _Name
    VirtualSize: int
    VirtualAddress: int
    RawDataSize: int
    PointerToRawData: int
    PointerToRelocations: int
    PointerToLineNumbers: int
    RelocationCount: int
    LineNumberCount: int
    Characteristics: Flags
    Relocations: list[dict[typing.Literal["Relocation"], COFFRelocation]]
    Symbols: list[dict[typing.Literal["Symbol"], COFFSymbol]]
    SectionData: SectionData  # XXX


class MachOSection(typing.TypedDict):
    Index: int
    Name: _Name
    Segment: _Name
    Address: int
    Size: int
    Offset: int
    Alignment: int
    RelocationOffset: int
    RelocationCount: int
    Type: _Value
    Attributes: Flags
    Reserved1: int
    Reserved2: int
    Reserved3: int
    Relocations: list[dict[typing.Literal["Relocation"], MachORelocation]]  # XXX
    Symbols: list[dict[typing.Literal["Symbol"], MachOSymbol]]  # XXX
    SectionData: SectionData  # XXX
