/* Return a string representing the compiler name */

#ifdef THINK_C
#define COMPILER " [THINK C]"
#endif

#ifdef __MWERKS__
#ifdef USE_GUSI
#define HASGUSI " w/GUSI"
#else
#define HASGUSI ""
#endif
#ifdef __powerc
#define COMPILER " [CW PPC" HASGUSI "]"
#else
#ifdef __CFM68K__
#define COMPILER " [CW CFM68K" HASGUSI "]"
#else
#define COMPILER " [CW 68K" HASGUSI "]"
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
Py_GetCompiler()
{
	return COMPILER;
}
