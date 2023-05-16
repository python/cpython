Tests in this directory are compiled into the _testcapi extension.
The main file for the extension is Modules/_testcapimodule.c, which
calls `_PyTestCapi_Init_*` from these functions.

General guideline for writing test C API.
* Use clinic tool as possible.
* Add a newline between the the argument spec and the docstring.
* If a test decription is needed, make sure the added docstring clearly and succinctly describes purpose of the function.
* DRY, use the clone feature of Argument Clinic.
* Try to avoid adding new interned strings; reuse existing parameter names if possible and use the `as` feature to override the C name.
