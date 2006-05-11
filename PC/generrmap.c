#include <stdio.h>
#include <errno.h>

/* Extract the mapping of Win32 error codes to errno */

int main()
{
	int i;
	printf("/* Generated file. Do not edit. */\n");
	printf("int winerror_to_errno(int winerror)\n");
	printf("{\n\tswitch(winerror) {\n");
	for(i=1; i < 65000; i++) {
		_dosmaperr(i);
		if (errno == EINVAL)
			continue;
		printf("\t\tcase %d: return %d;\n", i, errno);
	}
	printf("\t\tdefault: return EINVAL;\n");
	printf("\t}\n}\n");
}
