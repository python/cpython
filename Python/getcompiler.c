#ifdef __GNUC__
#define COMPILER " [GCC " __VERSION__ "]"
#endif

#ifndef COMPILER
#ifdef __cplusplus
#define COMPILER "[C++]"
#else
#define COMPILER "[C]"
#endif
#endif

char *
getcompiler()
{
	return COMPILER;
}
