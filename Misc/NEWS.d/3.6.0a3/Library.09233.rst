Issue #22115: Added methods trace_add, trace_remove and trace_info in the
tkinter.Variable class.  They replace old methods trace_variable, trace,
trace_vdelete and trace_vinfo that use obsolete Tcl commands and might
not work in future versions of Tcl.  Fixed old tracing methods:
trace_vdelete() with wrong mode no longer break tracing, trace_vinfo() now
always returns a list of pairs of strings, tracing in the "u" mode now works.