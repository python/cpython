/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(pysqlite_connect__doc__,
"connect($module, /, database, timeout=5.0, detect_types=0,\n"
"        isolation_level=\'\', check_same_thread=True,\n"
"        factory=ConnectionType, cached_statements=128, uri=False, *,\n"
"        autocommit=sqlite3.LEGACY_TRANSACTION_CONTROL)\n"
"--\n"
"\n"
"Open a connection to the SQLite database file \'database\'.\n"
"\n"
"You can use \":memory:\" to open a database connection to a database that\n"
"resides in RAM instead of on disk.");

#define PYSQLITE_CONNECT_METHODDEF    \
    {"connect", _PyCFunction_CAST(pysqlite_connect), METH_FASTCALL|METH_KEYWORDS, pysqlite_connect__doc__},
/*[clinic end generated code: output=6a8458c9edf8fb7f input=a9049054013a1b77]*/
