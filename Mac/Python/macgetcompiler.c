/* Return a string representing the compiler name */

#ifdef THINK_C
#define COMPILER " [THINK C]"
#endif

#ifdef __MWERKS__
#ifdef __powerc
#define COMPILER " [CW PPC]"
#else
#ifdef __CFM68K__
#define COMPILER " [CW CFM68K]"
#else
#define COMPILER " [CW 68K]"
#endif
#endif
#endif

#ifdef MPW
#ifdef __SC__
#define COMPILER " [Symantec MPW]"
#else
#define COMPILER " [Apple MPW]"
#endif
#endif

char *
getcompiler()
{
	return COMPILER;
}
