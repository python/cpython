/*
    File:               CFMLateImport.c

    Contains:           Implementation of CFM late import library.

    Written by:         Quinn

    Copyright:          Copyright © 1999 by Apple Computer, Inc., all rights reserved.

                            You may incorporate this Apple sample source code into your program(s) without
                            restriction. This Apple sample source code has been provided "AS IS" and the
                            responsibility for its operation is yours. You are not permitted to redistribute
                            this Apple sample source code as "Apple sample source code" after having made
                            changes. If you're going to re-distribute the source, we require that you make
                            it clear in the source that the code was descended from Apple sample source
                            code, but that you've made changes.

    Change History (most recent first):

    <13>     24/9/01    Quinn   Fixes to compile with C++ activated.
    <12>     21/9/01    Quinn   [2710489] Fix typo in the comments for FragmentLookup.
    <11>     21/9/01    Quinn   Changes for CWPro7 Mach-O build.
    <10>     19/9/01    Quinn   Corrected implementation of kPEFRelocSmBySection. Added
                                implementations of kPEFRelocSetPosition and kPEFRelocLgByImport
                                (from code contributed by Eric Grant, Ned Holbrook, and Steve
                                Kalkwarf), although I can't test them yet.
     <9>     19/9/01    Quinn   We now handle unpacked data sections, courtesy of some code from
                                Ned Holbrook.
     <8>     19/9/01    Quinn   Minor fixes for the previous checkin. Updated some comments and
                                killed some dead code.
     <7>     19/9/01    Quinn   Simplified API and implementation after a suggestion by Eric
                                Grant. You no longer have to CFM export a dummy function; you
                                can just pass in the address of your fragment's init routine.
     <6>     15/2/01    Quinn   Modify compile-time warnings to complain if you try to build
                                this module into a Mach-O binary.
     <5>      5/2/01    Quinn   Removed redundant assignment in CFMLateImportCore.
     <4>    30/11/00    Quinn   Added comment about future of data symbols in CF.
     <3>    16/11/00    Quinn   Allow symbol finding via a callback and use that to implement
                                CFBundle support.
     <2>    18/10/99    Quinn   Renamed CFMLateImport to CFMLateImportLibrary to allow for
                                possible future API expansion.
     <1>     15/6/99    Quinn   First checked in.
*/

// To Do List:
//
// o get rid of dependence on ANSI "string.h", but how?
//
// Done:
//
// Ã investigate alternative APIs, like an external lookup routine
//   renamed CFMLateImport to CFMLateImportLibrary to allow for
//   future expansion of the APIs for things like CFMLateImportSymbol
// Ã test with non-zero fragment offset in the file
// Ã test more with MPW fragments
// Ã test data imports

/////////////////////////////////////////////////////////////////

// MoreIsBetter Setup

//#include "MoreSetup.h"
#define MoreAssert(x) (true)
#define MoreAssertQ(x)

// Mac OS Interfaces

#if ! MORE_FRAMEWORK_INCLUDES
    #include <CodeFragments.h>
    #include <PEFBinaryFormat.h>
#endif

// Standard C Interfaces

#include <string.h>

// MIB Prototypes

//#include "MoreInterfaceLib.h"
#define MoreBlockZero BlockZero

// Our Prototypes

#include "CFMLateImport.h"

/////////////////////////////////////////////////////////////////

#if TARGET_RT_MAC_MACHO
    #error CFMLateImport is not suitable for use in a Mach-O project.
#elif !TARGET_RT_MAC_CFM || !TARGET_CPU_PPC
    #error CFMLateImport has not been qualified for 68K or CFM-68K use.
#endif

/////////////////////////////////////////////////////////////////
#pragma mark ----- Utility Routines -----

static OSStatus FSReadAtOffset(SInt16 refNum, SInt32 offset, SInt32 count, void *buffer)
    // A convenient wrapper around PBRead which has two advantages
    // over FSRead.  First, it takes count as a value parameter.
    // Second, it reads from an arbitrary offset into the file,
    // which avoids a bunch of SetFPos calls.
    //
    // I guess this should go into "MoreFiles.h", but I'm not sure
    // how we're going to integrate such a concept into MIB yet.
{
    ParamBlockRec pb;

    pb.ioParam.ioRefNum     = refNum;
    pb.ioParam.ioBuffer     = (Ptr) buffer;
    pb.ioParam.ioReqCount   = count;
    pb.ioParam.ioPosMode    = fsFromStart;
    pb.ioParam.ioPosOffset  = offset;

    return PBReadSync(&pb);
}

/////////////////////////////////////////////////////////////////
#pragma mark ----- Late Import Engine -----

// This structure represents the core data structure of the late import
// engine.  It basically holds information about the fragment we're going
// to fix up.  It starts off with the first three fields, which are
// provided by the client.  Then, as we procede through the operation,
// we fill out more fields.

struct FragToFixInfo {
    CFragSystem7DiskFlatLocator         locator;                                // How to find the fragment's container.
    CFragConnectionID                           connID;                                 // CFM connection to the fragment.
    CFragInitFunction                           initRoutine;                    // The CFM init routine for the fragment.
    PEFContainerHeader                          containerHeader;                // The CFM header, read in from the container.
    PEFSectionHeader                            *sectionHeaders;                // The CFM section headers.  A pointer block containing an array of containerHeader.sectionCount elements.
    PEFLoaderInfoHeader                         *loaderSection;                 // The entire CFM loader section in a pointer block.
    SInt16                                              fileRef;                                // A read-only path to the CFM container.  We keep this here because one that one routine needs to read from the container.
    void                                                *section0Base;                  // The base address of section 0, which we go through hoops to calculate.
    void                                                *section1Base;                  // The base address of section 1, which we go through hoops to calculate.
    Boolean                                             disposeSectionPointers; // See below.
};
typedef struct FragToFixInfo FragToFixInfo;

// The disposeSectionPointers Boolean is designed for future cool VM
// support.  If VM is on, the entire code fragment is file mapped into
// high memory, including the data we're forced to allocate the
// sectionHeaders and loaderSection memory blocks to maintain.  If
// we could find the address of the entire file mapped container,
// we could access the information directly from there and thus
// we wouldn't need to allocate (or dispose of) the memory blocks
// for sectionHeaders and loaderSection.
//
// I haven't implemented this yet because a) I'm not sure how to do
// it with documented APIs, and b) I couldn't be bothered, but
// disposeSectionPointers remains as vestigial support for the concept.

