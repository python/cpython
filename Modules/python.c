/* Minimal main program -- everything is loaded from the library */

#include "Python.h"
#include <locale.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#ifdef MS_WINDOWS
int
wmain(int argc, wchar_t **argv)
{
	return Py_Main(argc, argv);
}
#else
int
main(int argc, char **argv)
{
	wchar_t **argv_copy = PyMem_Malloc(sizeof(wchar_t*)*argc);
	/* We need a second copies, as Python might modify the first one. */
	wchar_t **argv_copy2 = PyMem_Malloc(sizeof(wchar_t*)*argc);
	int i, res;
	char *oldloc;
	/* 754 requires that FP exceptions run in "no stop" mode by default,
	 * and until C vendors implement C99's ways to control FP exceptions,
	 * Python requires non-stop mode.  Alas, some platforms enable FP
	 * exceptions by default.  Here we disable them.
	 */
#ifdef __FreeBSD__
	fp_except_t m;

	m = fpgetmask();
	fpsetmask(m & ~FP_X_OFL);
#endif
	if (!argv_copy || !argv_copy2) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	oldloc = setlocale(LC_ALL, NULL);
	setlocale(LC_ALL, "");
	for (i = 0; i < argc; i++) {
#ifdef HAVE_BROKEN_MBSTOWCS
		/* Some platforms have a broken implementation of
		 * mbstowcs which does not count the characters that
		 * would result from conversion.  Use an upper bound.
		 */
		size_t argsize = strlen(argv[i]);
#else
		size_t argsize = mbstowcs(NULL, argv[i], 0);
#endif
		size_t count;
		if (argsize == (size_t)-1) {
			fprintf(stderr, "Could not convert argument %d to string\n", i);
			return 1;
		}
		argv_copy[i] = PyMem_Malloc((argsize+1)*sizeof(wchar_t));
		argv_copy2[i] = argv_copy[i];
		if (!argv_copy[i]) {
			fprintf(stderr, "out of memory\n");
			return 1;
		}
		count = mbstowcs(argv_copy[i], argv[i], argsize+1);
		if (count == (size_t)-1) {
			fprintf(stderr, "Could not convert argument %d to string\n", i);
			return 1;
		}
	}
	setlocale(LC_ALL, oldloc);
	res = Py_Main(argc, argv_copy);
	for (i = 0; i < argc; i++) {
		PyMem_Free(argv_copy2[i]);
	}
	PyMem_Free(argv_copy);
	PyMem_Free(argv_copy2);
	return res;
}
#endif
