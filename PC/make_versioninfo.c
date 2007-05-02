#include <stdio.h>
#include "patchlevel.h"
/*
 * This program prints out an include file containing fields required to build
 * the version info resource of pythonxx.dll because the resource compiler
 * cannot do the arithmetic.
 */
/*
 * FIELD3 is the third field of the version number.
 * This is what we'd like FIELD3 to be:
 *
 * #define FIELD3 (PY_MICRO_VERSION*1000 + PY_RELEASE_LEVEL*10 + PY_RELEASE_SERIAL)
 *
 * but that neither gives an error nor comes anywhere close to working.
 *
 * For 2.4a0,
 * PY_MICRO_VERSION = 0
 * PY_RELEASE_LEVEL = 'alpha' = 0xa
 * PY_RELEASE_SERIAL = 0
 *
 * gives FIELD3 = 0*1000 + 10*10 + 0 = 100
 */
int main(int argc, char **argv)
{
	printf("/* This file created by make_versioninfo.exe */\n");
	printf("#define FIELD3 %d\n",
		PY_MICRO_VERSION*1000 + PY_RELEASE_LEVEL*10 + PY_RELEASE_SERIAL);
	printf("#define MS_DLL_ID \"%d.%d\"\n",
	       PY_MAJOR_VERSION, PY_MINOR_VERSION);
	printf("#ifndef _DEBUG\n");
	printf("#define PYTHON_DLL_NAME \"python%d%d.dll\"\n",
	       PY_MAJOR_VERSION, PY_MINOR_VERSION);
	printf("#else\n");
	printf("#define PYTHON_DLL_NAME \"python%d%d_d.dll\"\n",
	       PY_MAJOR_VERSION, PY_MINOR_VERSION);
	printf("#endif\n");
	return 0;
}