static OSStatus ReadContainerBasics(FragToFixInfo *fragToFix)
    // Reads some basic information from the container of the
    // fragment to fix and stores it in various fields of
    // fragToFix.  This includes:
    //
    // o containerHeader -- The contain header itself.
    // o sectionHeaders  -- The array of section headers (in a newly allocated pointer block).
    // o loaderSection   -- The entire loader section (in a newly allocated pointer block).
    //
    // Also sets disposeSectionPointers to indicate whether
    // the last two pointers should be disposed of.
    //
    // Finally, it leaves the container file open for later
    // folks who want to read data from it.
{
    OSStatus            err;
    UInt16              sectionIndex;
    Boolean             found;

    MoreAssertQ(fragToFix != nil);
    MoreAssertQ(fragToFix->locator.fileSpec != nil);
    MoreAssertQ(fragToFix->connID != nil);
    MoreAssertQ(fragToFix->loaderSection == nil);
    MoreAssertQ(fragToFix->sectionHeaders == nil);
    MoreAssertQ(fragToFix->fileRef == 0);

    fragToFix->disposeSectionPointers = true;

    // Open up the file, read the container head, then read in
    // all the section headers, then go looking through the
    // section headers for the loader section (PEF defines
    // that there can be only one).

    err = FSpOpenDF(fragToFix->locator.fileSpec, fsRdPerm, &fragToFix->fileRef);
    if (err == noErr) {
        err = FSReadAtOffset(fragToFix->fileRef,
                                                        fragToFix->locator.offset,
                                                        sizeof(fragToFix->containerHeader),
                                                        &fragToFix->containerHeader);
        if (err == noErr) {
            if (   fragToFix->containerHeader.tag1 != kPEFTag1
                || fragToFix->containerHeader.tag2 != kPEFTag2
                || fragToFix->containerHeader.architecture != kCompiledCFragArch
                || fragToFix->containerHeader.formatVersion != kPEFVersion) {
                err = cfragFragmentFormatErr;
            }
        }
        if (err == noErr) {
            fragToFix->sectionHeaders = (PEFSectionHeader *) NewPtr(fragToFix->containerHeader.sectionCount * sizeof(PEFSectionHeader));
            err = MemError();
        }
        if (err == noErr) {
            err = FSReadAtOffset(fragToFix->fileRef,
                                                            fragToFix->locator.offset + sizeof(fragToFix->containerHeader),
                                                            fragToFix->containerHeader.sectionCount * sizeof(PEFSectionHeader),
                                                            fragToFix->sectionHeaders);
        }
        if (err == noErr) {
            sectionIndex = 0;
            found = false;
            while ( sectionIndex < fragToFix->containerHeader.sectionCount && ! found ) {
                found = (fragToFix->sectionHeaders[sectionIndex].sectionKind == kPEFLoaderSection);
                if ( ! found ) {
                    sectionIndex += 1;
                }
            }
        }
        if (err == noErr && ! found) {
            err = cfragNoSectionErr;
        }

        // Now read allocate a pointer block and read the loader section into it.

        if (err == noErr) {
            fragToFix->loaderSection = (PEFLoaderInfoHeader *) NewPtr(fragToFix->sectionHeaders[sectionIndex].containerLength);
            err = MemError();
        }
        if (err == noErr) {
            err = FSReadAtOffset(fragToFix->fileRef,
                                                            fragToFix->locator.offset + fragToFix->sectionHeaders[sectionIndex].containerOffset,
                                                            fragToFix->sectionHeaders[sectionIndex].containerLength,
                                                            fragToFix->loaderSection);
        }
    }

    // No clean up.  The client must init fragToFix to zeros and then
    // clean up regardless of whether we return an error.

    return err;
}

static UInt32 DecodeVCountValue(const UInt8 *start, UInt32 *outCount)
    // Given a pointer to the start of a variable length PEF value,
    // work out the value (in *outCount).  Returns the number of bytes
    // consumed by the value.
{
    UInt8 *                     bytePtr;
    UInt8                       byte;
    UInt32                      count;

    bytePtr = (UInt8 *)start;

    // Code taken from "PEFBinaryFormat.h".
    count = 0;
    do {
        byte = *bytePtr++;
        count = (count << kPEFPkDataVCountShift) | (byte & kPEFPkDataVCountMask);
    } while ((byte & kPEFPkDataVCountEndMask) != 0);

    *outCount = count;
    return bytePtr - start;
}

static UInt32 DecodeInstrCountValue(const UInt8 *inOpStart, UInt32 *outCount)
    // Given a pointer to the start of an opcode (inOpStart), work out the
    // count argument for that opcode (*outCount).  Returns the number of
    // bytes consumed by the opcode and count combination.
{
    MoreAssertQ(inOpStart != nil);
    MoreAssertQ(outCount  != nil);

    if (PEFPkDataCount5(*inOpStart) != 0)
    {
        // Simple case, count encoded in opcode.
        *outCount = PEFPkDataCount5(*inOpStart);
        return 1;
    }
    else
    {
        // Variable-length case.
        return 1 + DecodeVCountValue(inOpStart + 1, outCount);
    }
}

