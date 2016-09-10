#include <Python.h>
#include "pydtrace.h"

#ifndef WITH_DTRACE
extern inline void PyDTrace_LINE(const char *arg0, const char *arg1, int arg2);
extern inline void PyDTrace_FUNCTION_ENTRY(const char *arg0, const char *arg1, int arg2);
extern inline void PyDTrace_FUNCTION_RETURN(const char *arg0, const char *arg1, int arg2);
extern inline void PyDTrace_GC_START(int arg0);
extern inline void PyDTrace_GC_DONE(int arg0);
extern inline void PyDTrace_INSTANCE_NEW_START(int arg0);
extern inline void PyDTrace_INSTANCE_NEW_DONE(int arg0);
extern inline void PyDTrace_INSTANCE_DELETE_START(int arg0);
extern inline void PyDTrace_INSTANCE_DELETE_DONE(int arg0);

extern inline int PyDTrace_LINE_ENABLED(void);
extern inline int PyDTrace_FUNCTION_ENTRY_ENABLED(void);
extern inline int PyDTrace_FUNCTION_RETURN_ENABLED(void);
extern inline int PyDTrace_GC_START_ENABLED(void);
extern inline int PyDTrace_GC_DONE_ENABLED(void);
extern inline int PyDTrace_INSTANCE_NEW_START_ENABLED(void);
extern inline int PyDTrace_INSTANCE_NEW_DONE_ENABLED(void);
extern inline int PyDTrace_INSTANCE_DELETE_START_ENABLED(void);
extern inline int PyDTrace_INSTANCE_DELETE_DONE_ENABLED(void);
#endif
