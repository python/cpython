Issue #27122: When an exception is raised within the context being managed
by a contextlib.ExitStack() and one of the exit stack generators
catches and raises it in a chain, do not re-raise the original exception
when exiting, let the new chained one through.  This avoids the PEP 479
bug described in issue25782.