static OSStatus UnpackPEFDataSection(const UInt8 * const packedData,   UInt32 packedSize,
                                                                           UInt8 * const unpackedData, UInt32 unpackedSize)
{
    OSErr                       err;
    UInt32                      offset;
    UInt8                       opCode;
    UInt8 *                     unpackCursor;

    MoreAssertQ(packedData != nil);
    MoreAssertQ(unpackedData != nil);
    MoreAssertQ(unpackedSize >= packedSize);

    // The following asserts assume that the client allocated the memory with NewPtr,
    // which may not always be true.  However, the asserts' value in preventing accidental
    // memory block overruns outweighs the possible maintenance effort.

    MoreAssertQ( packedSize   == GetPtrSize( (Ptr) packedData  ) );
    MoreAssertQ( unpackedSize == GetPtrSize( (Ptr) unpackedData) );

    err          = noErr;
    offset       = 0;
    unpackCursor = unpackedData;
    while (offset < packedSize) {
        MoreAssertQ(unpackCursor < &unpackedData[unpackedSize]);

        opCode = packedData[offset];

        switch (PEFPkDataOpcode(opCode)) {
            case kPEFPkDataZero:
                {
                    UInt32                      count;

                    offset += DecodeInstrCountValue(&packedData[offset], &count);

                    MoreBlockZero(unpackCursor, count);
                    unpackCursor += count;
                }
                break;

            case kPEFPkDataBlock:
                {
                    UInt32                      blockSize;

                    offset += DecodeInstrCountValue(&packedData[offset], &blockSize);

                    BlockMoveData(&packedData[offset], unpackCursor, blockSize);
                    unpackCursor += blockSize;
                    offset += blockSize;
                }
                break;

            case kPEFPkDataRepeat:
                {
                    UInt32                      blockSize;
                    UInt32                      repeatCount;
                    UInt32  loopCounter;

                    offset += DecodeInstrCountValue(&packedData[offset], &blockSize);
                    offset += DecodeVCountValue(&packedData[offset], &repeatCount);
                    repeatCount += 1;                           // stored value is (repeatCount - 1)

                    for (loopCounter = 0; loopCounter < repeatCount; loopCounter++) {
                        BlockMoveData(&packedData[offset], unpackCursor, blockSize);
                        unpackCursor += blockSize;
                    }
                    offset += blockSize;
                }
                break;

            case kPEFPkDataRepeatBlock:
                {
                    UInt32                      commonSize;
                    UInt32                      customSize;
                    UInt32                      repeatCount;
                    const UInt8 *commonData;
                    const UInt8 *customData;
                    UInt32 loopCounter;

                    offset += DecodeInstrCountValue(&packedData[offset], &commonSize);
                    offset += DecodeVCountValue(&packedData[offset], &customSize);
                    offset += DecodeVCountValue(&packedData[offset], &repeatCount);

                    commonData = &packedData[offset];
                    customData = &packedData[offset + commonSize];

                    for (loopCounter = 0; loopCounter < repeatCount; loopCounter++) {
                        BlockMoveData(commonData, unpackCursor, commonSize);
                        unpackCursor += commonSize;
                        BlockMoveData(customData, unpackCursor, customSize);
                        unpackCursor += customSize;
                        customData += customSize;
                    }
                    BlockMoveData(commonData, unpackCursor, commonSize);
                    unpackCursor += commonSize;
                    offset += (repeatCount * (commonSize + customSize)) + commonSize;
                }
                break;

            case kPEFPkDataRepeatZero:
                {
                    UInt32                      commonSize;
                    UInt32                      customSize;
                    UInt32                      repeatCount;
                    const UInt8 *customData;
                    UInt32 loopCounter;

                    offset += DecodeInstrCountValue(&packedData[offset], &commonSize);
                    offset += DecodeVCountValue(&packedData[offset], &customSize);
                    offset += DecodeVCountValue(&packedData[offset], &repeatCount);

                    customData = &packedData[offset];

                    for (loopCounter = 0; loopCounter < repeatCount; loopCounter++) {
                        MoreBlockZero(unpackCursor, commonSize);
                        unpackCursor += commonSize;
                        BlockMoveData(customData, unpackCursor, customSize);
                        unpackCursor += customSize;
                        customData += customSize;
                    }
                    MoreBlockZero(unpackCursor, commonSize);
                    unpackCursor += commonSize;
                    offset += repeatCount * customSize;
                }
                break;

            default:
                #if MORE_DEBUG
                    DebugStr("\pUnpackPEFDataSection: Unexpected data opcode");
                #endif
                err = cfragFragmentCorruptErr;
                goto leaveNow;
                break;
        }
    }

leaveNow:
    return err;
}

/*      SetupSectionBaseAddresses Rationale
    -----------------------------------

    OK, here's where things get weird.  In order to run the relocation
    engine, I need to be able to find the base address of an instantiated
    section of the fragment we're fixing up given only its section number.
    This isn't hard for CFM to do because it's the one that instantiated the
    sections in the first place.  It's surprisingly difficult to do if
    you're not CFM.  [And you don't have access to the private CFM APis for
    doing it.]

    [Alan Lillich is going to kill me when he reads this!  I should point out
     that TVector's don't have to contain two words, they can be longer,
     and that the second word isn't necessarily a TOC pointer, it's
     just that the calling conventions require that it be put in the
     TOC register when the code is called.

     Furthermore, the code section isn't always section 0, and the data
     section isn't always section 1, and there can be zero to many sections
     of each type.

     But these niceties are besides the point: I'm doing something tricky
     because I don't have a nice API for getting section base addresses.
     If I had a nice API for doing that, none of this code would exist.
    ]

    The technique is very sneaky (thanks to Eric Grant).  The fragment to
    fix necessarily has a CFM init routine (because it needs that routine
    in order to capture the fragment location and connection ID).  Thus the
    fragment to fix must have a TVector in its data section.  TVectors are
    interesting because they're made up of two words.  The first is a pointer
    to the code that implements the routine; the second is a pointer to the TOC
    for the fragment that's exporting the TVector.  How TVectors are
    created is interesting too.  On disk, a TVector consists of two words,
    the first being the offset from the start of the code section to the
    routine, the second being the offset from the start of the data section
    to the TOC base.  When CFM prepares a TVector, it applies the following
    transform:

        tvector.codePtr = tvector.codeOffset + base of code section
        tvector.tocPtr  = tvector.tocOffset  + base of data section

    Now, you can reverse these questions to make them:

        base of code section = tvector.codePtr - tvector.codeOffset
        base of data section = tvector.dataPtr - tvector.dataOffset

    So if you can find the relocated contents of the TVector and
    find the original offsets that made up the TVector, you can then
    calculate the base address of both the code and data sections.

    Finding the relocated contents of the TVector is easy; I simply
    require the client to pass in a pointer to its init routine.
    A routine pointer is a TVector pointer, so you can just cast it
    and extract the pair of words.

    Finding the original offsets is a trickier.  My technique is to
    look up the init routine in the fragment's loader info header.  This
    yields the section number and offset where the init routine's unrelocated
    TVector exists.  Once I have that, I can just read the unrelocated TVector
    out of the file and extract the offsets.
*/

struct TVector {
    void *codePtr;
    void *tocPtr;
};
typedef struct TVector TVector;

