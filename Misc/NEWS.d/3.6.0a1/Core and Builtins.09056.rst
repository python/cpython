Issue #23564: Fixed a partially broken sanity check in the _posixsubprocess
internals regarding how fds_to_pass were passed to the child.  The bug had
no actual impact as subprocess.py already avoided it.