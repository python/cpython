#include <Python.h>

char* cmd = "import exceptions";

int main()
{
	Py_Initialize();
	PyEval_InitThreads();
	PyRun_SimpleString(cmd);
	Py_EndInterpreter(PyThreadState_Get());

	Py_NewInterpreter();
	PyRun_SimpleString(cmd);
	Py_Finalize();

	return 0;
}