static OSStatus SetupSectionBaseAddresses(FragToFixInfo *fragToFix)
    // This routine initialises the section0Base and section1Base
    // base fields of fragToFix to the base addresses of the
    // instantiated fragment represented by the other fields
    // of fragToFix.  The process works in three states:
    //
    // 1.       Find the contents of the relocated TVector of the
    //      fragment's initialisation routine, provided to us by
    //      the caller.
    //
    // 2.       Find the contents of the non-relocated TVector by
    //      looking it up in the PEF loader info header and then
    //      using that to read the TVector contents from disk.
    //      This yields the offsets from the section bases for
    //      the init routine.
    //
    // 3.       Subtract 2 from 3.
{
    OSStatus                            err;
    TVector *                           relocatedExport;
    SInt32                              initSection;
    UInt32                              initOffset;
    PEFSectionHeader *          initSectionHeader;
    Ptr                                         packedDataSection;
    Ptr                                         unpackedDataSection;
    TVector                             originalOffsets;

    packedDataSection   = nil;
    unpackedDataSection = nil;

    // Step 1.

    // First find the init routine's TVector, which gives us the relocated
    // offsets of the init routine into the data and code sections.

    relocatedExport = (TVector *) fragToFix->initRoutine;

    // Step 2.

    // Now find the init routine's TVector's offsets in the data section on
    // disk.  This gives us the raw offsets from the data and code section
    // of the beginning of the init routine.

    err = noErr;
    initSection = fragToFix->loaderSection->initSection;
    initOffset  = fragToFix->loaderSection->initOffset;
    if (initSection == -1) {
        err = cfragFragmentUsageErr;
    }
    if (err == noErr) {
        MoreAssertQ( initSection >= 0 );                        // Negative indexes are pseudo-sections which are just not allowed!
        MoreAssertQ( initSection < fragToFix->containerHeader.sectionCount );

        initSectionHeader = &fragToFix->sectionHeaders[initSection];

        // If the data section is packed, unpack it to a temporary buffer and then get the
        // original offsets from that buffer.  If the data section is unpacked, just read
        // the original offsets directly off the disk.

        if ( initSectionHeader->sectionKind == kPEFPackedDataSection ) {

            // Allocate space for packed and unpacked copies of the section.

            packedDataSection = NewPtr(initSectionHeader->containerLength);
            err = MemError();

            if (err == noErr) {
                unpackedDataSection = NewPtr(initSectionHeader->unpackedLength);
                err = MemError();
            }

            // Read the contents of the packed section.

            if (err == noErr) {
                err = FSReadAtOffset(                   fragToFix->fileRef,
                                                                fragToFix->locator.offset
                                                                + initSectionHeader->containerOffset,
                                                                initSectionHeader->containerLength,
                                                                packedDataSection);
            }

            // Unpack the data into the unpacked section.

            if (err == noErr) {
                err = UnpackPEFDataSection( (UInt8 *) packedDataSection,   initSectionHeader->containerLength,
                                                            (UInt8 *) unpackedDataSection, initSectionHeader->unpackedLength);
            }

            // Extract the init routine's TVector from the unpacked section.

            if (err == noErr) {
                BlockMoveData(unpackedDataSection + initOffset, &originalOffsets, sizeof(TVector));
            }

        } else {
            MoreAssertQ(fragToFix->sectionHeaders[initSection].sectionKind == kPEFUnpackedDataSection);
            err = FSReadAtOffset(fragToFix->fileRef,
                                                            fragToFix->locator.offset
                                                            + fragToFix->sectionHeaders[initSection].containerOffset
                                                            + initOffset,
                                                            sizeof(TVector),
                                                            &originalOffsets);
        }
    }

    // Step 3.

    // Do the maths to subtract the unrelocated offsets from the current address
    // to get the base address.

    if (err == noErr) {
        fragToFix->section0Base = ((char *) relocatedExport->codePtr) - (UInt32) originalOffsets.codePtr;
        fragToFix->section1Base = ((char *) relocatedExport->tocPtr)  - (UInt32) originalOffsets.tocPtr;
    }

    // Clean up.

    if (packedDataSection != nil) {
        DisposePtr(packedDataSection);
        MoreAssertQ( MemError() == noErr );
    }
    if (unpackedDataSection != nil) {
        DisposePtr(unpackedDataSection);
        MoreAssertQ( MemError() == noErr );
    }
    return err;
}

static void *GetSectionBaseAddress(const FragToFixInfo *fragToFix, UInt16 sectionIndex)
    // This routine returns the base of the instantiated section
    // whose index is sectionIndex.  This routine is the evil twin
    // of SetupSectionBaseAddresses.  It simply returns the values
    // for section 0 and 1 that we derived in SetupSectionBaseAddresses.
    // In a real implementation, this routine would call CFM API
    // to get this information, and SetupSectionBaseAddresses would
    // not exist, but CFM does not export the necessary APIs to
    // third parties.
{
    void *result;

    MoreAssertQ(fragToFix != nil);
    MoreAssertQ(fragToFix->containerHeader.tag1 == kPEFTag1);

    switch (sectionIndex) {
        case 0:
            result = fragToFix->section0Base;
            break;
        case 1:
            result = fragToFix->section1Base;
            break;
        default:
            result = nil;
            break;
    }
    return result;
}


static OSStatus FindImportLibrary(PEFLoaderInfoHeader *loaderSection, const char *libraryName, PEFImportedLibrary **importLibrary)
    // This routine finds the import library description (PEFImportedLibrary)
    // for the import library libraryName in the PEF loader section.
    // It sets *importLibrary to the address of the description.
{
    OSStatus                            err;
    UInt32                              librariesRemaining;
    PEFImportedLibrary          *thisImportLibrary;
    Boolean                             found;

    MoreAssertQ(loaderSection != nil);
    MoreAssertQ(libraryName != nil);
    MoreAssertQ(importLibrary != nil);

    // Loop through each import library looking for a matching name.

    // Initialise thisImportLibrary to point to the byte after the
    // end of the loader section's header.

    thisImportLibrary = (PEFImportedLibrary *) (loaderSection + 1);
    librariesRemaining = loaderSection->importedLibraryCount;
    found = false;
    while ( librariesRemaining > 0 && ! found ) {
        // PEF defines that import library names will have
        // a null terminator, so we can just use strcmp.
        found = (strcmp( libraryName,
                                        ((char *)loaderSection)
                                        + loaderSection->loaderStringsOffset
                                        + thisImportLibrary->nameOffset) == 0);
        // *** Remove ANSI strcmp eventually.
        if ( ! found ) {
            thisImportLibrary += 1;
            librariesRemaining -= 1;
        }
    }

    if (found) {
        *importLibrary = thisImportLibrary;
        err = noErr;
    } else {
        *importLibrary = nil;
        err = cfragNoLibraryErr;
    }
    return err;
}

