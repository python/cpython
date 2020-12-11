/*
 * tclEncoding.c --
 *
 *	Contains the implementation of the encoding conversion package.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

typedef size_t (LengthProc)(const char *src);

/*
 * The following data structure represents an encoding, which describes how to
 * convert between various character sets and UTF-8.
 */

typedef struct Encoding {
    char *name;			/* Name of encoding. Malloced because (1) hash
				 * table entry that owns this encoding may be
				 * freed prior to this encoding being freed,
				 * (2) string passed in the Tcl_EncodingType
				 * structure may not be persistent. */
    Tcl_EncodingConvertProc *toUtfProc;
				/* Function to convert from external encoding
				 * into UTF-8. */
    Tcl_EncodingConvertProc *fromUtfProc;
				/* Function to convert from UTF-8 into
				 * external encoding. */
    Tcl_EncodingFreeProc *freeProc;
				/* If non-NULL, function to call when this
				 * encoding is deleted. */
    int nullSize;		/* Number of 0x00 bytes that signify
				 * end-of-string in this encoding. This number
				 * is used to determine the source string
				 * length when the srcLen argument is
				 * negative. This number can be 1 or 2. */
    ClientData clientData;	/* Arbitrary value associated with encoding
				 * type. Passed to conversion functions. */
    LengthProc *lengthProc;	/* Function to compute length of
				 * null-terminated strings in this encoding.
				 * If nullSize is 1, this is strlen; if
				 * nullSize is 2, this is a function that
				 * returns the number of bytes in a 0x0000
				 * terminated string. */
    int refCount;		/* Number of uses of this structure. */
    Tcl_HashEntry *hPtr;	/* Hash table entry that owns this encoding. */
} Encoding;

/*
 * The following structure is the clientData for a dynamically-loaded,
 * table-driven encoding created by LoadTableEncoding(). It maps between
 * Unicode and a single-byte, double-byte, or multibyte (1 or 2 bytes only)
 * encoding.
 */

typedef struct TableEncodingData {
    int fallback;		/* Character (in this encoding) to substitute
				 * when this encoding cannot represent a UTF-8
				 * character. */
    char prefixBytes[256];	/* If a byte in the input stream is a lead
				 * byte for a 2-byte sequence, the
				 * corresponding entry in this array is 1,
				 * otherwise it is 0. */
    unsigned short **toUnicode;	/* Two dimensional sparse matrix to map
				 * characters from the encoding to Unicode.
				 * Each element of the toUnicode array points
				 * to an array of 256 shorts. If there is no
				 * corresponding character in Unicode, the
				 * value in the matrix is 0x0000.
				 * malloc'd. */
    unsigned short **fromUnicode;
				/* Two dimensional sparse matrix to map
				 * characters from Unicode to the encoding.
				 * Each element of the fromUnicode array
				 * points to an array of 256 shorts. If there
				 * is no corresponding character the encoding,
				 * the value in the matrix is 0x0000.
				 * malloc'd. */
} TableEncodingData;

/*
 * The following structures is the clientData for a dynamically-loaded,
 * escape-driven encoding that is itself comprised of other simpler encodings.
 * An example is "iso-2022-jp", which uses escape sequences to switch between
 * ascii, jis0208, jis0212, gb2312, and ksc5601. Note that "escape-driven"
 * does not necessarily mean that the ESCAPE character is the character used
 * for switching character sets.
 */

typedef struct EscapeSubTable {
    unsigned sequenceLen;	/* Length of following string. */
    char sequence[16];		/* Escape code that marks this encoding. */
    char name[32];		/* Name for encoding. */
    Encoding *encodingPtr;	/* Encoding loaded using above name, or NULL
				 * if this sub-encoding has not been needed
				 * yet. */
} EscapeSubTable;

typedef struct EscapeEncodingData {
    int fallback;		/* Character (in this encoding) to substitute
				 * when this encoding cannot represent a UTF-8
				 * character. */
    unsigned initLen;		/* Length of following string. */
    char init[16];		/* String to emit or expect before first char
				 * in conversion. */
    unsigned finalLen;		/* Length of following string. */
    char final[16];		/* String to emit or expect after last char in
				 * conversion. */
    char prefixBytes[256];	/* If a byte in the input stream is the first
				 * character of one of the escape sequences in
				 * the following array, the corresponding
				 * entry in this array is 1, otherwise it is
				 * 0. */
    int numSubTables;		/* Length of following array. */
    EscapeSubTable subTables[1];/* Information about each EscapeSubTable used
				 * by this encoding type. The actual size will
				 * be as large as necessary to hold all
				 * EscapeSubTables. */
} EscapeEncodingData;

/*
 * Constants used when loading an encoding file to identify the type of the
 * file.
 */

#define ENCODING_SINGLEBYTE	0
#define ENCODING_DOUBLEBYTE	1
#define ENCODING_MULTIBYTE	2
#define ENCODING_ESCAPE		3

/*
 * A list of directories in which Tcl should look for *.enc files. This list
 * is shared by all threads. Access is governed by a mutex lock.
 */

static TclInitProcessGlobalValueProc InitializeEncodingSearchPath;
static ProcessGlobalValue encodingSearchPath = {
    0, 0, NULL, NULL, InitializeEncodingSearchPath, NULL, NULL
};

/*
 * A map from encoding names to the directories in which their data files have
 * been seen. The string value of the map is shared by all threads. Access to
 * the shared string is governed by a mutex lock.
 */

static ProcessGlobalValue encodingFileMap = {
    0, 0, NULL, NULL, NULL, NULL, NULL
};

/*
 * A list of directories making up the "library path". Historically this
 * search path has served many uses, but the only one remaining is a base for
 * the encodingSearchPath above. If the application does not explicitly set
 * the encodingSearchPath, then it will be initialized by appending /encoding
 * to each directory in this "libraryPath".
 */

static ProcessGlobalValue libraryPath = {
    0, 0, NULL, NULL, TclpInitLibraryPath, NULL, NULL
};

static int encodingsInitialized = 0;

/*
 * Hash table that keeps track of all loaded Encodings. Keys are the string
 * names that represent the encoding, values are (Encoding *).
 */

static Tcl_HashTable encodingTable;
TCL_DECLARE_MUTEX(encodingMutex)

/*
 * The following are used to hold the default and current system encodings.
 * If NULL is passed to one of the conversion routines, the current setting of
 * the system encoding will be used to perform the conversion.
 */

static Tcl_Encoding defaultEncoding = NULL;
static Tcl_Encoding systemEncoding = NULL;
Tcl_Encoding tclIdentityEncoding = NULL;

/*
 * The following variable is used in the sparse matrix code for a
 * TableEncoding to represent a page in the table that has no entries.
 */

static unsigned short emptyPage[256];

/*
 * Functions used only in this module.
 */

static int		BinaryProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static void		DupEncodingIntRep(Tcl_Obj *srcPtr, Tcl_Obj *dupPtr);
static void		EscapeFreeProc(ClientData clientData);
static int		EscapeFromUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		EscapeToUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static void		FillEncodingFileMap(void);
static void		FreeEncoding(Tcl_Encoding encoding);
static void		FreeEncodingIntRep(Tcl_Obj *objPtr);
static Encoding *	GetTableEncoding(EscapeEncodingData *dataPtr,
			    int state);
static Tcl_Encoding	LoadEncodingFile(Tcl_Interp *interp, const char *name);
static Tcl_Encoding	LoadTableEncoding(const char *name, int type,
			    Tcl_Channel chan);
static Tcl_Encoding	LoadEscapeEncoding(const char *name, Tcl_Channel chan);
static Tcl_Channel	OpenEncodingFileChannel(Tcl_Interp *interp,
			    const char *name);
static void		TableFreeProc(ClientData clientData);
static int		TableFromUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		TableToUtfProc(ClientData clientData, const char *src,
			    int srcLen, int flags, Tcl_EncodingState *statePtr,
			    char *dst, int dstLen, int *srcReadPtr,
			    int *dstWrotePtr, int *dstCharsPtr);
static size_t		unilen(const char *src);
static int		UnicodeToUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		UtfToUnicodeProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		UtfToUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr, int pureNullMode);
static int		UtfIntToUtfExtProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		UtfExtToUtfIntProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		Iso88591FromUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);
static int		Iso88591ToUtfProc(ClientData clientData,
			    const char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst,
			    int dstLen, int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr);

/*
 * A Tcl_ObjType for holding a cached Tcl_Encoding in the twoPtrValue.ptr1 field
 * of the intrep. This should help the lifetime of encodings be more useful.
 * See concerns raised in [Bug 1077262].
 */

