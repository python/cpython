/*
** mac __start for python-with-shared-library.
**
** Partially stolen from MW Startup.c, which is
**	Copyright © 1993 metrowerks inc. All Rights Reserved.
*/

#include <setjmp.h>

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
	
	clear_stackframe_backlink();
	main(0, &argv);
	exit(0);
}