static OSStatus LookupSymbol(CFMLateImportLookupProc lookup, void *refCon,
                                                        PEFLoaderInfoHeader *loaderSection,
                                                        UInt32 symbolIndex,
                                                        UInt32 *symbolValue)
    // This routine is used to look up a symbol during relocation.
    // "lookup" is a client callback and refCon is its argument.
    // Typically refCon is the CFM connection to the library that is
    // substituting for the weak linked library.  loaderSection
    // is a pointer to the loader section of the fragment to fix up.
    // symbolIndex is the index of the imported symbol in the loader section.
    // The routine sets the word pointed to by symbolValue to the
    // value of the symbol.
    //
    // The routine works by using symbolIndex to index into the imported
    // symbol table to find the offset of the symbol's name in the string
    // table.  It then looks up the symbol by calling the client's "lookup"
    // function and passes the resulting symbol address back in symbolValue.
{
    OSStatus                            err;
    UInt32                              *importSymbolTable;
    UInt32                              symbolStringOffset;
    Boolean                             symbolIsWeak;
    CFragSymbolClass            symbolClass;
    char                                *symbolStringAddress;
    Str255                              symbolString;

    MoreAssertQ(lookup != nil);
    MoreAssertQ(loaderSection != nil);
    MoreAssertQ(symbolIndex < loaderSection->totalImportedSymbolCount);
    MoreAssertQ(symbolValue != nil);

    // Find the base of the imported symbol table.

    importSymbolTable = (UInt32 *)(((char *)(loaderSection + 1)) + (loaderSection->importedLibraryCount * sizeof(PEFImportedLibrary)));

    // Grab the appropriate entry out of the table and
    // extract the information from that entry.

    symbolStringOffset = importSymbolTable[symbolIndex];
    symbolClass = PEFImportedSymbolClass(symbolStringOffset);
    symbolIsWeak = ((symbolClass & kPEFWeakImportSymMask) != 0);
    symbolClass = symbolClass & ~kPEFWeakImportSymMask;
    symbolStringOffset = PEFImportedSymbolNameOffset(symbolStringOffset);

    // Find the string for the symbol in the strings table and
    // extract it from the table into a Pascal string on the stack.

    symbolStringAddress = ((char *)loaderSection) + loaderSection->loaderStringsOffset + symbolStringOffset;
    symbolString[0] = strlen(symbolStringAddress);              // *** remove ANSI strlen
    BlockMoveData(symbolStringAddress, &symbolString[1], symbolString[0]);

    // Look up the symbol in substitute library.  If it fails, return
    // a 0 value and check whether the error is fatal (a strong linked
    // symbol) or benign (a weak linked symbol).

    err = lookup(symbolString, symbolClass, (void **) symbolValue, refCon);
    if (err != noErr) {
        *symbolValue = 0;
        if (symbolIsWeak) {
            err = noErr;
        }
    }
    return err;
}

// The EngineState structure encapsulates all of the persistent state
// of the CFM relocation engine virtual machine.  I originally defined
// this structure so I could pass the state around between routines
// that implement various virtual opcodes, however I later worked
// out that the relocation was sufficiently simple that I could put it
// in in one routine.  Still, I left the state in this structure in
// case I ever need to reverse that decision.  It's also a convenient
// instructional design.

struct EngineState {
    UInt32 currentReloc;                // Index of current relocation opcodes
    UInt32 terminatingReloc;            // Index of relocation opcodes which terminates relocation
    UInt32 *sectionBase;                // Start of the section
    UInt32 *relocAddress;               // Address within the section where the relocations are to be performed
    UInt32 importIndex;                         // Symbol index, which is used to access an imported symbol's address
    void  *sectionC;                            // Memory address of an instantiated section within the PEF container; this variable is used by relocation opcodes that relocate section addresses
    void  *sectionD;                            // Memory address of an instantiated section within the PEF container; this variable is used by relocation opcodes that relocate section addresses
};
typedef struct EngineState EngineState;

// Note:
// If I ever have to support the repeat opcodes, I'll probably
// have to add a repeat counter to EngineState.

static OSStatus InitEngineState(const FragToFixInfo *fragToFix,
                                                                UInt16 relocHeaderIndex,
                                                                EngineState *state)
    // This routine initialises the engine state suitably for
    // running the relocation opcodes for the section whose
    // index is relocHeaderIndex.  relocHeaderIndex is not a
    // a section number.  See the comment where it's used below
    // for details.  The routine basically fills out all the fields
    // in the EngineState structure as described by
    // "Mac OS Runtime Architectures".
{
    OSStatus err;
    PEFLoaderRelocationHeader *relocHeader;

    MoreAssertQ(fragToFix != nil);
    MoreAssertQ(state != nil);

    // This bit is tricky.  relocHeaderIndex is an index into the relocation
    // header table, starting at relocSectionCount (which is in the loader
    // section header) for the first relocated section and decrementing
    // down to 1 for the last relocated section.  I find the relocation
    // header by using relocHeaderIndex as a index backwards from the
    // start of the relocation opcodes (ie relocInstrOffset).  If you
    // look at the diagram of the layout of the container in
    // "PEFBinaryFormat.h", you'll see that the relocation opcodes
    // immediately follow the relocation headers.
    //
    // I did this because the alternative (starting at the loader
    // header and stepping past the import library table and the
    // import symbol table) was a pain.

    relocHeader = (PEFLoaderRelocationHeader *) (((char *) fragToFix->loaderSection) + fragToFix->loaderSection->relocInstrOffset - relocHeaderIndex * sizeof(PEFLoaderRelocationHeader));

    MoreAssertQ(relocHeader->reservedA == 0);                   // PEF spec says it must be; we check to try to catch bugs in calculation of relocHeader

    state->currentReloc = relocHeader->firstRelocOffset;
    state->terminatingReloc = relocHeader->firstRelocOffset + relocHeader->relocCount;
    state->sectionBase = (UInt32 *) GetSectionBaseAddress(fragToFix, relocHeader->sectionIndex);
    state->relocAddress = state->sectionBase;
    state->importIndex = 0;

    // From "Mac OS Runtime Architectures":
    //
    // The sectionC and sectionD variables actually contain the
    // memory address of an instantiated section minus the
    // default address for that section. The default address for a
    // section is contained in the defaultAddress field of the
    // section header. However, in almost all cases the default
    // address should be 0, so the simplified definition suffices.
    //
    // In the debug version, we drop into MacsBug if this weird case
    // ever executes because it's more likely we made a mistake than
    // we encountered a section with a default address.

    state->sectionC = GetSectionBaseAddress(fragToFix, 0);
    if (state->sectionC != nil) {
        #if MORE_DEBUG
            if (fragToFix->sectionHeaders[0].defaultAddress != 0) {
                DebugStr("\pInitEngineState: Executing weird case.");
            }
        #endif
        (char *) state->sectionC -= fragToFix->sectionHeaders[0].defaultAddress;
    }
    state->sectionD = GetSectionBaseAddress(fragToFix, 1);
    if (state->sectionD != nil) {
        #if MORE_DEBUG
            if (fragToFix->sectionHeaders[1].defaultAddress != 0) {
                DebugStr("\pInitEngineState: Executing weird case.");
            }
        #endif
        (char *) state->sectionD -= fragToFix->sectionHeaders[1].defaultAddress;
    }

    err = noErr;
    if (state->relocAddress == nil) {
        err = cfragFragmentUsageErr;
    }
    return err;
}

