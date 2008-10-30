#include <Python.h>

#if 0
char* cmd = "import codecs, encodings.utf_8, types; print(types)";
#else
char* cmd = "import types; print(types)";
#endif

int main()
{
	printf("Initialize interpreter\n");
	Py_Initialize();
	PyEval_InitThreads();
	PyRun_SimpleString(cmd);
	Py_EndInterpreter(PyThreadState_Get());

	printf("\nInitialize subinterpreter\n");
	Py_NewInterpreter();
	PyRun_SimpleString(cmd);
	Py_Finalize();

	return 0;
}
