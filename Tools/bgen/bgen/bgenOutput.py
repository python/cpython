"""Output primitives for the binding generator classes.

This should really be a class, but then everybody would be passing
the output object to each other.  I chose for the simpler approach
of a module with a global variable.  Use SetOutputFile() to change
the output file.
"""

def SetOutputFile(file = None):
	"""Call this with an open file object to make that the output file.

	Call it without arguments to reset the output file to sys.stdout.
	"""
	global _File
	if file is None:
		import sys
		file = sys.stdout
	_File = file

SetOutputFile()	# Initialize _File

_Level = 0		# Indentation level

def GetLevel():
	""""Return the current indentation level."""
	return _Level

def SetLevel(level):
	"""Set the current indentation level.

	This does no type or range checking -- use at own risk.
	"""
	global _Level
	_Level = level

def Output(format = "", *args):
	"""Call this with a format string and arguments for the format.

	A newline is always added.  Each line in the output is indented
	to the proper indentation level -- even if the result of the
	format expansion contains embedded newlines.  Exception: lines
	beginning with '#' are not indented -- these are assumed to be
	C preprprocessor lines.
	"""
	text = format % args
	if _Level > 0:
		indent = '\t' * _Level
		import string
		lines = string.splitfields(text, '\n')
		for i in range(len(lines)):
			if lines[i] and lines[i][0] != '#':
				lines[i] = indent + lines[i]
		text = string.joinfields(lines, '\n')
	_File.write(text + '\n')

def IndentLevel(by = 1):
	"""Increment the indentation level by one.

	When called with an argument, adds it to the indentation level.
	"""
	global _Level
	if _Level+by < 0:
		raise Error, "indentation underflow (internal error)"
	_Level = _Level + by

def DedentLevel(by = 1):
	"""Decfrement the indentation level by one.

	When called with an argument, subtracts it from the indentation level.
	"""
	IndentLevel(-by)

def OutLbrace():
	"""Output a '{' on a line by itself and increase the indentation level."""
	Output("{")
	IndentLevel()

def OutRbrace():
	"""Decrease the indentation level and output a '}' on a line by itself."""
	DedentLevel()
	Output("}")

def OutHeader(text, dash):
	"""Output a header comment using a given dash character."""
	n = 64 - len(text)
	Output()
	Output("/* %s %s %s */", dash * (n/2), text, dash * (n - n/2))
	Output()

def OutHeader1(text):
	"""Output a level 1 header comment (uses '=' dashes)."""
	OutHeader(text, "=")

def OutHeader2(text):
	"""Output a level 2 header comment (uses '-' dashes)."""
	OutHeader(text, "-")


def _test():
	"""Test program.  Run when the module is run as a script."""
	OutHeader1("test bgenOutput")
	Output("""
#include <Python.h>
#include <stdio.h>
""")
	Output("main(argc, argv)")
	IndentLevel()
	Output("int argc;")
	Output("char **argv;")
	DedentLevel()
	OutLbrace()
	Output("int i;")
	Output()
	Output("""\
/* Here are a few comment lines.
   Just to test indenting multiple lines.

   End of the comment lines. */
""")
	Output("for (i = 0; i < argc; i++)")
	OutLbrace()
	Output('printf("argv[%%d] = %%s\\n", i, argv[i]);')
	OutRbrace()
	Output("exit(0)")
	OutRbrace()
	OutHeader2("end test")

if __name__ == '__main__':
	_test()
