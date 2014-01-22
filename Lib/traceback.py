"""Extract, format and print information about Python stack traces."""

import linecache
import sys
import operator

__all__ = ['extract_stack', 'extract_tb', 'format_exception',
           'format_exception_only', 'format_list', 'format_stack',
           'format_tb', 'print_exc', 'format_exc', 'print_exception',
           'print_last', 'print_stack', 'print_tb',
           'clear_frames']

#
# Formatting and printing lists of traceback lines.
#

def _format_list_iter(extracted_list):
    for filename, lineno, name, line in extracted_list:
        item = '  File "{}", line {}, in {}\n'.format(filename, lineno, name)
        if line:
            item = item + '    {}\n'.format(line.strip())
        yield item

def print_list(extracted_list, file=None):
    """Print the list of tuples as returned by extract_tb() or
    extract_stack() as a formatted stack trace to the given file."""
    if file is None:
        file = sys.stderr
    for item in _format_list_iter(extracted_list):
        print(item, file=file, end="")

def format_list(extracted_list):
    """Format a list of traceback entry tuples for printing.

    Given a list of tuples as returned by extract_tb() or
    extract_stack(), return a list of strings ready for printing.
    Each string in the resulting list corresponds to the item with the
    same index in the argument list.  Each string ends in a newline;
    the strings may contain internal newlines as well, for those items
    whose source text line is not None.
    """
    return list(_format_list_iter(extracted_list))

#
# Printing and Extracting Tracebacks.
#

# extractor takes curr and needs to return a tuple of:
# - Frame object
# - Line number
# - Next item (same type as curr)
# In practice, curr is either a traceback or a frame.
def _extract_tb_or_stack_iter(curr, limit, extractor):
    if limit is None:
        limit = getattr(sys, 'tracebacklimit', None)

    n = 0
    while curr is not None and (limit is None or n < limit):
        f, lineno, next_item = extractor(curr)
        co = f.f_code
        filename = co.co_filename
        name = co.co_name

        linecache.checkcache(filename)
        line = linecache.getline(filename, lineno, f.f_globals)

        if line:
            line = line.strip()
        else:
            line = None

        yield (filename, lineno, name, line)
        curr = next_item
        n += 1

def _extract_tb_iter(tb, limit):
    return _extract_tb_or_stack_iter(
                tb, limit,
                operator.attrgetter("tb_frame", "tb_lineno", "tb_next"))

def print_tb(tb, limit=None, file=None):
    """Print up to 'limit' stack trace entries from the traceback 'tb'.

    If 'limit' is omitted or None, all entries are printed.  If 'file'
    is omitted or None, the output goes to sys.stderr; otherwise
    'file' should be an open file or file-like object with a write()
    method.
    """
    print_list(extract_tb(tb, limit=limit), file=file)

def format_tb(tb, limit=None):
    """A shorthand for 'format_list(extract_tb(tb, limit))'."""
    return format_list(extract_tb(tb, limit=limit))

def extract_tb(tb, limit=None):
    """Return list of up to limit pre-processed entries from traceback.

    This is useful for alternate formatting of stack traces.  If
    'limit' is omitted or None, all entries are extracted.  A
    pre-processed stack trace entry is a quadruple (filename, line
    number, function name, text) representing the information that is
    usually printed for a stack trace.  The text is a string with
    leading and trailing whitespace stripped; if the source is not
    available it is None.
    """
    return list(_extract_tb_iter(tb, limit=limit))

#
# Exception formatting and output.
#

_cause_message = (
    "\nThe above exception was the direct cause "
    "of the following exception:\n")

_context_message = (
    "\nDuring handling of the above exception, "
    "another exception occurred:\n")

def _iter_chain(exc, custom_tb=None, seen=None):
    if seen is None:
        seen = set()
    seen.add(exc)
    its = []
    context = exc.__context__
    cause = exc.__cause__
    if cause is not None and cause not in seen:
        its.append(_iter_chain(cause, False, seen))
        its.append([(_cause_message, None)])
    elif (context is not None and
          not exc.__suppress_context__ and
          context not in seen):
        its.append(_iter_chain(context, None, seen))
        its.append([(_context_message, None)])
    its.append([(exc, custom_tb or exc.__traceback__)])
    # itertools.chain is in an extension module and may be unavailable
    for it in its:
        yield from it

def _format_exception_iter(etype, value, tb, limit, chain):
    if chain:
        values = _iter_chain(value, tb)
    else:
        values = [(value, tb)]

    for value, tb in values:
        if isinstance(value, str):
            # This is a cause/context message line
            yield value + '\n'
            continue
        if tb:
            yield 'Traceback (most recent call last):\n'
            yield from _format_list_iter(_extract_tb_iter(tb, limit=limit))
        yield from _format_exception_only_iter(type(value), value)

def print_exception(etype, value, tb, limit=None, file=None, chain=True):
    """Print exception up to 'limit' stack trace entries from 'tb' to 'file'.

    This differs from print_tb() in the following ways: (1) if
    traceback is not None, it prints a header "Traceback (most recent
    call last):"; (2) it prints the exception type and value after the
    stack trace; (3) if type is SyntaxError and value has the
    appropriate format, it prints the line where the syntax error
    occurred with a caret on the next line indicating the approximate
    position of the error.
    """
    if file is None:
        file = sys.stderr
    for line in _format_exception_iter(etype, value, tb, limit, chain):
        print(line, file=file, end="")

