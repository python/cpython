
/* Support for dynamic loading of extension modules */

#include <windows.h>
#include <direct.h>
#include <ctype.h>

#include "Python.h"
#include "importdl.h"

const struct filedescr _PyImport_DynLoadFiletab[] = {
#ifdef _DEBUG
	{"_d.pyd", "rb", C_EXTENSION},
	{"_d.dll", "rb", C_EXTENSION},
#else
	{".pyd", "rb", C_EXTENSION},
	{".dll", "rb", C_EXTENSION},
#endif
	{0, 0}
};


/* Case insensitive string compare, to avoid any dependencies on particular
   C RTL implementations */

static int strcasecmp (char *string1, char *string2)
{ 
	int first, second;

	do {
		first  = tolower(*string1);
		second = tolower(*string2);
		string1++;
		string2++;
	} while (first && first == second);

	return (first - second);
} 


/* Function to return the name of the "python" DLL that the supplied module
   directly imports.  Looks through the list of imported modules and
   returns the first entry that starts with "python" (case sensitive) and
   is followed by nothing but numbers until the separator (period).

   Returns a pointer to the import name, or NULL if no matching name was
   located.

   This function parses through the PE header for the module as loaded in
   memory by the system loader.  The PE header is accessed as documented by
   Microsoft in the MSDN PE and COFF specification (2/99), and handles
   both PE32 and PE32+.  It only worries about the direct import table and
   not the delay load import table since it's unlikely an extension is
   going to be delay loading Python (after all, it's already loaded).

   If any magic values are not found (e.g., the PE header or optional
   header magic), then this function simply returns NULL. */

#define DWORD_AT(mem) (*(DWORD *)(mem))
#define WORD_AT(mem)  (*(WORD *)(mem))