static const Tcl_ObjType encodingType = {
    "encoding", FreeEncodingIntRep, DupEncodingIntRep, NULL, NULL
};

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEncodingFromObj --
 *
 *	Writes to (*encodingPtr) the Tcl_Encoding value of (*objPtr), if
 *	possible, and returns TCL_OK. If no such encoding exists, TCL_ERROR is
 *	returned, and if interp is non-NULL, an error message is written
 *	there.
 *
 * Results:
 *	Standard Tcl return code.
 *
 * Side effects:
 *	Caches the Tcl_Encoding value as the internal rep of (*objPtr).
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEncodingFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Tcl_Encoding *encodingPtr)
{
    const char *name = TclGetString(objPtr);

    if (objPtr->typePtr != &encodingType) {
	Tcl_Encoding encoding = Tcl_GetEncoding(interp, name);

	if (encoding == NULL) {
	    return TCL_ERROR;
	}
	TclFreeIntRep(objPtr);
	objPtr->internalRep.twoPtrValue.ptr1 = encoding;
	objPtr->typePtr = &encodingType;
    }
    *encodingPtr = Tcl_GetEncoding(NULL, name);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeEncodingIntRep --
 *
 *	The Tcl_FreeInternalRepProc for the "encoding" Tcl_ObjType.
 *
 *----------------------------------------------------------------------
 */

static void
FreeEncodingIntRep(
    Tcl_Obj *objPtr)
{
    Tcl_FreeEncoding(objPtr->internalRep.twoPtrValue.ptr1);
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupEncodingIntRep --
 *
 *	The Tcl_DupInternalRepProc for the "encoding" Tcl_ObjType.
 *
 *----------------------------------------------------------------------
 */

static void
DupEncodingIntRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    dupPtr->internalRep.twoPtrValue.ptr1 = Tcl_GetEncoding(NULL, srcPtr->bytes);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEncodingSearchPath --
 *
 *	Keeps the per-thread copy of the encoding search path current with
 *	changes to the global copy.
 *
 * Results:
 *	Returns a "list" (Tcl_Obj *) that contains the encoding search path.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetEncodingSearchPath(void)
{
    return TclGetProcessGlobalValue(&encodingSearchPath);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetEncodingSearchPath --
 *
 *	Keeps the per-thread copy of the encoding search path current with
 *	changes to the global copy.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetEncodingSearchPath(
    Tcl_Obj *searchPath)
{
    int dummy;

    if (TCL_ERROR == Tcl_ListObjLength(NULL, searchPath, &dummy)) {
	return TCL_ERROR;
    }
    TclSetProcessGlobalValue(&encodingSearchPath, searchPath, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetLibraryPath --
 *
 *	Keeps the per-thread copy of the library path current with changes to
 *	the global copy.
 *
 * Results:
 *	Returns a "list" (Tcl_Obj *) that contains the library path.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclGetLibraryPath(void)
{
    return TclGetProcessGlobalValue(&libraryPath);
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetLibraryPath --
 *
 *	Keeps the per-thread copy of the library path current with changes to
 *	the global copy.
 *
 *	NOTE: this routine returns void, so there's no way to report the error
 *	that searchPath is not a valid list. In that case, this routine will
 *	silently do nothing.
 *
 *----------------------------------------------------------------------
 */

void
TclSetLibraryPath(
    Tcl_Obj *path)
{
    int dummy;

    if (TCL_ERROR == Tcl_ListObjLength(NULL, path, &dummy)) {
	return;
    }
    TclSetProcessGlobalValue(&libraryPath, path, NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * FillEncodingFileMap --
 *
 *	Called to bring the encoding file map in sync with the current value
 *	of the encoding search path.
 *
 *	Scan the directories on the encoding search path, find the *.enc
 *	files, and store the found pathnames in a map associated with the
 *	encoding name.
 *
 *	In particular, if $dir is on the encoding search path, and the file
 *	$dir/foo.enc is found, then store a "foo" -> $dir entry in the map.
 *	Later, any need for the "foo" encoding will quickly * be able to
 *	construct the $dir/foo.enc pathname for reading the encoding data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Entries are added to the encoding file map.
 *
 *---------------------------------------------------------------------------
 */

static void
FillEncodingFileMap(void)
{
    int i, numDirs = 0;
    Tcl_Obj *map, *searchPath;

    searchPath = Tcl_GetEncodingSearchPath();
    Tcl_IncrRefCount(searchPath);
    Tcl_ListObjLength(NULL, searchPath, &numDirs);
    map = Tcl_NewDictObj();
    Tcl_IncrRefCount(map);

    for (i = numDirs-1; i >= 0; i--) {
	/*
	 * Iterate backwards through the search path so as we overwrite
	 * entries found, we favor files earlier on the search path.
	 */

	int j, numFiles;
	Tcl_Obj *directory, *matchFileList = Tcl_NewObj();
	Tcl_Obj **filev;
	Tcl_GlobTypeData readableFiles = {
	    TCL_GLOB_TYPE_FILE, TCL_GLOB_PERM_R, NULL, NULL
	};

	Tcl_ListObjIndex(NULL, searchPath, i, &directory);
	Tcl_IncrRefCount(directory);
	Tcl_IncrRefCount(matchFileList);
	Tcl_FSMatchInDirectory(NULL, matchFileList, directory, "*.enc",
		&readableFiles);

	Tcl_ListObjGetElements(NULL, matchFileList, &numFiles, &filev);
	for (j=0; j<numFiles; j++) {
	    Tcl_Obj *encodingName, *fileObj;

	    fileObj = TclPathPart(NULL, filev[j], TCL_PATH_TAIL);
	    encodingName = TclPathPart(NULL, fileObj, TCL_PATH_ROOT);
	    Tcl_DictObjPut(NULL, map, encodingName, directory);
	    Tcl_DecrRefCount(fileObj);
	    Tcl_DecrRefCount(encodingName);
	}
	Tcl_DecrRefCount(matchFileList);
	Tcl_DecrRefCount(directory);
    }
    Tcl_DecrRefCount(searchPath);
    TclSetProcessGlobalValue(&encodingFileMap, map, NULL);
    Tcl_DecrRefCount(map);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclInitEncodingSubsystem --
 *
 *	Initialize all resources used by this subsystem on a per-process
 *	basis.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the memory, object, and IO subsystems.
 *
 *---------------------------------------------------------------------------
 */

void
TclInitEncodingSubsystem(void)
{
    Tcl_EncodingType type;
    TableEncodingData *dataPtr;
    unsigned size;
    unsigned short i;

    if (encodingsInitialized) {
	return;
    }

    Tcl_MutexLock(&encodingMutex);
    Tcl_InitHashTable(&encodingTable, TCL_STRING_KEYS);
    Tcl_MutexUnlock(&encodingMutex);

    /*
     * Create a few initial encodings. Note that the UTF-8 to UTF-8
     * translation is not a no-op, because it will turn a stream of improperly
     * formed UTF-8 into a properly formed stream.
     */

    type.encodingName	= "identity";
    type.toUtfProc	= BinaryProc;
    type.fromUtfProc	= BinaryProc;
    type.freeProc	= NULL;
    type.nullSize	= 1;
    type.clientData	= NULL;
    tclIdentityEncoding = Tcl_CreateEncoding(&type);

    type.encodingName	= "utf-8";
    type.toUtfProc	= UtfExtToUtfIntProc;
    type.fromUtfProc	= UtfIntToUtfExtProc;
    type.freeProc	= NULL;
    type.nullSize	= 1;
    type.clientData	= NULL;
    Tcl_CreateEncoding(&type);

    type.encodingName   = "unicode";
    type.toUtfProc	= UnicodeToUtfProc;
    type.fromUtfProc    = UtfToUnicodeProc;
    type.freeProc	= NULL;
    type.nullSize	= 2;
    type.clientData	= NULL;
    Tcl_CreateEncoding(&type);

    /*
     * Need the iso8859-1 encoding in order to process binary data, so force
     * it to always be embedded. Note that this encoding *must* be a proper
     * table encoding or some of the escape encodings crash! Hence the ugly
     * code to duplicate the structure of a table encoding here.
     */

    dataPtr = ckalloc(sizeof(TableEncodingData));
    memset(dataPtr, 0, sizeof(TableEncodingData));
    dataPtr->fallback = '?';

    size = 256*(sizeof(unsigned short *) + sizeof(unsigned short));
    dataPtr->toUnicode = ckalloc(size);
    memset(dataPtr->toUnicode, 0, size);
    dataPtr->fromUnicode = ckalloc(size);
    memset(dataPtr->fromUnicode, 0, size);

    dataPtr->toUnicode[0] = (unsigned short *) (dataPtr->toUnicode + 256);
    dataPtr->fromUnicode[0] = (unsigned short *) (dataPtr->fromUnicode + 256);
    for (i=1 ; i<256 ; i++) {
	dataPtr->toUnicode[i] = emptyPage;
	dataPtr->fromUnicode[i] = emptyPage;
    }

    for (i=0 ; i<256 ; i++) {
	dataPtr->toUnicode[0][i] = i;
	dataPtr->fromUnicode[0][i] = i;
    }

    type.encodingName	= "iso8859-1";
    type.toUtfProc	= Iso88591ToUtfProc;
    type.fromUtfProc	= Iso88591FromUtfProc;
    type.freeProc	= TableFreeProc;
    type.nullSize	= 1;
    type.clientData	= dataPtr;
    defaultEncoding	= Tcl_CreateEncoding(&type);
    systemEncoding	= Tcl_GetEncoding(NULL, type.encodingName);

    encodingsInitialized = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeEncodingSubsystem --
 *
 *	Release the state associated with the encoding subsystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees all of the encodings.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeEncodingSubsystem(void)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;

    Tcl_MutexLock(&encodingMutex);
    encodingsInitialized = 0;
    FreeEncoding(systemEncoding);
    systemEncoding = NULL;
    defaultEncoding = NULL;
    FreeEncoding(tclIdentityEncoding);
    tclIdentityEncoding = NULL;

    hPtr = Tcl_FirstHashEntry(&encodingTable, &search);
    while (hPtr != NULL) {
	/*
	 * Call FreeEncoding instead of doing it directly to handle refcounts
	 * like escape encodings use. [Bug 524674] Make sure to call
	 * Tcl_FirstHashEntry repeatedly so that all encodings are eventually
	 * cleaned up.
	 */

	FreeEncoding(Tcl_GetHashValue(hPtr));
	hPtr = Tcl_FirstHashEntry(&encodingTable, &search);
    }

    Tcl_DeleteHashTable(&encodingTable);
    Tcl_MutexUnlock(&encodingMutex);
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetDefaultEncodingDir --
 *
 *	Legacy public interface to retrieve first directory in the encoding
 *	searchPath.
 *
 * Results:
 *	The directory pathname, as a string, or NULL for an empty encoding
 *	search path.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

const char *
Tcl_GetDefaultEncodingDir(void)
{
    int numDirs;
    Tcl_Obj *first, *searchPath = Tcl_GetEncodingSearchPath();

    Tcl_ListObjLength(NULL, searchPath, &numDirs);
    if (numDirs == 0) {
	return NULL;
    }
    Tcl_ListObjIndex(NULL, searchPath, 0, &first);

    return TclGetString(first);
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_SetDefaultEncodingDir --
 *
 *	Legacy public interface to set the first directory in the encoding
 *	search path.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the encoding search path.
 *
 *-------------------------------------------------------------------------
 */

void
Tcl_SetDefaultEncodingDir(
    const char *path)
{
    Tcl_Obj *searchPath = Tcl_GetEncodingSearchPath();
    Tcl_Obj *directory = Tcl_NewStringObj(path, -1);

    searchPath = Tcl_DuplicateObj(searchPath);
    Tcl_ListObjReplace(NULL, searchPath, 0, 0, 1, &directory);
    Tcl_SetEncodingSearchPath(searchPath);
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetEncoding --
 *
 *	Given the name of a encoding, find the corresponding Tcl_Encoding
 *	token. If the encoding did not already exist, Tcl attempts to
 *	dynamically load an encoding by that name.
 *
 * Results:
 *	Returns a token that represents the encoding. If the name didn't refer
 *	to any known or loadable encoding, NULL is returned. If NULL was
 *	returned, an error message is left in interp's result object, unless
 *	interp was NULL.
 *
 * Side effects:
 *	The new encoding type is entered into a table visible to all
 *	interpreters, keyed off the encoding's name. For each call to this
 *	function, there should eventually be a call to Tcl_FreeEncoding, so
 *	that the database can be cleaned up when encodings aren't needed
 *	anymore.
 *
 *-------------------------------------------------------------------------
 */

Tcl_Encoding
Tcl_GetEncoding(
    Tcl_Interp *interp,		/* Interp for error reporting, if not NULL. */
    const char *name)		/* The name of the desired encoding. */
{
    Tcl_HashEntry *hPtr;
    Encoding *encodingPtr;

    Tcl_MutexLock(&encodingMutex);
    if (name == NULL) {
	encodingPtr = (Encoding *) systemEncoding;
	encodingPtr->refCount++;
	Tcl_MutexUnlock(&encodingMutex);
	return systemEncoding;
    }

    hPtr = Tcl_FindHashEntry(&encodingTable, name);
    if (hPtr != NULL) {
	encodingPtr = Tcl_GetHashValue(hPtr);
	encodingPtr->refCount++;
	Tcl_MutexUnlock(&encodingMutex);
	return (Tcl_Encoding) encodingPtr;
    }
    Tcl_MutexUnlock(&encodingMutex);

    return LoadEncodingFile(interp, name);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FreeEncoding --
 *
 *	This function is called to release an encoding allocated by
 *	Tcl_CreateEncoding() or Tcl_GetEncoding().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the encoding is decremented and
 *	the encoding may be deleted if nothing is using it anymore.
 *
 *---------------------------------------------------------------------------
 */

void
Tcl_FreeEncoding(
    Tcl_Encoding encoding)
{
    Tcl_MutexLock(&encodingMutex);
    FreeEncoding(encoding);
    Tcl_MutexUnlock(&encodingMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeEncoding --
 *
 *	This function is called to release an encoding by functions that
 *	already have the encodingMutex.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the encoding is decremented and
 *	the encoding may be deleted if nothing is using it anymore.
 *
 *----------------------------------------------------------------------
 */

static void
FreeEncoding(
    Tcl_Encoding encoding)
{
    Encoding *encodingPtr = (Encoding *) encoding;

    if (encodingPtr == NULL) {
	return;
    }
    if (encodingPtr->refCount<=0) {
	Tcl_Panic("FreeEncoding: refcount problem !!!");
    }
    encodingPtr->refCount--;
    if (encodingPtr->refCount == 0) {
	if (encodingPtr->freeProc != NULL) {
	    encodingPtr->freeProc(encodingPtr->clientData);
	}
	if (encodingPtr->hPtr != NULL) {
	    Tcl_DeleteHashEntry(encodingPtr->hPtr);
	}
	ckfree(encodingPtr->name);
	ckfree(encodingPtr);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetEncodingName --
 *
 *	Given an encoding, return the name that was used to constuct the
 *	encoding.
 *
 * Results:
 *	The name of the encoding.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tcl_GetEncodingName(
    Tcl_Encoding encoding)	/* The encoding whose name to fetch. */
{
    if (encoding == NULL) {
	encoding = systemEncoding;
    }

    return ((Encoding *) encoding)->name;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetEncodingNames --
 *
 *	Get the list of all known encodings, including the ones stored as
 *	files on disk in the encoding path.
 *
 * Results:
 *	Modifies interp's result object to hold a list of all the available
 *	encodings.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

void
Tcl_GetEncodingNames(
    Tcl_Interp *interp)		/* Interp to hold result. */
{
    Tcl_HashTable table;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *map, *name, *result = Tcl_NewObj();
    Tcl_DictSearch mapSearch;
    int dummy, done = 0;

    Tcl_InitObjHashTable(&table);

    /*
     * Copy encoding names from loaded encoding table to table.
     */

    Tcl_MutexLock(&encodingMutex);
    for (hPtr = Tcl_FirstHashEntry(&encodingTable, &search); hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&search)) {
	Encoding *encodingPtr = Tcl_GetHashValue(hPtr);

	Tcl_CreateHashEntry(&table,
		Tcl_NewStringObj(encodingPtr->name, -1), &dummy);
    }
    Tcl_MutexUnlock(&encodingMutex);

    FillEncodingFileMap();
    map = TclGetProcessGlobalValue(&encodingFileMap);

    /*
     * Copy encoding names from encoding file map to table.
     */

    Tcl_DictObjFirst(NULL, map, &mapSearch, &name, NULL, &done);
    for (; !done; Tcl_DictObjNext(&mapSearch, &name, NULL, &done)) {
	Tcl_CreateHashEntry(&table, name, &dummy);
    }

    /*
     * Pull all encoding names from table into the result list.
     */

    for (hPtr = Tcl_FirstHashEntry(&table, &search); hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&search)) {
	Tcl_ListObjAppendElement(NULL, result,
		(Tcl_Obj *) Tcl_GetHashKey(&table, hPtr));
    }
    Tcl_SetObjResult(interp, result);
    Tcl_DeleteHashTable(&table);
}

/*
 *------------------------------------------------------------------------
 *
 * Tcl_SetSystemEncoding --
 *
 *	Sets the default encoding that should be used whenever the user passes
 *	a NULL value in to one of the conversion routines. If the supplied
 *	name is NULL, the system encoding is reset to the default system
 *	encoding.
 *
 * Results:
 *	The return value is TCL_OK if the system encoding was successfully set
 *	to the encoding specified by name, TCL_ERROR otherwise. If TCL_ERROR
 *	is returned, an error message is left in interp's result object,
 *	unless interp was NULL.
 *
 * Side effects:
 *	The reference count of the new system encoding is incremented. The
 *	reference count of the old system encoding is decremented and it may
 *	be freed. All VFS cached information is invalidated.
 *
 *------------------------------------------------------------------------
 */

int
Tcl_SetSystemEncoding(
    Tcl_Interp *interp,		/* Interp for error reporting, if not NULL. */
    const char *name)		/* The name of the desired encoding, or NULL/""
				 * to reset to default encoding. */
{
    Tcl_Encoding encoding;
    Encoding *encodingPtr;

    if (!name || !*name) {
	Tcl_MutexLock(&encodingMutex);
	encoding = defaultEncoding;
	encodingPtr = (Encoding *) encoding;
	encodingPtr->refCount++;
	Tcl_MutexUnlock(&encodingMutex);
    } else {
	encoding = Tcl_GetEncoding(interp, name);
	if (encoding == NULL) {
	    return TCL_ERROR;
	}
    }

    Tcl_MutexLock(&encodingMutex);
    FreeEncoding(systemEncoding);
    systemEncoding = encoding;
    Tcl_MutexUnlock(&encodingMutex);
    Tcl_FSMountsChanged(NULL);

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_CreateEncoding --
 *
 *	This function is called to define a new encoding and the functions
 *	that are used to convert between the specified encoding and Unicode.
 *
 * Results:
 *	Returns a token that represents the encoding. If an encoding with the
 *	same name already existed, the old encoding token remains valid and
 *	continues to behave as it used to, and will eventually be garbage
 *	collected when the last reference to it goes away. Any subsequent
 *	calls to Tcl_GetEncoding with the specified name will retrieve the
 *	most recent encoding token.
 *
 * Side effects:
 *	The new encoding type is entered into a table visible to all
 *	interpreters, keyed off the encoding's name. For each call to this
 *	function, there should eventually be a call to Tcl_FreeEncoding, so
 *	that the database can be cleaned up when encodings aren't needed
 *	anymore.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Encoding
Tcl_CreateEncoding(
    const Tcl_EncodingType *typePtr)
				/* The encoding type. */
{
    Tcl_HashEntry *hPtr;
    int isNew;
    Encoding *encodingPtr;
    char *name;

    Tcl_MutexLock(&encodingMutex);
    hPtr = Tcl_CreateHashEntry(&encodingTable, typePtr->encodingName, &isNew);
    if (isNew == 0) {
	/*
	 * Remove old encoding from hash table, but don't delete it until last
	 * reference goes away.
	 */

	encodingPtr = Tcl_GetHashValue(hPtr);
	encodingPtr->hPtr = NULL;
    }

    name = ckalloc(strlen(typePtr->encodingName) + 1);

    encodingPtr = ckalloc(sizeof(Encoding));
    encodingPtr->name		= strcpy(name, typePtr->encodingName);
    encodingPtr->toUtfProc	= typePtr->toUtfProc;
    encodingPtr->fromUtfProc	= typePtr->fromUtfProc;
    encodingPtr->freeProc	= typePtr->freeProc;
    encodingPtr->nullSize	= typePtr->nullSize;
    encodingPtr->clientData	= typePtr->clientData;
    if (typePtr->nullSize == 1) {
	encodingPtr->lengthProc = (LengthProc *) strlen;
    } else {
	encodingPtr->lengthProc = (LengthProc *) unilen;
    }
    encodingPtr->refCount	= 1;
    encodingPtr->hPtr		= hPtr;
    Tcl_SetHashValue(hPtr, encodingPtr);

    Tcl_MutexUnlock(&encodingMutex);

    return (Tcl_Encoding) encodingPtr;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_ExternalToUtfDString --
 *
 *	Convert a source buffer from the specified encoding into UTF-8. If any
 *	of the bytes in the source buffer are invalid or cannot be represented
 *	in the target encoding, a default fallback character will be
 *	substituted.
 *
 * Results:
 *	The converted bytes are stored in the DString, which is then NULL
 *	terminated. The return value is a pointer to the value stored in the
 *	DString.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

char *
Tcl_ExternalToUtfDString(
    Tcl_Encoding encoding,	/* The encoding for the source string, or NULL
				 * for the default system encoding. */
    const char *src,		/* Source string in specified encoding. */
    int srcLen,			/* Source string length in bytes, or < 0 for
				 * encoding-specific string length. */
    Tcl_DString *dstPtr)	/* Uninitialized or free DString in which the
				 * converted string is stored. */
{
    char *dst;
    Tcl_EncodingState state;
    const Encoding *encodingPtr;
    int flags, dstLen, result, soFar, srcRead, dstWrote, dstChars;

    Tcl_DStringInit(dstPtr);
    dst = Tcl_DStringValue(dstPtr);
    dstLen = dstPtr->spaceAvl - 1;

    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = encodingPtr->lengthProc(src);
    }

    flags = TCL_ENCODING_START | TCL_ENCODING_END;

    while (1) {
	result = encodingPtr->toUtfProc(encodingPtr->clientData, src, srcLen,
		flags, &state, dst, dstLen, &srcRead, &dstWrote, &dstChars);
	soFar = dst + dstWrote - Tcl_DStringValue(dstPtr);

	if (result != TCL_CONVERT_NOSPACE) {
	    Tcl_DStringSetLength(dstPtr, soFar);
	    return Tcl_DStringValue(dstPtr);
	}

	flags &= ~TCL_ENCODING_START;
	src += srcRead;
	srcLen -= srcRead;
	if (Tcl_DStringLength(dstPtr) == 0) {
	    Tcl_DStringSetLength(dstPtr, dstLen);
	}
	Tcl_DStringSetLength(dstPtr, 2 * Tcl_DStringLength(dstPtr) + 1);
	dst = Tcl_DStringValue(dstPtr) + soFar;
	dstLen = Tcl_DStringLength(dstPtr) - soFar - 1;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_ExternalToUtf --
 *
 *	Convert a source buffer from the specified encoding into UTF-8.
 *
 * Results:
 *	The return value is one of TCL_OK, TCL_CONVERT_MULTIBYTE,
 *	TCL_CONVERT_SYNTAX, TCL_CONVERT_UNKNOWN, or TCL_CONVERT_NOSPACE, as
 *	documented in tcl.h.
 *
 * Side effects:
 *	The converted bytes are stored in the output buffer.
 *
 *-------------------------------------------------------------------------
 */

int
Tcl_ExternalToUtf(
    Tcl_Interp *interp,		/* Interp for error return, if not NULL. */
    Tcl_Encoding encoding,	/* The encoding for the source string, or NULL
				 * for the default system encoding. */
    const char *src,		/* Source string in specified encoding. */
    int srcLen,			/* Source string length in bytes, or < 0 for
				 * encoding-specific string length. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const Encoding *encodingPtr;
    int result, srcRead, dstWrote, dstChars = 0;
    int noTerminate = flags & TCL_ENCODING_NO_TERMINATE;
    int charLimited = (flags & TCL_ENCODING_CHAR_LIMIT) && dstCharsPtr;
    int maxChars = INT_MAX;
    Tcl_EncodingState state;

    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = encodingPtr->lengthProc(src);
    }
    if (statePtr == NULL) {
	flags |= TCL_ENCODING_START | TCL_ENCODING_END;
	statePtr = &state;
    }
    if (srcReadPtr == NULL) {
	srcReadPtr = &srcRead;
    }
    if (dstWrotePtr == NULL) {
	dstWrotePtr = &dstWrote;
    }
    if (dstCharsPtr == NULL) {
	dstCharsPtr = &dstChars;
	flags &= ~TCL_ENCODING_CHAR_LIMIT;
    } else if (charLimited) {
	maxChars = *dstCharsPtr;
    }

    if (!noTerminate) {
	/*
	 * If there are any null characters in the middle of the buffer,
	 * they will converted to the UTF-8 null character (\xC080). To get
	 * the actual \0 at the end of the destination buffer, we need to
	 * append it manually.  First make room for it...
	 */

	dstLen--;
    }
    do {
	int savedFlags = flags;
	Tcl_EncodingState savedState = *statePtr;

	result = encodingPtr->toUtfProc(encodingPtr->clientData, src, srcLen,
		flags, statePtr, dst, dstLen, srcReadPtr, dstWrotePtr,
		dstCharsPtr);
	if (*dstCharsPtr <= maxChars) {
	    break;
	}
	dstLen = Tcl_UtfAtIndex(dst, maxChars) - dst + (TCL_UTF_MAX - 1);
	flags = savedFlags;
	*statePtr = savedState;
    } while (1);
    if (!noTerminate) {
	/* ...and then append it */

	dst[*dstWrotePtr] = '\0';
    }

    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_UtfToExternalDString --
 *
 *	Convert a source buffer from UTF-8 into the specified encoding. If any
 *	of the bytes in the source buffer are invalid or cannot be represented
 *	in the target encoding, a default fallback character will be
 *	substituted.
 *
 * Results:
 *	The converted bytes are stored in the DString, which is then NULL
 *	terminated in an encoding-specific manner. The return value is a
 *	pointer to the value stored in the DString.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

char *
Tcl_UtfToExternalDString(
    Tcl_Encoding encoding,	/* The encoding for the converted string, or
				 * NULL for the default system encoding. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes, or < 0 for
				 * strlen(). */
    Tcl_DString *dstPtr)	/* Uninitialized or free DString in which the
				 * converted string is stored. */
{
    char *dst;
    Tcl_EncodingState state;
    const Encoding *encodingPtr;
    int flags, dstLen, result, soFar, srcRead, dstWrote, dstChars;

    Tcl_DStringInit(dstPtr);
    dst = Tcl_DStringValue(dstPtr);
    dstLen = dstPtr->spaceAvl - 1;

    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = strlen(src);
    }
    flags = TCL_ENCODING_START | TCL_ENCODING_END;
    while (1) {
	result = encodingPtr->fromUtfProc(encodingPtr->clientData, src,
		srcLen, flags, &state, dst, dstLen, &srcRead, &dstWrote,
		&dstChars);
	soFar = dst + dstWrote - Tcl_DStringValue(dstPtr);

	if (result != TCL_CONVERT_NOSPACE) {
	    if (encodingPtr->nullSize == 2) {
		Tcl_DStringSetLength(dstPtr, soFar + 1);
	    }
	    Tcl_DStringSetLength(dstPtr, soFar);
	    return Tcl_DStringValue(dstPtr);
	}

	flags &= ~TCL_ENCODING_START;
	src += srcRead;
	srcLen -= srcRead;
	if (Tcl_DStringLength(dstPtr) == 0) {
	    Tcl_DStringSetLength(dstPtr, dstLen);
	}
	Tcl_DStringSetLength(dstPtr, 2 * Tcl_DStringLength(dstPtr) + 1);
	dst = Tcl_DStringValue(dstPtr) + soFar;
	dstLen = Tcl_DStringLength(dstPtr) - soFar - 1;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_UtfToExternal --
 *
 *	Convert a buffer from UTF-8 into the specified encoding.
 *
 * Results:
 *	The return value is one of TCL_OK, TCL_CONVERT_MULTIBYTE,
 *	TCL_CONVERT_SYNTAX, TCL_CONVERT_UNKNOWN, or TCL_CONVERT_NOSPACE, as
 *	documented in tcl.h.
 *
 * Side effects:
 *	The converted bytes are stored in the output buffer.
 *
 *-------------------------------------------------------------------------
 */

int
Tcl_UtfToExternal(
    Tcl_Interp *interp,		/* Interp for error return, if not NULL. */
    Tcl_Encoding encoding,	/* The encoding for the converted string, or
				 * NULL for the default system encoding. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes, or < 0 for
				 * strlen(). */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string
				 * is stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const Encoding *encodingPtr;
    int result, srcRead, dstWrote, dstChars;
    Tcl_EncodingState state;

    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = strlen(src);
    }
    if (statePtr == NULL) {
	flags |= TCL_ENCODING_START | TCL_ENCODING_END;
	statePtr = &state;
    }
    if (srcReadPtr == NULL) {
	srcReadPtr = &srcRead;
    }
    if (dstWrotePtr == NULL) {
	dstWrotePtr = &dstWrote;
    }
    if (dstCharsPtr == NULL) {
	dstCharsPtr = &dstChars;
    }

    dstLen -= encodingPtr->nullSize;
    result = encodingPtr->fromUtfProc(encodingPtr->clientData, src, srcLen,
	    flags, statePtr, dst, dstLen, srcReadPtr, dstWrotePtr,
	    dstCharsPtr);
    if (encodingPtr->nullSize == 2) {
	dst[*dstWrotePtr + 1] = '\0';
    }
    dst[*dstWrotePtr] = '\0';

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FindExecutable --
 *
 *	This function computes the absolute path name of the current
 *	application, given its argv[0] value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The absolute pathname for the application is computed and stored to be
 *	returned later by [info nameofexecutable].
 *
 *---------------------------------------------------------------------------
 */
#undef Tcl_FindExecutable
void
Tcl_FindExecutable(
    const char *argv0)		/* The value of the application's argv[0]
				 * (native). */
{
    TclInitSubsystems();
    TclpSetInitialEncodings();
    TclpFindExecutable(argv0);
}

/*
 *---------------------------------------------------------------------------
 *
 * OpenEncodingFileChannel --
 *
 *	Open the file believed to hold data for the encoding, "name".
 *
 * Results:
 *	Returns the readable Tcl_Channel from opening the file, or NULL if the
 *	file could not be successfully opened. If NULL was returned, an error
 *	message is left in interp's result object, unless interp was NULL.
 *
 * Side effects:
 *	Channel may be opened. Information about the filesystem may be cached
 *	to speed later calls.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Channel
OpenEncodingFileChannel(
    Tcl_Interp *interp,		/* Interp for error reporting, if not NULL. */
    const char *name)		/* The name of the encoding file on disk and
				 * also the name for new encoding. */
{
    Tcl_Obj *nameObj = Tcl_NewStringObj(name, -1);
    Tcl_Obj *fileNameObj = Tcl_DuplicateObj(nameObj);
    Tcl_Obj *searchPath = Tcl_DuplicateObj(Tcl_GetEncodingSearchPath());
    Tcl_Obj *map = TclGetProcessGlobalValue(&encodingFileMap);
    Tcl_Obj **dir, *path, *directory = NULL;
    Tcl_Channel chan = NULL;
    int i, numDirs;

    Tcl_ListObjGetElements(NULL, searchPath, &numDirs, &dir);
    Tcl_IncrRefCount(nameObj);
    Tcl_AppendToObj(fileNameObj, ".enc", -1);
    Tcl_IncrRefCount(fileNameObj);
    Tcl_DictObjGet(NULL, map, nameObj, &directory);

    /*
     * Check that any cached directory is still on the encoding search path.
     */

    if (NULL != directory) {
	int verified = 0;

	for (i=0; i<numDirs && !verified; i++) {
	    if (dir[i] == directory) {
		verified = 1;
	    }
	}
	if (!verified) {
	    const char *dirString = TclGetString(directory);

	    for (i=0; i<numDirs && !verified; i++) {
		if (strcmp(dirString, TclGetString(dir[i])) == 0) {
		    verified = 1;
		}
	    }
	}
	if (!verified) {
	    /*
	     * Directory no longer on the search path. Remove from cache.
	     */

	    map = Tcl_DuplicateObj(map);
	    Tcl_DictObjRemove(NULL, map, nameObj);
	    TclSetProcessGlobalValue(&encodingFileMap, map, NULL);
	    directory = NULL;
	}
    }

    if (NULL != directory) {
	/*
	 * Got a directory from the cache. Try to use it first.
	 */

	Tcl_IncrRefCount(directory);
	path = Tcl_FSJoinToPath(directory, 1, &fileNameObj);
	Tcl_IncrRefCount(path);
	Tcl_DecrRefCount(directory);
	chan = Tcl_FSOpenFileChannel(NULL, path, "r", 0);
	Tcl_DecrRefCount(path);
    }

    /*
     * Scan the search path until we find it.
     */

    for (i=0; i<numDirs && (chan == NULL); i++) {
	path = Tcl_FSJoinToPath(dir[i], 1, &fileNameObj);
	Tcl_IncrRefCount(path);
	chan = Tcl_FSOpenFileChannel(NULL, path, "r", 0);
	Tcl_DecrRefCount(path);
	if (chan != NULL) {
	    /*
	     * Save directory in the cache.
	     */

	    map = Tcl_DuplicateObj(TclGetProcessGlobalValue(&encodingFileMap));
	    Tcl_DictObjPut(NULL, map, nameObj, dir[i]);
	    TclSetProcessGlobalValue(&encodingFileMap, map, NULL);
	}
    }

    if ((NULL == chan) && (interp != NULL)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown encoding \"%s\"", name));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ENCODING", name, NULL);
    }
    Tcl_DecrRefCount(fileNameObj);
    Tcl_DecrRefCount(nameObj);
    Tcl_DecrRefCount(searchPath);

    return chan;
}

/*
 *---------------------------------------------------------------------------
 *
 * LoadEncodingFile --
 *
 *	Read a file that describes an encoding and create a new Encoding from
 *	the data.
 *
 * Results:
 *	The return value is the newly loaded Encoding, or NULL if the file
 *	didn't exist of was in the incorrect format. If NULL was returned, an
 *	error message is left in interp's result object, unless interp was
 *	NULL.
 *
 * Side effects:
 *	File read from disk.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Encoding
LoadEncodingFile(
    Tcl_Interp *interp,		/* Interp for error reporting, if not NULL. */
    const char *name)		/* The name of the encoding file on disk and
				 * also the name for new encoding. */
{
    Tcl_Channel chan = NULL;
    Tcl_Encoding encoding = NULL;
    int ch;

    chan = OpenEncodingFileChannel(interp, name);
    if (chan == NULL) {
	return NULL;
    }

    Tcl_SetChannelOption(NULL, chan, "-encoding", "utf-8");

    while (1) {
	Tcl_DString ds;

	Tcl_DStringInit(&ds);
	Tcl_Gets(chan, &ds);
	ch = Tcl_DStringValue(&ds)[0];
	Tcl_DStringFree(&ds);
	if (ch != '#') {
	    break;
	}
    }

    switch (ch) {
    case 'S':
	encoding = LoadTableEncoding(name, ENCODING_SINGLEBYTE, chan);
	break;
    case 'D':
	encoding = LoadTableEncoding(name, ENCODING_DOUBLEBYTE, chan);
	break;
    case 'M':
	encoding = LoadTableEncoding(name, ENCODING_MULTIBYTE, chan);
	break;
    case 'E':
	encoding = LoadEscapeEncoding(name, chan);
	break;
    }
    if ((encoding == NULL) && (interp != NULL)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"invalid encoding file \"%s\"", name));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ENCODING", name, NULL);
    }
    Tcl_Close(NULL, chan);

    return encoding;
}

/*
 *-------------------------------------------------------------------------
 *
 * LoadTableEncoding --
 *
 *	Helper function for LoadEncodingTable(). Loads a table to that
 *	converts between Unicode and some other encoding and creates an
 *	encoding (using a TableEncoding structure) from that information.
 *
 *	File contains binary data, but begins with a marker to indicate
 *	byte-ordering, so that same binary file can be read on either endian
 *	platforms.
 *
 * Results:
 *	The return value is the new encoding, or NULL if the encoding could
 *	not be created (because the file contained invalid data).
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static Tcl_Encoding
LoadTableEncoding(
    const char *name,		/* Name for new encoding. */
    int type,			/* Type of encoding (ENCODING_?????). */
    Tcl_Channel chan)		/* File containing new encoding. */
{
    Tcl_DString lineString;
    Tcl_Obj *objPtr;
    char *line;
    int i, hi, lo, numPages, symbol, fallback, len;
    unsigned char used[256];
    unsigned size;
    TableEncodingData *dataPtr;
    unsigned short *pageMemPtr, *page;
    Tcl_EncodingType encType;

    /*
     * Speed over memory. Use a full 256 character table to decode hex
     * sequences in the encoding files.
     */

    static const char staticHex[] = {
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*   0 ...  15 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  16 ...  31 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  32 ...  47 */
      0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /*  48 ...  63 */
      0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  64 ...  79 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  80 ...  95 */
      0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  96 ... 111 */
      0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 112 ... 127 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 128 ... 143 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 144 ... 159 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 160 ... 175 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 176 ... 191 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 192 ... 207 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 208 ... 223 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 224 ... 239 */
      0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 240 ... 255 */
    };

    Tcl_DStringInit(&lineString);
    if (Tcl_Gets(chan, &lineString) == -1) {
	return NULL;
    }
    line = Tcl_DStringValue(&lineString);

    fallback = (int) strtol(line, &line, 16);
    symbol = (int) strtol(line, &line, 10);
    numPages = (int) strtol(line, &line, 10);
    Tcl_DStringFree(&lineString);

    if (numPages < 0) {
	numPages = 0;
    } else if (numPages > 256) {
	numPages = 256;
    }

    memset(used, 0, sizeof(used));

#undef PAGESIZE
#define PAGESIZE    (256 * sizeof(unsigned short))

    dataPtr = ckalloc(sizeof(TableEncodingData));
    memset(dataPtr, 0, sizeof(TableEncodingData));

    dataPtr->fallback = fallback;

    /*
     * Read the table that maps characters to Unicode. Performs a single
     * malloc to get the memory for the array and all the pages needed by the
     * array.
     */

    size = 256 * sizeof(unsigned short *) + numPages * PAGESIZE;
    dataPtr->toUnicode = ckalloc(size);
    memset(dataPtr->toUnicode, 0, size);
    pageMemPtr = (unsigned short *) (dataPtr->toUnicode + 256);

    TclNewObj(objPtr);
    Tcl_IncrRefCount(objPtr);
    for (i = 0; i < numPages; i++) {
	int ch;
	const char *p;
	int expected = 3 + 16 * (16 * 4 + 1);

	if (Tcl_ReadChars(chan, objPtr, expected, 0) != expected) {
	    return NULL;
	}
	p = TclGetString(objPtr);
	hi = (staticHex[UCHAR(p[0])] << 4) + staticHex[UCHAR(p[1])];
	dataPtr->toUnicode[hi] = pageMemPtr;
	p += 2;
	for (lo = 0; lo < 256; lo++) {
	    if ((lo & 0x0f) == 0) {
		p++;
	    }
	    ch = (staticHex[UCHAR(p[0])] << 12) + (staticHex[UCHAR(p[1])] << 8)
		    + (staticHex[UCHAR(p[2])] << 4) + staticHex[UCHAR(p[3])];
	    if (ch != 0) {
		used[ch >> 8] = 1;
	    }
	    *pageMemPtr = (unsigned short) ch;
	    pageMemPtr++;
	    p += 4;
	}
    }
    TclDecrRefCount(objPtr);

    if (type == ENCODING_DOUBLEBYTE) {
	memset(dataPtr->prefixBytes, 1, sizeof(dataPtr->prefixBytes));
    } else {
	for (hi = 1; hi < 256; hi++) {
	    if (dataPtr->toUnicode[hi] != NULL) {
		dataPtr->prefixBytes[hi] = 1;
	    }
	}
    }

    /*
     * Invert toUnicode array to produce the fromUnicode array. Performs a
     * single malloc to get the memory for the array and all the pages needed
     * by the array. While reading in the toUnicode array, we remembered what
     * pages that would be needed for the fromUnicode array.
     */

    if (symbol) {
	used[0] = 1;
    }
    numPages = 0;
    for (hi = 0; hi < 256; hi++) {
	if (used[hi]) {
	    numPages++;
	}
    }
    size = 256 * sizeof(unsigned short *) + numPages * PAGESIZE;
    dataPtr->fromUnicode = ckalloc(size);
    memset(dataPtr->fromUnicode, 0, size);
    pageMemPtr = (unsigned short *) (dataPtr->fromUnicode + 256);

    for (hi = 0; hi < 256; hi++) {
	if (dataPtr->toUnicode[hi] == NULL) {
	    dataPtr->toUnicode[hi] = emptyPage;
	    continue;
	}
	for (lo = 0; lo < 256; lo++) {
	    int ch = dataPtr->toUnicode[hi][lo];

	    if (ch != 0) {
		page = dataPtr->fromUnicode[ch >> 8];
		if (page == NULL) {
		    page = pageMemPtr;
		    pageMemPtr += 256;
		    dataPtr->fromUnicode[ch >> 8] = page;
		}
		page[ch & 0xff] = (unsigned short) ((hi << 8) + lo);
	    }
	}
    }
    if (type == ENCODING_MULTIBYTE) {
	/*
	 * If multibyte encodings don't have a backslash character, define
	 * one. Otherwise, on Windows, native file names won't work because
	 * the backslash in the file name will map to the unknown character
	 * (question mark) when converting from UTF-8 to external encoding.
	 */

	if (dataPtr->fromUnicode[0] != NULL) {
	    if (dataPtr->fromUnicode[0]['\\'] == '\0') {
		dataPtr->fromUnicode[0]['\\'] = '\\';
	    }
	}
    }
    if (symbol) {
	/*
	 * Make a special symbol encoding that not only maps the symbol
	 * characters from their Unicode code points down into page 0, but
	 * also ensure that the characters on page 0 map to themselves. This
	 * is so that a symbol font can be used to display a simple string
	 * like "abcd" and have alpha, beta, chi, delta show up, rather than
	 * have "unknown" chars show up because strictly speaking the symbol
	 * font doesn't have glyphs for those low ASCII chars.
	 */

	page = dataPtr->fromUnicode[0];
	if (page == NULL) {
	    page = pageMemPtr;
	    dataPtr->fromUnicode[0] = page;
	}
	for (lo = 0; lo < 256; lo++) {
	    if (dataPtr->toUnicode[0][lo] != 0) {
		page[lo] = (unsigned short) lo;
	    }
	}
    }
    for (hi = 0; hi < 256; hi++) {
	if (dataPtr->fromUnicode[hi] == NULL) {
	    dataPtr->fromUnicode[hi] = emptyPage;
	}
    }

    /*
     * For trailing 'R'everse encoding, see [Patch 689341]
     */

    Tcl_DStringInit(&lineString);

    /*
     * Skip leading empty lines.
     */

    while ((len = Tcl_Gets(chan, &lineString)) == 0) {
	/* empty body */
    }
    if (len < 0) {
	goto doneParse;
    }

    /*
     * Require that it starts with an 'R'.
     */

    line = Tcl_DStringValue(&lineString);
    if (line[0] != 'R') {
	goto doneParse;
    }

    /*
     * Read lines from the encoding until EOF.
     */

    for (TclDStringClear(&lineString);
	    (len = Tcl_Gets(chan, &lineString)) >= 0;
	    TclDStringClear(&lineString)) {
	const unsigned char *p;
	int to, from;

	/*
	 * Skip short lines.
	 */

	if (len < 5) {
	    continue;
	}

	/*
	 * Parse the line as a sequence of hex digits.
	 */

	p = (const unsigned char *) Tcl_DStringValue(&lineString);
	to = (staticHex[p[0]] << 12) + (staticHex[p[1]] << 8)
		+ (staticHex[p[2]] << 4) + staticHex[p[3]];
	if (to == 0) {
	    continue;
	}
	for (p += 5, len -= 5; len >= 0 && *p; p += 5, len -= 5) {
	    from = (staticHex[p[0]] << 12) + (staticHex[p[1]] << 8)
		    + (staticHex[p[2]] << 4) + staticHex[p[3]];
	    if (from == 0) {
		continue;
	    }
	    dataPtr->fromUnicode[from >> 8][from & 0xff] = to;
	}
    }
  doneParse:
    Tcl_DStringFree(&lineString);

    /*
     * Package everything into an encoding structure.
     */

    encType.encodingName    = name;
    encType.toUtfProc	    = TableToUtfProc;
    encType.fromUtfProc	    = TableFromUtfProc;
    encType.freeProc	    = TableFreeProc;
    encType.nullSize	    = (type == ENCODING_DOUBLEBYTE) ? 2 : 1;
    encType.clientData	    = dataPtr;

    return Tcl_CreateEncoding(&encType);
}

/*
 *-------------------------------------------------------------------------
 *
 * LoadEscapeEncoding --
 *
 *	Helper function for LoadEncodingTable(). Loads a state machine that
 *	converts between Unicode and some other encoding.
 *
 *	File contains text data that describes the escape sequences that are
 *	used to choose an encoding and the associated names for the
 *	sub-encodings.
 *
 * Results:
 *	The return value is the new encoding, or NULL if the encoding could
 *	not be created (because the file contained invalid data).
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static Tcl_Encoding
LoadEscapeEncoding(
    const char *name,		/* Name for new encoding. */
    Tcl_Channel chan)		/* File containing new encoding. */
{
    int i;
    unsigned size;
    Tcl_DString escapeData;
    char init[16], final[16];
    EscapeEncodingData *dataPtr;
    Tcl_EncodingType type;

    init[0] = '\0';
    final[0] = '\0';
    Tcl_DStringInit(&escapeData);

    while (1) {
	int argc;
	const char **argv;
	char *line;
	Tcl_DString lineString;

	Tcl_DStringInit(&lineString);
	if (Tcl_Gets(chan, &lineString) < 0) {
	    break;
	}
	line = Tcl_DStringValue(&lineString);
	if (Tcl_SplitList(NULL, line, &argc, &argv) != TCL_OK) {
	    Tcl_DStringFree(&lineString);
	    continue;
	}
	if (argc >= 2) {
	    if (strcmp(argv[0], "name") == 0) {
		/* do nothing */
	    } else if (strcmp(argv[0], "init") == 0) {
		strncpy(init, argv[1], sizeof(init));
		init[sizeof(init) - 1] = '\0';
	    } else if (strcmp(argv[0], "final") == 0) {
		strncpy(final, argv[1], sizeof(final));
		final[sizeof(final) - 1] = '\0';
	    } else {
		EscapeSubTable est;
		Encoding *e;

		strncpy(est.sequence, argv[1], sizeof(est.sequence));
		est.sequence[sizeof(est.sequence) - 1] = '\0';
		est.sequenceLen = strlen(est.sequence);

		strncpy(est.name, argv[0], sizeof(est.name));
		est.name[sizeof(est.name) - 1] = '\0';

		/*
		 * To avoid infinite recursion in [encoding system iso2022-*]
		 */

		e = (Encoding *) Tcl_GetEncoding(NULL, est.name);
		if ((e != NULL) && (e->toUtfProc != TableToUtfProc)
			&& (e->toUtfProc != Iso88591ToUtfProc)) {
		   Tcl_FreeEncoding((Tcl_Encoding) e);
		   e = NULL;
		}
		est.encodingPtr = e;
		Tcl_DStringAppend(&escapeData, (char *) &est, sizeof(est));
	    }
	}
	ckfree(argv);
	Tcl_DStringFree(&lineString);
    }

    size = sizeof(EscapeEncodingData) - sizeof(EscapeSubTable)
	    + Tcl_DStringLength(&escapeData);
    dataPtr = ckalloc(size);
    dataPtr->initLen = strlen(init);
    memcpy(dataPtr->init, init, dataPtr->initLen + 1);
    dataPtr->finalLen = strlen(final);
    memcpy(dataPtr->final, final, dataPtr->finalLen + 1);
    dataPtr->numSubTables =
	    Tcl_DStringLength(&escapeData) / sizeof(EscapeSubTable);
    memcpy(dataPtr->subTables, Tcl_DStringValue(&escapeData),
	    Tcl_DStringLength(&escapeData));
    Tcl_DStringFree(&escapeData);

    memset(dataPtr->prefixBytes, 0, sizeof(dataPtr->prefixBytes));
    for (i = 0; i < dataPtr->numSubTables; i++) {
	dataPtr->prefixBytes[UCHAR(dataPtr->subTables[i].sequence[0])] = 1;
    }
    if (dataPtr->init[0] != '\0') {
	dataPtr->prefixBytes[UCHAR(dataPtr->init[0])] = 1;
    }
    if (dataPtr->final[0] != '\0') {
	dataPtr->prefixBytes[UCHAR(dataPtr->final[0])] = 1;
    }

    /*
     * Package everything into an encoding structure.
     */

    type.encodingName	= name;
    type.toUtfProc	= EscapeToUtfProc;
    type.fromUtfProc    = EscapeFromUtfProc;
    type.freeProc	= EscapeFreeProc;
    type.nullSize	= 1;
    type.clientData	= dataPtr;

    return Tcl_CreateEncoding(&type);
}

/*
 *-------------------------------------------------------------------------
 *
 * BinaryProc --
 *
 *	The default conversion when no other conversion is specified. No
 *	translation is done; source bytes are copied directly to destination
 *	bytes.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
BinaryProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string (unknown encoding). */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    int result;

    result = TCL_OK;
    dstLen -= TCL_UTF_MAX - 1;
    if (dstLen < 0) {
	dstLen = 0;
    }
    if ((flags & TCL_ENCODING_CHAR_LIMIT) && srcLen > *dstCharsPtr) {
	srcLen = *dstCharsPtr;
    }
    if (srcLen > dstLen) {
	srcLen = dstLen;
	result = TCL_CONVERT_NOSPACE;
    }

    *srcReadPtr = srcLen;
    *dstWrotePtr = srcLen;
    *dstCharsPtr = srcLen;
    memcpy(dst, src, srcLen);
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfIntToUtfExtProc --
 *
 *	Convert from UTF-8 to UTF-8. While converting null-bytes from the
 *	Tcl's internal representation (0xc0, 0x80) to the official
 *	representation (0x00). See UtfToUtfProc for details.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
UtfIntToUtfExtProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string
				 * is stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    return UtfToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	    srcReadPtr, dstWrotePtr, dstCharsPtr, 1);
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfExtToUtfIntProc --
 *
 *	Convert from UTF-8 to UTF-8 while converting null-bytes from the
 *	official representation (0x00) to Tcl's internal representation (0xc0,
 *	0x80). See UtfToUtfProc for details.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
UtfExtToUtfIntProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    return UtfToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	    srcReadPtr, dstWrotePtr, dstCharsPtr, 0);
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfToUtfProc --
 *
 *	Convert from UTF-8 to UTF-8. Note that the UTF-8 to UTF-8 translation
 *	is not a no-op, because it will turn a stream of improperly formed
 *	UTF-8 into a properly formed stream.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
UtfToUtfProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr,		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
    int pureNullMode)		/* Convert embedded nulls from internal
				 * representation to real null-bytes or vice
				 * versa. Also combine or separate surrogate pairs */
{
    const char *srcStart, *srcEnd, *srcClose;
    const char *dstStart, *dstEnd;
    int result, numChars, charLimit = INT_MAX;
    Tcl_UniChar *chPtr = (Tcl_UniChar *) statePtr;

    if (flags & TCL_ENCODING_START) {
    	*statePtr = 0;
    }
    result = TCL_OK;

    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= 6;
    }
    if (flags & TCL_ENCODING_CHAR_LIMIT) {
	charLimit = *dstCharsPtr;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - ((pureNullMode == 1) ? 4 : TCL_UTF_MAX);

    for (numChars = 0; src < srcEnd && numChars <= charLimit; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	if (UCHAR(*src) < 0x80 && !(UCHAR(*src) == 0 && pureNullMode == 0)) {
	    /*
	     * Copy 7bit characters, but skip null-bytes when we are in input
	     * mode, so that they get converted to 0xc080.
	     */

	    *dst++ = *src++;
	} else if (pureNullMode == 1 && UCHAR(*src) == 0xc0 &&
		(src + 1 < srcEnd) && UCHAR(*(src+1)) == 0x80) {
	    /*
	     * Convert 0xc080 to real nulls when we are in output mode.
	     */

	    *dst++ = 0;
	    src += 2;
	} else if (!Tcl_UtfCharComplete(src, srcEnd - src)) {
	    /*
	     * Always check before using TclUtfToUniChar. Not doing can so
	     * cause it run beyond the end of the buffer! If we happen such an
	     * incomplete char its bytes are made to represent themselves.
	     */

	    *chPtr = (unsigned char) *src;
	    src += 1;
	    dst += Tcl_UniCharToUtf(*chPtr, dst);
	} else {
	    src += TclUtfToUniChar(src, chPtr);
	    if ((*chPtr & 0xFC00) == 0xD800) {
		/* A high surrogate character is detected, handle especially */
		Tcl_UniChar low = *chPtr;
		size_t len = (src <= srcEnd-3) ? Tcl_UtfToUniChar(src, &low) : 0;
		if ((low & 0xFC00) != 0xDC00) {
			*dst++ = (char) (((*chPtr >> 12) | 0xE0) & 0xEF);
			*dst++ = (char) (((*chPtr >> 6) | 0x80) & 0xBF);
			*dst++ = (char) ((*chPtr | 0x80) & 0xBF);
			continue;
		} else if (pureNullMode == 1) {
		    int full = (((*chPtr & 0x3FF) << 10) | (low & 0x3FF)) + 0x10000;
		    *dst++ = (char) (((full >> 18) | 0xF0) & 0xF7);
		    *dst++ = (char) (((full >> 12) | 0x80) & 0xBF);
		    *dst++ = (char) (((full >> 6) | 0x80) & 0xBF);
		    *dst++ = (char) ((full | 0x80) & 0xBF);
			*chPtr = 0;
		    src += len;
		    continue;
		}
	    }
	    dst += Tcl_UniCharToUtf(*chPtr, dst);
	}
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UnicodeToUtfProc --
 *
 *	Convert from Unicode to UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
UnicodeToUtfProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string in Unicode. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd;
    const char *dstEnd, *dstStart;
    int result, numChars, charLimit = INT_MAX;
    Tcl_UniChar ch;

    if (flags & TCL_ENCODING_CHAR_LIMIT) {
	charLimit = *dstCharsPtr;
    }
    result = TCL_OK;
    if ((srcLen % sizeof(Tcl_UniChar)) != 0) {
	result = TCL_CONVERT_MULTIBYTE;
	srcLen /= sizeof(Tcl_UniChar);
	srcLen *= sizeof(Tcl_UniChar);
    }

    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    for (numChars = 0; src < srcEnd && numChars <= charLimit; numChars++) {
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}

	/*
	 * Special case for 1-byte utf chars for speed. Make sure we work with
	 * Tcl_UniChar-size data.
	 */

	ch = *(Tcl_UniChar *)src;
	if (ch && ch < 0x80) {
	    *dst++ = (ch & 0xFF);
	} else {
	    dst += Tcl_UniCharToUtf(ch, dst);
	}
	src += sizeof(Tcl_UniChar);
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfToUnicodeProc --
 *
 *	Convert from UTF-8 to Unicode.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
UtfToUnicodeProc(
    ClientData clientData,	/* TableEncodingData that specifies
				 * encoding. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd, *srcClose, *dstStart, *dstEnd;
    int result, numChars;
    Tcl_UniChar *chPtr = (Tcl_UniChar *) statePtr;

    if (flags & TCL_ENCODING_START) {
    	*statePtr = 0;
    }
    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd   = dst + dstLen - sizeof(Tcl_UniChar);

    result = TCL_OK;
    for (numChars = 0; src < srcEnd; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	src += TclUtfToUniChar(src, chPtr);

	/*
	 * Need to handle this in a way that won't cause misalignment by
	 * casting dst to a Tcl_UniChar. [Bug 1122671]
	 */

#ifdef WORDS_BIGENDIAN
#if TCL_UTF_MAX > 4
	*dst++ = (*chPtr >> 24);
	*dst++ = ((*chPtr >> 16) & 0xFF);
	*dst++ = ((*chPtr >> 8) & 0xFF);
	*dst++ = (*chPtr & 0xFF);
#else
	*dst++ = (*chPtr >> 8);
	*dst++ = (*chPtr & 0xFF);
#endif
#else
#if TCL_UTF_MAX > 4
	*dst++ = (*chPtr & 0xFF);
	*dst++ = ((*chPtr >> 8) & 0xFF);
	*dst++ = ((*chPtr >> 16) & 0xFF);
	*dst++ = (*chPtr >> 24);
#else
	*dst++ = (*chPtr & 0xFF);
	*dst++ = (*chPtr >> 8);
#endif
#endif
    }
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * TableToUtfProc --
 *
 *	Convert from the encoding specified by the TableEncodingData into
 *	UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
TableToUtfProc(
    ClientData clientData,	/* TableEncodingData that specifies
				 * encoding. */
    const char *src,		/* Source string in specified encoding. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd;
    const char *dstEnd, *dstStart, *prefixBytes;
    int result, byte, numChars, charLimit = INT_MAX;
    Tcl_UniChar ch = 0;
    const unsigned short *const *toUnicode;
    const unsigned short *pageZero;
    TableEncodingData *dataPtr = clientData;

    if (flags & TCL_ENCODING_CHAR_LIMIT) {
	charLimit = *dstCharsPtr;
    }
    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    toUnicode = (const unsigned short *const *) dataPtr->toUnicode;
    prefixBytes = dataPtr->prefixBytes;
    pageZero = toUnicode[0];

    result = TCL_OK;
    for (numChars = 0; src < srcEnd && numChars <= charLimit; numChars++) {
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	byte = *((unsigned char *) src);
	if (prefixBytes[byte]) {
	    src++;
	    if (src >= srcEnd) {
		src--;
		result = TCL_CONVERT_MULTIBYTE;
		break;
	    }
	    ch = toUnicode[byte][*((unsigned char *) src)];
	} else {
	    ch = pageZero[byte];
	}
	if ((ch == 0) && (byte != 0)) {
	    if (flags & TCL_ENCODING_STOPONERROR) {
		result = TCL_CONVERT_SYNTAX;
		break;
	    }
	    if (prefixBytes[byte]) {
		src--;
	    }
	    ch = (Tcl_UniChar) byte;
	}

	/*
	 * Special case for 1-byte utf chars for speed.
	 */

	if (ch && ch < 0x80) {
	    *dst++ = (char) ch;
	} else {
	    dst += Tcl_UniCharToUtf(ch, dst);
	}
	src++;
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * TableFromUtfProc --
 *
 *	Convert from UTF-8 into the encoding specified by the
 *	TableEncodingData.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
TableFromUtfProc(
    ClientData clientData,	/* TableEncodingData that specifies
				 * encoding. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd, *srcClose;
    const char *dstStart, *dstEnd, *prefixBytes;
    Tcl_UniChar ch = 0;
    int result, len, word, numChars;
    TableEncodingData *dataPtr = clientData;
    const unsigned short *const *fromUnicode;

    result = TCL_OK;

    prefixBytes = dataPtr->prefixBytes;
    fromUnicode = (const unsigned short *const *) dataPtr->fromUnicode;

    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - 1;

    for (numChars = 0; src < srcEnd; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	len = TclUtfToUniChar(src, &ch);

#if TCL_UTF_MAX > 4
	/*
	 * This prevents a crash condition. More evaluation is required for
	 * full support of int Tcl_UniChar. [Bug 1004065]
	 */

	if (ch & 0xffff0000) {
	    word = 0;
	} else
#elif TCL_UTF_MAX == 4
	if (!len) {
	    word = 0;
	} else
#endif
	    word = fromUnicode[(ch >> 8)][ch & 0xff];

	if ((word == 0) && (ch != 0)) {
	    if (flags & TCL_ENCODING_STOPONERROR) {
		result = TCL_CONVERT_UNKNOWN;
		break;
	    }
	    word = dataPtr->fallback;
	}
	if (prefixBytes[(word >> 8)] != 0) {
	    if (dst + 1 > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) (word >> 8);
	    dst[1] = (char) word;
	    dst += 2;
	} else {
	    if (dst > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) word;
	    dst++;
	}
	src += len;
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * Iso88591ToUtfProc --
 *
 *	Convert from the "iso8859-1" encoding into UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
Iso88591ToUtfProc(
    ClientData clientData,	/* Ignored. */
    const char *src,		/* Source string in specified encoding. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd;
    const char *dstEnd, *dstStart;
    int result, numChars, charLimit = INT_MAX;

    if (flags & TCL_ENCODING_CHAR_LIMIT) {
	charLimit = *dstCharsPtr;
    }
    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    result = TCL_OK;
    for (numChars = 0; src < srcEnd && numChars <= charLimit; numChars++) {
	Tcl_UniChar ch = 0;

	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	ch = (Tcl_UniChar) *((unsigned char *) src);

	/*
	 * Special case for 1-byte utf chars for speed.
	 */

	if (ch && ch < 0x80) {
	    *dst++ = (char) ch;
	} else {
	    dst += Tcl_UniCharToUtf(ch, dst);
	}
	src++;
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * Iso88591FromUtfProc --
 *
 *	Convert from UTF-8 into the encoding "iso8859-1".
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
Iso88591FromUtfProc(
    ClientData clientData,	/* Ignored. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd, *srcClose;
    const char *dstStart, *dstEnd;
    int result, numChars;
    Tcl_UniChar ch = 0;

    result = TCL_OK;

    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - 1;

    for (numChars = 0; src < srcEnd; numChars++) {
	int len;

	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	len = TclUtfToUniChar(src, &ch);

	/*
	 * Check for illegal characters.
	 */

	if (ch > 0xff
#if TCL_UTF_MAX == 4
		|| ((ch >= 0xD800) && (len < 3))
#endif
		) {
	    if (flags & TCL_ENCODING_STOPONERROR) {
		result = TCL_CONVERT_UNKNOWN;
		break;
	    }
#if TCL_UTF_MAX == 4
	    if ((ch >= 0xD800) && (len < 3)) len = 4;
#endif

	    /*
	     * Plunge on, using '?' as a fallback character.
	     */

	    ch = (Tcl_UniChar) '?';
	}

	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	*(dst++) = (char) ch;
	src += len;
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TableFreeProc --
 *
 *	This function is invoked when an encoding is deleted. It deletes the
 *	memory used by the TableEncodingData.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
TableFreeProc(
    ClientData clientData)	/* TableEncodingData that specifies
				 * encoding. */
{
    TableEncodingData *dataPtr = clientData;

    /*
     * Make sure we aren't freeing twice on shutdown. [Bug 219314]
     */

    ckfree(dataPtr->toUnicode);
    dataPtr->toUnicode = NULL;
    ckfree(dataPtr->fromUnicode);
    dataPtr->fromUnicode = NULL;
    ckfree(dataPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * EscapeToUtfProc --
 *
 *	Convert from the encoding specified by the EscapeEncodingData into
 *	UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
EscapeToUtfProc(
    ClientData clientData,	/* EscapeEncodingData that specifies
				 * encoding. */
    const char *src,		/* Source string in specified encoding. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    EscapeEncodingData *dataPtr = clientData;
    const char *prefixBytes, *tablePrefixBytes, *srcStart, *srcEnd;
    const unsigned short *const *tableToUnicode;
    const Encoding *encodingPtr;
    int state, result, numChars, charLimit = INT_MAX;
    const char *dstStart, *dstEnd;

    if (flags & TCL_ENCODING_CHAR_LIMIT) {
	charLimit = *dstCharsPtr;
    }
    result = TCL_OK;
    tablePrefixBytes = NULL;	/* lint. */
    tableToUnicode = NULL;	/* lint. */
    prefixBytes = dataPtr->prefixBytes;
    encodingPtr = NULL;

    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    state = PTR2INT(*statePtr);
    if (flags & TCL_ENCODING_START) {
	state = 0;
    }

    for (numChars = 0; src < srcEnd && numChars <= charLimit; ) {
	int byte, hi, lo, ch;

	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	byte = *((unsigned char *) src);
	if (prefixBytes[byte]) {
	    unsigned left, len, longest;
	    int checked, i;
	    const EscapeSubTable *subTablePtr;

	    /*
	     * Saw the beginning of an escape sequence.
	     */

	    left = srcEnd - src;
	    len = dataPtr->initLen;
	    longest = len;
	    checked = 0;

	    if (len <= left) {
		checked++;
		if ((len > 0) && (memcmp(src, dataPtr->init, len) == 0)) {
		    /*
		     * If we see initialization string, skip it, even if we're
		     * not at the beginning of the buffer.
		     */

		    src += len;
		    continue;
		}
	    }

	    len = dataPtr->finalLen;
	    if (len > longest) {
		longest = len;
	    }

	    if (len <= left) {
		checked++;
		if ((len > 0) && (memcmp(src, dataPtr->final, len) == 0)) {
		    /*
		     * If we see finalization string, skip it, even if we're
		     * not at the end of the buffer.
		     */

		    src += len;
		    continue;
		}
	    }

	    subTablePtr = dataPtr->subTables;
	    for (i = 0; i < dataPtr->numSubTables; i++) {
		len = subTablePtr->sequenceLen;
		if (len > longest) {
		    longest = len;
		}
		if (len <= left) {
		    checked++;
		    if ((len > 0) &&
			    (memcmp(src, subTablePtr->sequence, len) == 0)) {
			state = i;
			encodingPtr = NULL;
			subTablePtr = NULL;
			src += len;
			break;
		    }
		}
		subTablePtr++;
	    }

	    if (subTablePtr == NULL) {
		/*
		 * A match was found, the escape sequence was consumed, and
		 * the state was updated.
		 */

		continue;
	    }

	    /*
	     * We have a split-up or unrecognized escape sequence. If we
	     * checked all the sequences, then it's a syntax error, otherwise
	     * we need more bytes to determine a match.
	     */

	    if ((checked == dataPtr->numSubTables + 2)
		    || (flags & TCL_ENCODING_END)) {
		if ((flags & TCL_ENCODING_STOPONERROR) == 0) {
		    /*
		     * Skip the unknown escape sequence.
		     */

		    src += longest;
		    continue;
		}
		result = TCL_CONVERT_SYNTAX;
	    } else {
		result = TCL_CONVERT_MULTIBYTE;
	    }
	    break;
	}

	if (encodingPtr == NULL) {
	    TableEncodingData *tableDataPtr;

	    encodingPtr = GetTableEncoding(dataPtr, state);
	    tableDataPtr = encodingPtr->clientData;
	    tablePrefixBytes = tableDataPtr->prefixBytes;
	    tableToUnicode = (const unsigned short *const*)
		    tableDataPtr->toUnicode;
	}

	if (tablePrefixBytes[byte]) {
	    src++;
	    if (src >= srcEnd) {
		src--;
		result = TCL_CONVERT_MULTIBYTE;
		break;
	    }
	    hi = byte;
	    lo = *((unsigned char *) src);
	} else {
	    hi = 0;
	    lo = byte;
	}

	ch = tableToUnicode[hi][lo];
	dst += Tcl_UniCharToUtf(ch, dst);
	src++;
	numChars++;
    }

    *statePtr = (Tcl_EncodingState) INT2PTR(state);
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * EscapeFromUtfProc --
 *
 *	Convert from UTF-8 into the encoding specified by the
 *	EscapeEncodingData.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
EscapeFromUtfProc(
    ClientData clientData,	/* EscapeEncodingData that specifies
				 * encoding. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    EscapeEncodingData *dataPtr = clientData;
    const Encoding *encodingPtr;
    const char *srcStart, *srcEnd, *srcClose;
    const char *dstStart, *dstEnd;
    int state, result, numChars;
    const TableEncodingData *tableDataPtr;
    const char *tablePrefixBytes;
    const unsigned short *const *tableFromUnicode;
    Tcl_UniChar ch = 0;

    result = TCL_OK;

    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - 1;

    /*
     * RFC 1468 states that the text starts in ASCII, and switches to Japanese
     * characters, and that the text must end in ASCII. [Patch 474358]
     */

    if (flags & TCL_ENCODING_START) {
	state = 0;
	if ((dst + dataPtr->initLen) > dstEnd) {
	    *srcReadPtr = 0;
	    *dstWrotePtr = 0;
	    return TCL_CONVERT_NOSPACE;
	}
	memcpy(dst, dataPtr->init, dataPtr->initLen);
	dst += dataPtr->initLen;
    } else {
	state = PTR2INT(*statePtr);
    }

    encodingPtr = GetTableEncoding(dataPtr, state);
    tableDataPtr = encodingPtr->clientData;
    tablePrefixBytes = tableDataPtr->prefixBytes;
    tableFromUnicode = (const unsigned short *const *)
	    tableDataPtr->fromUnicode;

    for (numChars = 0; src < srcEnd; numChars++) {
	unsigned len;
	int word;

	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	len = TclUtfToUniChar(src, &ch);
	word = tableFromUnicode[(ch >> 8)][ch & 0xff];

	if ((word == 0) && (ch != 0)) {
	    int oldState;
	    const EscapeSubTable *subTablePtr;

	    oldState = state;
	    for (state = 0; state < dataPtr->numSubTables; state++) {
		encodingPtr = GetTableEncoding(dataPtr, state);
		tableDataPtr = encodingPtr->clientData;
		word = tableDataPtr->fromUnicode[(ch >> 8)][ch & 0xff];
		if (word != 0) {
		    break;
		}
	    }

	    if (word == 0) {
		state = oldState;
		if (flags & TCL_ENCODING_STOPONERROR) {
		    result = TCL_CONVERT_UNKNOWN;
		    break;
		}
		encodingPtr = GetTableEncoding(dataPtr, state);
		tableDataPtr = encodingPtr->clientData;
		word = tableDataPtr->fallback;
	    }

	    tablePrefixBytes = (const char *) tableDataPtr->prefixBytes;
	    tableFromUnicode = (const unsigned short *const *)
		    tableDataPtr->fromUnicode;

	    /*
	     * The state variable has the value of oldState when word is 0.
	     * In this case, the escape sequence should not be copied to dst
	     * because the current character set is not changed.
	     */

	    if (state != oldState) {
		subTablePtr = &dataPtr->subTables[state];
		if ((dst + subTablePtr->sequenceLen) > dstEnd) {
		    /*
		     * If there is no space to write the escape sequence, the
		     * state variable must be changed to the value of oldState
		     * variable because this escape sequence must be written
		     * in the next conversion.
		     */

		    state = oldState;
		    result = TCL_CONVERT_NOSPACE;
		    break;
		}
		memcpy(dst, subTablePtr->sequence,
			subTablePtr->sequenceLen);
		dst += subTablePtr->sequenceLen;
	    }
	}

	if (tablePrefixBytes[(word >> 8)] != 0) {
	    if (dst + 1 > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) (word >> 8);
	    dst[1] = (char) word;
	    dst += 2;
	} else {
	    if (dst > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) word;
	    dst++;
	}
	src += len;
    }

    if ((result == TCL_OK) && (flags & TCL_ENCODING_END)) {
	unsigned len = dataPtr->subTables[0].sequenceLen;

	/*
	 * Certain encodings like iso2022-jp need to write an escape sequence
	 * after all characters have been converted. This logic checks that
	 * enough room is available in the buffer for the escape bytes. The
	 * TCL_ENCODING_END flag is cleared after a final escape sequence has
	 * been added to the buffer so that another call to this method does
	 * not attempt to append escape bytes a second time.
	 */

	if ((dst + dataPtr->finalLen + (state?len:0)) > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	} else {
	    if (state) {
		memcpy(dst, dataPtr->subTables[0].sequence, len);
		dst += len;
	    }
	    memcpy(dst, dataPtr->final, dataPtr->finalLen);
	    dst += dataPtr->finalLen;
	    state &= ~TCL_ENCODING_END;
	}
    }

    *statePtr = (Tcl_EncodingState) INT2PTR(state);
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * EscapeFreeProc --
 *
 *	This function is invoked when an EscapeEncodingData encoding is
 *	deleted. It deletes the memory used by the encoding.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
EscapeFreeProc(
    ClientData clientData)	/* EscapeEncodingData that specifies
				 * encoding. */
{
    EscapeEncodingData *dataPtr = clientData;
    EscapeSubTable *subTablePtr;
    int i;

    if (dataPtr == NULL) {
	return;
    }

    /*
     * The subTables should be freed recursively in normal operation but not
     * during TclFinalizeEncodingSubsystem because they are also present as a
     * weak reference in the toplevel encodingTable (i.e., they don't have a
     * +1 refcount for this), and unpredictable nuking order could remove them
     * from under the following loop's feet. [Bug 2891556]
     *
     * The encodingsInitialized flag, being reset on entry to TFES, can serve
     * as a "not in finalization" test.
     */

    if (encodingsInitialized) {
	subTablePtr = dataPtr->subTables;
	for (i = 0; i < dataPtr->numSubTables; i++) {
	    FreeEncoding((Tcl_Encoding) subTablePtr->encodingPtr);
	    subTablePtr->encodingPtr = NULL;
	    subTablePtr++;
	}
    }
    ckfree(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTableEncoding --
 *
 *	Helper function for the EscapeEncodingData conversions. Gets the
 *	encoding (of type TextEncodingData) that represents the specified
 *	state.
 *
 * Results:
 *	The return value is the encoding.
 *
 * Side effects:
 *	If the encoding that represents the specified state has not already
 *	been used by this EscapeEncoding, it will be loaded and cached in the
 *	dataPtr.
 *
 *---------------------------------------------------------------------------
 */

static Encoding *
GetTableEncoding(
    EscapeEncodingData *dataPtr,/* Contains names of encodings. */
    int state)			/* Index in dataPtr of desired Encoding. */
{
    EscapeSubTable *subTablePtr = &dataPtr->subTables[state];
    Encoding *encodingPtr = subTablePtr->encodingPtr;

    if (encodingPtr == NULL) {
	encodingPtr = (Encoding *) Tcl_GetEncoding(NULL, subTablePtr->name);
	if ((encodingPtr == NULL)
		|| (encodingPtr->toUtfProc != TableToUtfProc
		&& encodingPtr->toUtfProc != Iso88591ToUtfProc)) {
	    Tcl_Panic("EscapeToUtfProc: invalid sub table");
	}
	subTablePtr->encodingPtr = encodingPtr;
    }

    return encodingPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * unilen --
 *
 *	A helper function for the Tcl_ExternalToUtf functions. This function
 *	is similar to strlen for double-byte characters: it returns the number
 *	of bytes in a 0x0000 terminated string.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static size_t
unilen(
    const char *src)
{
    unsigned short *p;

    p = (unsigned short *) src;
    while (*p != 0x0000) {
	p++;
    }
    return (char *) p - src;
}

/*
 *-------------------------------------------------------------------------
 *
 * InitializeEncodingSearchPath	--
 *
 *	This is the fallback routine that sets the default value of the
 *	encoding search path if the application has not set one via a call to
 *	Tcl_SetEncodingSearchPath() by the first time the search path is needed
 *	to load encoding data.
 *
 *	The default encoding search path is produced by taking each directory
 *	in the library path, appending a subdirectory named "encoding", and if
 *	the resulting directory exists, adding it to the encoding search path.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the encoding search path to an initial value.
 *
 *-------------------------------------------------------------------------
 */

static void
InitializeEncodingSearchPath(
    char **valuePtr,
    int *lengthPtr,
    Tcl_Encoding *encodingPtr)
{
    const char *bytes;
    int i, numDirs, numBytes;
    Tcl_Obj *libPathObj, *encodingObj, *searchPathObj;

    TclNewLiteralStringObj(encodingObj, "encoding");
    TclNewObj(searchPathObj);
    Tcl_IncrRefCount(encodingObj);
    Tcl_IncrRefCount(searchPathObj);
    libPathObj = TclGetLibraryPath();
    Tcl_IncrRefCount(libPathObj);
    Tcl_ListObjLength(NULL, libPathObj, &numDirs);

    for (i = 0; i < numDirs; i++) {
	Tcl_Obj *directoryObj, *pathObj;
	Tcl_StatBuf stat;

	Tcl_ListObjIndex(NULL, libPathObj, i, &directoryObj);
	pathObj = Tcl_FSJoinToPath(directoryObj, 1, &encodingObj);
	Tcl_IncrRefCount(pathObj);
	if ((0 == Tcl_FSStat(pathObj, &stat)) && S_ISDIR(stat.st_mode)) {
	    Tcl_ListObjAppendElement(NULL, searchPathObj, pathObj);
	}
	Tcl_DecrRefCount(pathObj);
    }

    Tcl_DecrRefCount(libPathObj);
    Tcl_DecrRefCount(encodingObj);
    *encodingPtr = libraryPath.encoding;
    if (*encodingPtr) {
	((Encoding *)(*encodingPtr))->refCount++;
    }
    bytes = Tcl_GetStringFromObj(searchPathObj, &numBytes);

    *lengthPtr = numBytes;
    *valuePtr = ckalloc(numBytes + 1);
    memcpy(*valuePtr, bytes, (size_t) numBytes + 1);
    Tcl_DecrRefCount(searchPathObj);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
