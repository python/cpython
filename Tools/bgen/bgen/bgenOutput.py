"""Output primitives for the binding generator classes.

This should really be a class, but then everybody would be passing
the output object to each other.  I chose for the simpler approach
of a module with a global variable.  Use SetOutputFile() or
SetOutputFileName() to change the output file.
"""

_NeedClose = 0

def SetOutputFile(file = None, needclose = 0):
    """Call this with an open file object to make it the output file.

    Call it without arguments to close the current file (if necessary)
    and reset it to sys.stdout.
    If the second argument is true, the new file will be explicitly closed
    on a subsequence call.
    """
    global _File, _NeedClose
    if _NeedClose:
        tmp = _File
        _NeedClose = 0
        _File = None
        tmp.close()
    if file is None:
        import sys
        file = sys.stdout
    _File = file
    _NeedClose = file and needclose

def SetOutputFileName(filename = None):
    """Call this with a filename to make it the output file.

    Call it without arguments to close the current file (if necessary)
    and reset it to sys.stdout.
    """
    SetOutputFile()
    if filename:
        SetOutputFile(open(filename, 'w'), 1)

SetOutputFile() # Initialize _File

_Level = 0      # Indentation level

def GetLevel():
    """Return the current indentation level."""
    return _Level

def SetLevel(level):
    """Set the current indentation level.

    This does no type or range checking -- use at own risk.
    """
    global _Level
    _Level = level

def Output(format = "", *args):
    VaOutput(format, args)

def VaOutput(format, args):
    """Call this with a format string and argument tuple for the format.

    A newline is always added.  Each line in the output is indented
    to the proper indentation level -- even if the result of the
    format expansion contains embedded newlines.  Exception: lines
    beginning with '#' are not indented -- these are assumed to be
    C preprprocessor lines.
    """
    text = format % args
    if _Level > 0:
        indent = '\t' * _Level
        lines = text.split('\n')
        for i in range(len(lines)):
            if lines[i] and lines[i][0] != '#':
                lines[i] = indent + lines[i]
        text = '\n'.join(lines)
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
    """Decrement the indentation level by one.

    When called with an argument, subtracts it from the indentation level.
    """
    IndentLevel(-by)

def OutIndent(format = "", *args):
    """Combine Output() followed by IndentLevel().

    If no text is given, acts like lone IndentLevel().
    """
    if format: VaOutput(format, args)
    IndentLevel()

def OutDedent(format = "", *args):
    """Combine Output() followed by DedentLevel().

    If no text is given, acts like loneDedentLevel().
    """
    if format: VaOutput(format, args)
    DedentLevel()

def OutLbrace(format = "", *args):
    """Like Output, but add a '{' and increase the indentation level.

    If no text is given a lone '{' is output.
    """
    if format:
        format = format + " {"
    else:
        format = "{"
    VaOutput(format, args)
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

def Out(text):
    """Output multiline text that's internally indented.

    Pass this a multiline character string.  The whitespace before the
    first nonblank line of the string will be subtracted from all lines.
    The lines are then output using Output(), but without interpretation
    of formatting (if you need formatting you can do it before the call).
    Recommended use:

        Out('''
            int main(argc, argv)
                int argc;
                char *argv;
            {
                printf("Hello, world\\n");
                exit(0);
            }
        ''')

    Caveat: the indentation must be consistent -- if you use three tabs
    in the first line, (up to) three tabs are removed from following lines,
    but a line beginning with 24 spaces is not trimmed at all.  Don't use
    this as a feature.
    """
    # (Don't you love using triple quotes *inside* triple quotes? :-)

    lines = text.split('\n')
    indent = ""
    for line in lines:
        if line.strip():
            for c in line:
                if not c.isspace():
                    break
                indent = indent + c
            break
    n = len(indent)
    for line in lines:
        if line[:n] == indent:
            line = line[n:]
        else:
            for c in indent:
                if line[:1] <> c: break
                line = line[1:]
        VaOutput("%s", line)


def _test():
    """Test program.  Run when the module is run as a script."""
    OutHeader1("test bgenOutput")
    Out("""
        #include <Python.h>
        #include <stdio.h>

        main(argc, argv)
            int argc;
            char **argv;
        {
            int i;
    """)
    IndentLevel()
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
