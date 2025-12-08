Tests in this directory are compiled into the _testinternalcapi extension.
The main file for the extension is Modules/_testinternalcapimodule.c, which
calls `_PyTestInternalCapi_Init_*` from these functions.

See Modules/_testcapi/README.txt for guideline when writing C test code.