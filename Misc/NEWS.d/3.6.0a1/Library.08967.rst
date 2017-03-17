Issue #23848: On Windows, faulthandler.enable() now also installs an
exception handler to dump the traceback of all Python threads on any Windows
exception, not only on UNIX signals (SIGSEGV, SIGFPE, SIGABRT).