// kPEFRelocBasicOpcodes is a table that maps the top 7 bits of the opcode
// to a fundamental action.  It's contents are defined for me in "PEFBinaryFormat.h",
// which is really convenient.

static UInt8 kPEFRelocBasicOpcodes[kPEFRelocBasicOpcodeRange] = { PEFMaskedBasicOpcodes };

static OSStatus RunRelocationEngine(const FragToFixInfo *fragToFix,
                                                                                PEFImportedLibrary  *importLibrary,
                                                                                CFMLateImportLookupProc lookup, void *refCon)
    // This is where the rubber really hits the.  Given a fully
    // populated fragToFix structure, the import library description
    // of the weak imported library we're resolving, and a connection
    // to the library we're going to substitute it, re-execute the
    // relocation instructions (CFM has already executed them once)
    // but only *do* instructions (ie store the change to the data section)
    // that CFM skipped because the weak symbols were missing.
{
    OSStatus            err;
    EngineState         state;
    UInt16              sectionsLeftToRelocate;
    UInt32              totalRelocs;
    UInt16              *relocInstrTable;
    UInt16              opCode;

    MoreAssertQ(fragToFix != nil);
    MoreAssertQ(fragToFix->containerHeader.tag1 == kPEFTag1);
    MoreAssertQ(fragToFix->sectionHeaders != nil);
    MoreAssertQ(fragToFix->loaderSection != nil);
    MoreAssertQ(fragToFix->section0Base != nil);        // Technically, having a nil for these two is not a problem, ...
    MoreAssertQ(fragToFix->section1Base != nil);        // but in practise it a wildly deviant case and we should know about it.
    MoreAssertQ(importLibrary != nil);
    MoreAssertQ(lookup != nil);

    // Before entering the loop, work out some information in advance.

    // totalRelocs is only used for debugging, to make sure our
    // relocation PC (state.currentReloc) doesn't run wild.

    totalRelocs = (fragToFix->loaderSection->loaderStringsOffset - fragToFix->loaderSection->relocInstrOffset) / sizeof(UInt16);

    // relocInstrTable is the base address of the table of relocation
    // instructions in the fragment to fix.

    relocInstrTable = (UInt16 *)((char *) fragToFix->loaderSection + fragToFix->loaderSection->relocInstrOffset);

    // sectionsLeftToRelocate is the loop counter for the outer loop.

    MoreAssertQ(fragToFix->loaderSection->relocSectionCount <= 0x0FFFF);
    sectionsLeftToRelocate = fragToFix->loaderSection->relocSectionCount;

    // Now let's run the relocation engine.  We run it once per
    // section in the table.  Each time around, we init the engine
    // and then loop again, this time executing individual opcodes.
    // The opcode loop terminates when the relocation PC
    // (state.currentReloc) hits the final opcode (state.terminatingReloc).

    // Note:
    // One design decision I made was to totally re-init the engine state
    // for each section.  The CFM spec is unclear as to whether you're supposed
    // to totally re-init the engine state, or just re-init the section-specific
    // state (ie currentReloc, terminatingReloc, and relocAddress).  I hope this
    // is correct, but it's hard to test without having a fragment with multiple
    // relocated sections, which is difficult to create.

    // How do I decide which opcodes should be effective (ie make changes to
    // the section being relocated) and which opcodes should just be executed
    // for their side effects (ie updated state.relocAddress or state.importIndex)?
    // The answer is both simple and subtle.  Opcodes whose actions are dependent
    // on a symbol that was in the weak linked library are effective, those that
    // an independent of those symbols are not.  The only opcodes that use
    // symbolic values are kPEFRelocImportRun and kPEFRelocSmByImport, and
    // these are only if the symbol is in the weak linked library.
    // All other cases are executed for their side effects only.
    //
    // How do I determine if a symbol is in the weak linked library?
    // Well I know the symbol's index and I know the lower bound and count
    // of the symbols in the weak linked library, so I just do a simple
    // bounds test, ie
    //
    //   firstImportedSymbol <= importIndex < firstImportedSymbol + importedSymbolCount

    // From this code, it's relatively easy to see which relocation opcodes
    // aren't implemented.  If you ever encounter one, you'll find yourself
    // in MacsBug with a message telling you which opcode was found.  The
    // two big groups of opcodes I skipped were the large format opcodes
    // and the repeating opcodes.  I skipped them because:
    //
    // a) I haven't got a way to generate them in a PEF container that I can
    //    test against. Without that, there's no way I could be assured that
    //    the code worked.
    //
    // b) I'm lazy.

    err = noErr;
    while ( sectionsLeftToRelocate > 0 ) {
        err = InitEngineState(fragToFix, sectionsLeftToRelocate, &state);
        if (err != noErr) {
            goto leaveNow;
        }

        while ( state.currentReloc != state.terminatingReloc ) {

            MoreAssertQ( state.currentReloc < totalRelocs );

            opCode = relocInstrTable[state.currentReloc];
            switch ( PEFRelocBasicOpcode(opCode) ) {
                case kPEFRelocBySectDWithSkip:
                    {
                        UInt16 skipCount;
                        UInt16 relocCount;

                        skipCount = ((opCode >> 6) & 0x00FF);
                        relocCount = (opCode & 0x003F);
                        state.relocAddress += skipCount;
                        state.relocAddress += relocCount;
                    }
                    break;
                case kPEFRelocBySectC:
                case kPEFRelocBySectD:
                    {
                        UInt16 runLength;

                        runLength = (opCode & 0x01FF) + 1;
                        state.relocAddress += runLength;
                    }
                    break;
                case kPEFRelocTVector12:
                    {
                        UInt16 runLength;

                        runLength = (opCode & 0x01FF) + 1;
                        state.relocAddress += (runLength * 3);
                    }
                    break;
                case kPEFRelocTVector8:
                case kPEFRelocVTable8:
                    {
                        UInt16 runLength;

                        runLength = (opCode & 0x01FF) + 1;
                        state.relocAddress += (runLength * 2);
                    }
                    break;
                case kPEFRelocImportRun:
                    {
                        UInt32 symbolValue;
                        UInt16 runLength;

                        runLength = (opCode & 0x01FF) + 1;
                        while (runLength > 0) {
                            if ( state.importIndex >= importLibrary->firstImportedSymbol && state.importIndex < (importLibrary->firstImportedSymbol + importLibrary->importedSymbolCount) ) {
                                err = LookupSymbol(lookup, refCon, fragToFix->loaderSection, state.importIndex, &symbolValue);
                                if (err != noErr) {
                                    goto leaveNow;
                                }
                                *(state.relocAddress) += symbolValue;
                            }
                            state.importIndex += 1;
                            state.relocAddress += 1;
                            runLength -= 1;
                        }
                    }
                    break;
                case kPEFRelocSmByImport:
                    {
                        UInt32 symbolValue;
                        UInt32 index;

                        index = (opCode & 0x01FF);
                        if ( index >= importLibrary->firstImportedSymbol && index < (importLibrary->firstImportedSymbol + importLibrary->importedSymbolCount) ) {
                            err = LookupSymbol(lookup, refCon, fragToFix->loaderSection, index, &symbolValue);
                            if (err != noErr) {
                                goto leaveNow;
                            }
                            *(state.relocAddress) += symbolValue;
                        }
                        state.importIndex = index + 1;
                        state.relocAddress += 1;
                    }
                    break;
                case kPEFRelocSmSetSectC:
                    {
                        UInt32 index;

                        index = (opCode & 0x01FF);
                        state.sectionC = GetSectionBaseAddress(fragToFix, index);
                        MoreAssertQ(state.sectionC != nil);
                    }
                    break;
                case kPEFRelocSmSetSectD:
                    {
                        UInt32 index;

                        index = (opCode & 0x01FF);
                        state.sectionD = GetSectionBaseAddress(fragToFix, index);
                        MoreAssertQ(state.sectionD != nil);
                    }
                    break;
                case kPEFRelocSmBySection:
                    state.relocAddress += 1;
                    break;
                case kPEFRelocIncrPosition:
                    {
                        UInt16 offset;

                        offset = (opCode & 0x0FFF) + 1;
                        ((char *) state.relocAddress) += offset;
                    }
                    break;
                case kPEFRelocSmRepeat:
                    #if MORE_DEBUG
                        DebugStr("\pRunRelocationEngine: kPEFRelocSmRepeat not yet implemented");
                    #endif
                    err = unimpErr;
                    goto leaveNow;
                    break;
                case kPEFRelocSetPosition:
                    {
                        UInt32 offset;

                        // Lot's of folks have tried various interpretations of the description of
                        // this opCode in "Mac OS Runtime Architectures" (which states "This instruction
                        // sets relocAddress to the address of the section offset offset."  *smile*).
                        // I eventually dug into the CFM source code to find my interpretation, which
                        // I believe is correct.  The key point is tht the offset is relative to
                        // the start of the section for which these relocations are being performed.

                        // Skip to next reloc word, which is the second chunk of the offset.

                        state.currentReloc += 1;

                        // Extract offset based on the most significant 10 bits in opCode and
                        // the next significant 16 bits in the next reloc word.

                        offset = PEFRelocSetPosFullOffset(opCode, relocInstrTable[state.currentReloc]);

                        state.relocAddress = (UInt32 *) ( ((char *) state.sectionBase) + offset);
                    }
                    break;
                case kPEFRelocLgByImport:
                    {
                        UInt32 symbolValue;
                        UInt32 index;

                        // Get the 26 bit symbol index from the current and next reloc words.

                        state.currentReloc += 1;
                        index = PEFRelocLgByImportFullIndex(opCode, relocInstrTable[state.currentReloc]);

                        if ( index >= importLibrary->firstImportedSymbol && index < (importLibrary->firstImportedSymbol + importLibrary->importedSymbolCount) ) {
                            err = LookupSymbol(lookup, refCon, fragToFix->loaderSection, index, &symbolValue);
                            if (err != noErr) {
                                goto leaveNow;
                            }
                            *(state.relocAddress) += symbolValue;
                        }
                        state.importIndex = index + 1;
                        state.relocAddress += 1;
                    }
                    break;
                case kPEFRelocLgRepeat:
                    #if MORE_DEBUG
                        DebugStr("\pRunRelocationEngine: kPEFRelocLgRepeat not yet implemented");
                    #endif
                    err = unimpErr;
                    goto leaveNow;
                    break;
                case kPEFRelocLgSetOrBySection:
                    #if MORE_DEBUG
                        DebugStr("\pRunRelocationEngine: kPEFRelocLgSetOrBySection not yet implemented");
                    #endif
                    err = unimpErr;
                    goto leaveNow;
                    break;
                case kPEFRelocUndefinedOpcode:
                    err = cfragFragmentCorruptErr;
                    goto leaveNow;
                    break;
                default:
                    MoreAssertQ(false);
                    err = cfragFragmentCorruptErr;
                    goto leaveNow;
                    break;
            }
            state.currentReloc += 1;
        }

        sectionsLeftToRelocate -= 1;
    }

leaveNow:
    return err;
}

