Tests in this directory are compiled into the _testcapi extension.
The main file for the extension is Modules/_testcapimodule.c, which
calls `_PyTestCapi_Init_*` from these functions.

General guideline when writing test code for C API.
* Use Argument Clinic to minimise the amount of boilerplate code.
* Add a newline between the argument spec and the docstring.
* If a test description is needed, make sure the added docstring clearly and succinctly describes purpose of the function.
* DRY, use the clone feature of Argument Clinic.
* Try to avoid adding new interned strings; reuse existing parameter names if possible. Use the `as` feature of Argument Clinic to override the C variable name, if needed.
