/*
    This is the entry point for Python DLL(s).
    It also provides an getenv() function that works from within DLLs.
*/

#define NULL 0

/* Make references to imported symbols to pull them from static library */
#define REF(s)	extern void s (); void *____ref_##s = &s;

REF (Py_Main);

#include <signal.h>

extern int _CRT_init (void);
extern void _CRT_term (void);
extern void __ctordtorInit (void);
extern void __ctordtorTerm (void);

unsigned long _DLL_InitTerm (unsigned long mod_handle, unsigned long flag)
{
	switch (flag)
	{
		case 0:
			if (_CRT_init ())
				return 0;
			__ctordtorInit ();

			/* Ignore fatal signals */
			signal (SIGSEGV, SIG_IGN);
			signal (SIGFPE, SIG_IGN);

			return 1;

		case 1:
			__ctordtorTerm ();
			_CRT_term ();
			return 1;

		default:
			return 0;
	}
}
