
/* Support for dynamic loading of extension modules */

#define  INCL_DOSERRORS
#define  INCL_DOSMODULEMGR
#include <os2.h>

#include "Python.h"
#include "importdl.h"


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{".pyd", "rb", C_EXTENSION},
	{".dll", "rb", C_EXTENSION},
	{0, 0}
};

dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;
	APIRET  rc;
	HMODULE hDLL;
	char failreason[256];
	char funcname[258];

	rc = DosLoadModule(failreason,
			   sizeof(failreason),
			   pathname,
			   &hDLL);

	if (rc != NO_ERROR) {
		char errBuf[256];
		sprintf(errBuf,
			"DLL load failed, rc = %d: %s",
			rc, failreason);
		PyErr_SetString(PyExc_ImportError, errBuf);
		return NULL;
	}

	sprintf(funcname, "init%.200s", shortname);
	rc = DosQueryProcAddr(hDLL, 0L, funcname, &p);
	if (rc != NO_ERROR)
		p = NULL; /* Signify Failure to Acquire Entrypoint */
	return p;
}
