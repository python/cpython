# pylint: disable = missing-class-docstring
"""Schema for the JSON produced by llvm-readobj --elf-output-style=JSON."""
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


class WrappedValue(typing.TypedDict):
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
    Symbol: WrappedValue
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
    Symbol: typing.NotRequired[WrappedValue]
    Section: typing.NotRequired[WrappedValue]


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
    Section: WrappedValue
    BaseType: WrappedValue
    ComplexType: WrappedValue
    StorageClass: int
    AuxSymbolCount: int
    AuxSectionDef: COFFAuxSectionDef


class ELFSymbol(typing.TypedDict):
    Name: WrappedValue
    Value: int
    Size: int
    Binding: WrappedValue
    Type: WrappedValue
    Other: int
    Section: WrappedValue


class MachOSymbol(typing.TypedDict):
    Name: WrappedValue
    Type: WrappedValue
    Section: WrappedValue
    RefType: WrappedValue
    Flags: Flags
    Value: int


class ELFSection(typing.TypedDict):
    Index: int
    Name: WrappedValue
    Type: WrappedValue
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
    SectionData: typing.NotRequired[SectionData]


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
    Type: WrappedValue
    Attributes: Flags
    Reserved1: int
    Reserved2: int
    Reserved3: int
    Relocations: typing.NotRequired[
        list[dict[typing.Literal["Relocation"], MachORelocation]]
    ]
    Symbols: typing.NotRequired[list[dict[typing.Literal["Symbol"], MachOSymbol]]]
    SectionData: typing.NotRequired[SectionData]
