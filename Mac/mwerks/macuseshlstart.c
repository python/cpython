/*
** mac __start for python-with-shared-library.
**
** Partially stolen from MW Startup.c, which is
**	Copyright © 1993 metrowerks inc. All Rights Reserved.
*/

#include <setjmp.h>

#ifdef PRE_CW8
extern jmp_buf __program_exit;			/*	exit() does a longjmp() to here		*/
extern void (*__atexit_hook)(void);		/*	atexit()  sets this up if it is ever called	*/
extern void (*___atexit_hook)(void);	/*	_atexit() sets this up if it is ever called	*/
extern int __aborting;					/*	abort() sets this and longjmps to __program_exit	*/
#endif

/*
 *	clear_stackframe_backlink	-	set 0(SP) to 0
 *
 */

static asm void clear_stackframe_backlink(void)
{
		li		r3,0
		stw		r3,0(SP)
		blr
}

void __start(void)
{
	char *argv = 0;
	
#ifdef PRE_CW8
	if (setjmp(__program_exit) == 0) {	//	set up jmp_buf for exit()
		main(0, &argv);				//	call main(argc, argv)
		if (__atexit_hook)
			__atexit_hook();			//	call atexit() procs
	}
	if (!__aborting) {
		if (___atexit_hook)
			___atexit_hook();			//	call _atexit() procs
	}
//	ExitToShell();
#else
	clear_stackframe_backlink();
	main(0, &argv);
	exit(0);
#endif
}