def format_exception(etype, value, tb, limit=None, chain=True):
    """Format a stack trace and the exception information.

    The arguments have the same meaning as the corresponding arguments
    to print_exception().  The return value is a list of strings, each
    ending in a newline and some containing internal newlines.  When
    these lines are concatenated and printed, exactly the same text is
    printed as does print_exception().
    """
    return list(_format_exception_iter(etype, value, tb, limit, chain))

def format_exception_only(etype, value):
    """Format the exception part of a traceback.

    The arguments are the exception type and value such as given by
    sys.last_type and sys.last_value. The return value is a list of
    strings, each ending in a newline.

    Normally, the list contains a single string; however, for
    SyntaxError exceptions, it contains several lines that (when
    printed) display detailed information about where the syntax
    error occurred.

    The message indicating which exception occurred is always the last
    string in the list.

    """
    return list(_format_exception_only_iter(etype, value))

def _format_exception_only_iter(etype, value):
    # Gracefully handle (the way Python 2.4 and earlier did) the case of
    # being called with (None, None).
    if etype is None:
        yield _format_final_exc_line(etype, value)
        return

    stype = etype.__name__
    smod = etype.__module__
    if smod not in ("__main__", "builtins"):
        stype = smod + '.' + stype

    if not issubclass(etype, SyntaxError):
        yield _format_final_exc_line(stype, value)
        return

    # It was a syntax error; show exactly where the problem was found.
    filename = value.filename or "<string>"
    lineno = str(value.lineno) or '?'
    yield '  File "{}", line {}\n'.format(filename, lineno)

    badline = value.text
    offset = value.offset
    if badline is not None:
        yield '    {}\n'.format(badline.strip())
        if offset is not None:
            caretspace = badline.rstrip('\n')
            offset = min(len(caretspace), offset) - 1
            caretspace = caretspace[:offset].lstrip()
            # non-space whitespace (likes tabs) must be kept for alignment
            caretspace = ((c.isspace() and c or ' ') for c in caretspace)
            yield '    {}^\n'.format(''.join(caretspace))
    msg = value.msg or "<no detail available>"
    yield "{}: {}\n".format(stype, msg)

def _format_final_exc_line(etype, value):
    valuestr = _some_str(value)
    if value is None or not valuestr:
        line = "%s\n" % etype
    else:
        line = "%s: %s\n" % (etype, valuestr)
    return line

def _some_str(value):
    try:
        return str(value)
    except:
        return '<unprintable %s object>' % type(value).__name__

def print_exc(limit=None, file=None, chain=True):
    """Shorthand for 'print_exception(*sys.exc_info(), limit, file)'."""
    print_exception(*sys.exc_info(), limit=limit, file=file, chain=chain)

def format_exc(limit=None, chain=True):
    """Like print_exc() but return a string."""
    return "".join(format_exception(*sys.exc_info(), limit=limit, chain=chain))

def print_last(limit=None, file=None, chain=True):
    """This is a shorthand for 'print_exception(sys.last_type,
    sys.last_value, sys.last_traceback, limit, file)'."""
    if not hasattr(sys, "last_type"):
        raise ValueError("no last exception")
    print_exception(sys.last_type, sys.last_value, sys.last_traceback,
                    limit, file, chain)

#
# Printing and Extracting Stacks.
#

def _extract_stack_iter(f, limit=None):
    return _extract_tb_or_stack_iter(
                f, limit, lambda f: (f, f.f_lineno, f.f_back))

def _get_stack(f):
    if f is None:
        f = sys._getframe().f_back.f_back
    return f

def print_stack(f=None, limit=None, file=None):
    """Print a stack trace from its invocation point.

    The optional 'f' argument can be used to specify an alternate
    stack frame at which to start. The optional 'limit' and 'file'
    arguments have the same meaning as for print_exception().
    """
    print_list(extract_stack(_get_stack(f), limit=limit), file=file)

def format_stack(f=None, limit=None):
    """Shorthand for 'format_list(extract_stack(f, limit))'."""
    return format_list(extract_stack(_get_stack(f), limit=limit))

def extract_stack(f=None, limit=None):
    """Extract the raw traceback from the current stack frame.

    The return value has the same format as for extract_tb().  The
    optional 'f' and 'limit' arguments have the same meaning as for
    print_stack().  Each item in the list is a quadruple (filename,
    line number, function name, text), and the entries are in order
    from oldest to newest stack frame.
    """
    stack = list(_extract_stack_iter(_get_stack(f), limit=limit))
    stack.reverse()
    return stack

def clear_frames(tb):
    "Clear all references to local variables in the frames of a traceback."
    while tb is not None:
        try:
            tb.tb_frame.clear()
        except RuntimeError:
            # Ignore the exception raised if the frame is still executing.
            pass
        tb = tb.tb_next