extern pascal OSStatus CFMLateImportCore(const CFragSystem7DiskFlatLocator *fragToFixLocator,
                                                                                CFragConnectionID fragToFixConnID,
                                                                                CFragInitFunction fragToFixInitRoutine,
                                                                                ConstStr255Param weakLinkedLibraryName,
                                                                                CFMLateImportLookupProc lookup,
                                                                                void *refCon)
    // See comments in interface part.
{
    OSStatus err;
    OSStatus junk;
    FragToFixInfo fragToFix;
    PEFImportedLibrary *importLibrary;
    char weakLinkedLibraryNameCString[256];

    MoreAssertQ(fragToFixLocator != nil);
    MoreAssertQ(fragToFixConnID != nil);
    MoreAssertQ(fragToFixInitRoutine != nil);
    MoreAssertQ(weakLinkedLibraryName != nil);
    MoreAssertQ(lookup != nil);

    // Fill out the bits of fragToFix which are passed in
    // by the client.

    MoreBlockZero(&fragToFix, sizeof(fragToFix));
    fragToFix.locator = *fragToFixLocator;
    fragToFix.connID  = fragToFixConnID;
    fragToFix.initRoutine = fragToFixInitRoutine;

    // Make a C string from weakLinkedLibraryName.

    BlockMoveData(weakLinkedLibraryName + 1, weakLinkedLibraryNameCString, weakLinkedLibraryName[0]);
    weakLinkedLibraryNameCString[weakLinkedLibraryName[0]] = 0;

    // Get the basic information from the fragment.
    // Fills out the containerHeader, sectionHeaders, loaderSection and fileRef fields
    // of fragToFix.

    err = ReadContainerBasics(&fragToFix);

    // Set up the base address fields in fragToFix (ie section0Base and section1Base)
    // by looking up our init routine (fragToFix.initRoutine) and subtracting
    // away the section offsets (which we get from the disk copy of the section)
    // to derive the bases of the sections themselves.

    if (err == noErr) {
        err = SetupSectionBaseAddresses(&fragToFix);
    }

    // Look inside the loader section for the import library description
    // of weakLinkedLibraryName.  We need this to know the range of symbol
    // indexes we're going to fix up.

    if (err == noErr) {
        err = FindImportLibrary(fragToFix.loaderSection, weakLinkedLibraryNameCString, &importLibrary);
    }

    // Do a quick check to ensure that the library was actually imported weak.
    // If it wasn't, it doesn't make much sense to resolve its weak imports
    // later on.  Resolving them again is likely to be bad.

    if (err == noErr) {
        if ((importLibrary->options & kPEFWeakImportLibMask) == 0) {
            err = cfragFragmentUsageErr;
        }
    }

    // Now run the main relocation engine.

    if (err == noErr) {
        err = RunRelocationEngine(&fragToFix, importLibrary, lookup, refCon);
    }

    // Clean up.

    if (fragToFix.disposeSectionPointers) {
        if (fragToFix.fileRef != 0) {
            junk = FSClose(fragToFix.fileRef);
            MoreAssertQ(junk == noErr);
        }
        if (fragToFix.loaderSection != nil) {
            DisposePtr( (Ptr) fragToFix.loaderSection);
            MoreAssertQ(MemError() == noErr);
        }
        if (fragToFix.sectionHeaders != nil) {
            DisposePtr( (Ptr) fragToFix.sectionHeaders);
            MoreAssertQ(MemError() == noErr);
        }
    }
    return err;
}

