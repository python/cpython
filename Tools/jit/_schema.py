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


class _RelocationType(typing.TypedDict):
    Value: HoleKind
    RawValue: int


class _WrappedValue(typing.TypedDict):
    Value: str
    RawValue: int


class _Flag(typing.TypedDict):
    Name: str
    Value: int


class _Flags(typing.TypedDict):
    RawFlags: int
    Flags: list[_Flag]


class _SectionData(typing.TypedDict):
    Offset: int
    Bytes: list[int]


class _Name(typing.TypedDict):
    Value: str
    Offset: int
    Bytes: list[int]


class ELFRelocation(typing.TypedDict):
    """An ELF object file relocation record."""

    Offset: int
    Type: _RelocationType
    Symbol: _WrappedValue
    Addend: int


class COFFRelocation(typing.TypedDict):
    """A COFF object file relocation record."""

    Offset: int
    Type: _RelocationType
    Symbol: str
    SymbolIndex: int


class MachORelocation(typing.TypedDict):
    """A Mach-O object file relocation record."""

    Offset: int
    PCRel: int
    Length: int
    Type: _RelocationType
    Symbol: typing.NotRequired[_WrappedValue]
    Section: typing.NotRequired[_WrappedValue]


class _COFFAuxSectionDef(typing.TypedDict):
    Length: int
    RelocationCount: int
    LineNumberCount: int
    Checksum: int
    Number: int
    Selection: int


class _COFFSymbol(typing.TypedDict):
    Name: str
    Value: int
    Section: _WrappedValue
    BaseType: _WrappedValue
    ComplexType: _WrappedValue
    StorageClass: int
    AuxSymbolCount: int
    AuxSectionDef: _COFFAuxSectionDef


class _ELFSymbol(typing.TypedDict):
    Name: _WrappedValue
    Value: int
    Size: int
    Binding: _WrappedValue
    Type: _WrappedValue
    Other: int
    Section: _WrappedValue


class _MachOSymbol(typing.TypedDict):
    Name: _WrappedValue
    Type: _WrappedValue
    Section: _WrappedValue
    RefType: _WrappedValue
    Flags: _Flags
    Value: int


class ELFSection(typing.TypedDict):
    """An ELF object file section."""

    Index: int
    Name: _WrappedValue
    Type: _WrappedValue
    Flags: _Flags
    Address: int
    Offset: int
    Size: int
    Link: int
    Info: int
    AddressAlignment: int
    EntrySize: int
    Relocations: list[dict[typing.Literal["Relocation"], ELFRelocation]]
    Symbols: list[dict[typing.Literal["Symbol"], _ELFSymbol]]
    SectionData: _SectionData


class COFFSection(typing.TypedDict):
    """A COFF object file section."""

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
    Characteristics: _Flags
    Relocations: list[dict[typing.Literal["Relocation"], COFFRelocation]]
    Symbols: list[dict[typing.Literal["Symbol"], _COFFSymbol]]
    SectionData: typing.NotRequired[_SectionData]


class MachOSection(typing.TypedDict):
    """A Mach-O object file section."""

    Index: int
    Name: _Name
    Segment: _Name
    Address: int
    Size: int
    Offset: int
    Alignment: int
    RelocationOffset: int
    RelocationCount: int
    Type: _WrappedValue
    Attributes: _Flags
    Reserved1: int
    Reserved2: int
    Reserved3: int
    Relocations: typing.NotRequired[
        list[dict[typing.Literal["Relocation"], MachORelocation]]
    ]
    Symbols: typing.NotRequired[list[dict[typing.Literal["Symbol"], _MachOSymbol]]]
    SectionData: typing.NotRequired[_SectionData]
