Tests in this directory are compiled into the _testcapi extension.
The main file for the extension is Modules/_testcapimodule.c, which
calls `_PyTestCapi_Init_*` from these functions.