static char *GetPythonImport (HINSTANCE hModule)
{
	unsigned char *dllbase, *import_data, *import_name;
	DWORD pe_offset, opt_offset;
	WORD opt_magic;
	int num_dict_off, import_off;

	/* Safety check input */
	if (hModule == NULL) {
		return NULL;
	}

	/* Module instance is also the base load address.  First portion of
	   memory is the MS-DOS loader, which holds the offset to the PE
	   header (from the load base) at 0x3C */
	dllbase = (unsigned char *)hModule;
	pe_offset = DWORD_AT(dllbase + 0x3C);

	/* The PE signature must be "PE\0\0" */
	if (memcmp(dllbase+pe_offset,"PE\0\0",4)) {
		return NULL;
	}

	/* Following the PE signature is the standard COFF header (20
	   bytes) and then the optional header.  The optional header starts
	   with a magic value of 0x10B for PE32 or 0x20B for PE32+ (PE32+
	   uses 64-bits for some fields).  It might also be 0x107 for a ROM
	   image, but we don't process that here.

	   The optional header ends with a data dictionary that directly
	   points to certain types of data, among them the import entries
	   (in the second table entry). Based on the header type, we
	   determine offsets for the data dictionary count and the entry
	   within the dictionary pointing to the imports. */

	opt_offset = pe_offset + 4 + 20;
	opt_magic = WORD_AT(dllbase+opt_offset);
	if (opt_magic == 0x10B) {
		/* PE32 */
		num_dict_off = 92;
		import_off   = 104;
	} else if (opt_magic == 0x20B) {
		/* PE32+ */
		num_dict_off = 108;
		import_off   = 120;
	} else {
		/* Unsupported */
		return NULL;
	}

	/* Now if an import table exists, offset to it and walk the list of
	   imports.  The import table is an array (ending when an entry has
	   empty values) of structures (20 bytes each), which contains (at
	   offset 12) a relative address (to the module base) at which a
	   string constant holding the import name is located. */

	if (DWORD_AT(dllbase + opt_offset + num_dict_off) >= 2) {
		/* We have at least 2 tables - the import table is the second
		   one.  But still it may be that the table size is zero */
		if (0 == DWORD_AT(dllbase + opt_offset + import_off + sizeof(DWORD)))
			return NULL;
		import_data = dllbase + DWORD_AT(dllbase +
						 opt_offset +
						 import_off);
		while (DWORD_AT(import_data)) {
			import_name = dllbase + DWORD_AT(import_data+12);
			if (strlen(import_name) >= 6 &&
			    !strncmp(import_name,"python",6)) {
				char *pch;

				/* Ensure python prefix is followed only
				   by numbers to the end of the basename */
				pch = import_name + 6;
#ifdef _DEBUG
				while (*pch && pch[0] != '_' && pch[1] != 'd' && pch[2] != '.') {
#else
				while (*pch && *pch != '.') {
#endif
					if (*pch >= '0' && *pch <= '9') {
						pch++;
					} else {
						pch = NULL;
						break;
					}
				}
	    
				if (pch) {
					/* Found it - return the name */
					return import_name;
				}
			}
			import_data += 20;
		}
	}

	return NULL;
}


dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;
	char funcname[258], *import_python;

	PyOS_snprintf(funcname, sizeof(funcname), "init%.200s", shortname);

	{
		HINSTANCE hDLL = NULL;
		char pathbuf[260];
		LPTSTR dummy;
		/* We use LoadLibraryEx so Windows looks for dependent DLLs 
		    in directory of pathname first.  However, Windows95
		    can sometimes not work correctly unless the absolute
		    path is used.  If GetFullPathName() fails, the LoadLibrary
		    will certainly fail too, so use its error code */
		if (GetFullPathName(pathname,
				    sizeof(pathbuf),
				    pathbuf,
				    &dummy))
			/* XXX This call doesn't exist in Windows CE */
			hDLL = LoadLibraryEx(pathname, NULL,
					     LOAD_WITH_ALTERED_SEARCH_PATH);
		if (hDLL==NULL){
			char errBuf[256];
			unsigned int errorCode;

			/* Get an error string from Win32 error code */
			char theInfo[256]; /* Pointer to error text
					      from system */
			int theLength; /* Length of error text */

			errorCode = GetLastError();

			theLength = FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM, /* flags */
				NULL, /* message source */
				errorCode, /* the message (error) ID */
				0, /* default language environment */
				(LPTSTR) theInfo, /* the buffer */
				sizeof(theInfo), /* the buffer size */
				NULL); /* no additional format args. */

			/* Problem: could not get the error message.
			   This should not happen if called correctly. */
			if (theLength == 0) {
				PyOS_snprintf(errBuf, sizeof(errBuf),
				      "DLL load failed with error code %d",
					      errorCode);
			} else {
				size_t len;
				/* For some reason a \r\n
				   is appended to the text */
				if (theLength >= 2 &&
				    theInfo[theLength-2] == '\r' &&
				    theInfo[theLength-1] == '\n') {
					theLength -= 2;
					theInfo[theLength] = '\0';
				}
				strcpy(errBuf, "DLL load failed: ");
				len = strlen(errBuf);
				strncpy(errBuf+len, theInfo,
					sizeof(errBuf)-len);
				errBuf[sizeof(errBuf)-1] = '\0';
			}
			PyErr_SetString(PyExc_ImportError, errBuf);
			return NULL;
		} else {
			char buffer[256];

#ifdef _DEBUG
			PyOS_snprintf(buffer, sizeof(buffer), "python%d%d_d.dll",
#else
			PyOS_snprintf(buffer, sizeof(buffer), "python%d%d.dll",
#endif
				      PY_MAJOR_VERSION,PY_MINOR_VERSION);
			import_python = GetPythonImport(hDLL);

			if (import_python &&
			    strcasecmp(buffer,import_python)) {
				PyOS_snprintf(buffer, sizeof(buffer),
					      "Module use of %.150s conflicts "
					      "with this version of Python.",
					      import_python);
				PyErr_SetString(PyExc_ImportError,buffer);
				FreeLibrary(hDLL);
				return NULL;
			}
		}
		p = GetProcAddress(hDLL, funcname);
	}

	return p;
}