static pascal OSStatus FragmentLookup(ConstStr255Param symName, CFragSymbolClass symClass,
                                                                        void **symAddr, void *refCon)
    // This is the CFMLateImportLookupProc callback used when
    // late importing from a CFM shared library.
{
    OSStatus err;
    CFragConnectionID connIDToImport;
    CFragSymbolClass  foundSymClass;

    MoreAssertQ(symName != nil);
    MoreAssertQ(symAddr != nil);
    MoreAssertQ(refCon  != nil);

    connIDToImport = (CFragConnectionID) refCon;

    // Shame there's no way to validate that connIDToImport is valid.

    err = FindSymbol(connIDToImport, symName, (Ptr *) symAddr, &foundSymClass);
    if (err == noErr) {
        // If the symbol isn't of the right class, we act like we didn't
        // find it, but also assert in the debug build because weird things
        // are afoot.
        if (foundSymClass != symClass) {
            MoreAssertQ(false);
            *symAddr = nil;
            err = cfragNoSymbolErr;
        }
    }
    return err;
}

extern pascal OSStatus CFMLateImportLibrary(const CFragSystem7DiskFlatLocator *fragToFixLocator,
                                                                                CFragConnectionID fragToFixConnID,
                                                                                CFragInitFunction fragToFixInitRoutine,
                                                                                ConstStr255Param weakLinkedLibraryName,
                                                                                CFragConnectionID connIDToImport)
    // See comments in interface part.
{
    MoreAssertQ(connIDToImport != nil);
    return CFMLateImportCore(fragToFixLocator, fragToFixConnID, fragToFixInitRoutine,
                                                                            weakLinkedLibraryName, FragmentLookup, connIDToImport);
}

static pascal OSStatus BundleLookup(ConstStr255Param symName, CFragSymbolClass symClass,
                                                                        void **symAddr, void *refCon)
    // This is the CFMLateImportLookupProc callback used when
    // late importing from a CFBundle.
{
    OSStatus            err;
    CFBundleRef bundleToImport;
    CFStringRef symNameStr;

    MoreAssertQ(symName != nil);
    MoreAssertQ(symAddr != nil);
    MoreAssertQ(refCon  != nil);

    symNameStr = nil;

    bundleToImport = (CFBundleRef) refCon;

    // Shame there's no way to validate that bundleToImport is really a bundle.

    // We can only find function pointers because CFBundleGetFunctionPointerForName
    // only works for function pointers.  So if the client is asking for something
    // other than a function pointer (ie TVector symbol) then we don't even true.
    // Also assert in the debug build because this shows a certain lack of
    // understanding on the part of the client.
    //
    // CF is being revise to support accessing data symbols using a new API
    // (currently this is available to Apple internal developers as
    // CFBundleGetDataPointerForName).  When the new API is available in a
    // public header file I should revise this code to lift this restriction.

    err = noErr;
    if (symClass != kTVectorCFragSymbol) {
        MoreAssertQ(false);
        err = cfragNoSymbolErr;
    }
    if (err == noErr) {
        symNameStr = CFStringCreateWithPascalString(kCFAllocatorSystemDefault,
                                                                                                symName, kCFStringEncodingMacRoman);
        if (symNameStr == nil) {
            err = coreFoundationUnknownErr;
        }
    }
    if (err == noErr) {
        *symAddr = CFBundleGetFunctionPointerForName(bundleToImport, symNameStr);
        if (*symAddr == nil) {
            err = cfragNoSymbolErr;
        }
    }
    if (symNameStr != nil) {
        CFRelease(symNameStr);
    }
    return err;
}

extern pascal OSStatus CFMLateImportBundle(const CFragSystem7DiskFlatLocator *fragToFixLocator,
                                                                                CFragConnectionID fragToFixConnID,
                                                                                CFragInitFunction fragToFixInitRoutine,
                                                                                ConstStr255Param weakLinkedLibraryName,
                                                                                CFBundleRef bundleToImport)
    // See comments in interface part.
{
    MoreAssertQ(bundleToImport != nil);
    return CFMLateImportCore(fragToFixLocator, fragToFixConnID, fragToFixInitRoutine,
                                                                            weakLinkedLibraryName, BundleLookup, bundleToImport);
}
