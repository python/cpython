"""Helper module to tranlate 3.5 type comments to 3.6 variable annotations."""
import re
import os
import ast
import argparse
import tokenize
from collections import defaultdict
from textwrap import dedent
from io import BytesIO

__all__ = ['com2ann', 'TYPE_COM']

TYPE_COM = re.compile(r'\s*#\s*type\s*:.*$', flags=re.DOTALL)
TRAIL_OR_COM = re.compile(r'\s*$|\s*#.*$', flags=re.DOTALL)


class _Data:
    """Internal class describing global data on file."""
    def __init__(self, lines, tokens):
        self.lines = lines
        self.tokens = tokens
        ttab = defaultdict(list) # maps line number to token numbers
        for i, tok in enumerate(tokens):
            ttab[tok.start[0]].append(i)
        self.ttab = ttab
        self.success = [] # list of lines where type comments where processed
        self.fail = [] # list of lines where type comments where rejected


def skip_blank(d, lno):
    while d.lines[lno].strip() == '':
        lno += 1
    return lno


def find_start(d, lcom):
    """Find first char of the assignment target."""
    i = d.ttab[lcom + 1][-2] # index of type comment token in tokens list
    while ((d.tokens[i].exact_type != tokenize.NEWLINE) and
           (d.tokens[i].exact_type != tokenize.ENCODING)):
        i -= 1
    lno = d.tokens[i].start[0]
    return skip_blank(d, lno)


def check_target(stmt):
    if len(stmt.body):
        assign = stmt.body[0]
    else:
        return False
    if isinstance(assign, ast.Assign) and len(assign.targets) == 1:
        targ = assign.targets[0]
    else:
        return False
    if (isinstance(targ, ast.Name) or isinstance(targ, ast.Attribute)
        or isinstance(targ, ast.Subscript)):
        return True
    return False


def find_eq(d, lstart):
    """Find equal sign starting from lstart taking care about d[f(x=1)] = 5."""
    col = pars = 0
    lno = lstart
    while d.lines[lno][col] != '=' or pars != 0:
        ch = d.lines[lno][col]
        if ch in '([{':
            pars += 1
        elif ch in ')]}':
            pars -= 1
        if ch == '#' or col == len(d.lines[lno])-1:
            lno = skip_blank(d, lno+1)
            col = 0
        else:
            col += 1
    return lno, col


def find_val(d, poseq):
    """Find position of first char of assignment value starting from poseq."""
    lno, col = poseq
    while (d.lines[lno][col].isspace() or d.lines[lno][col] in '=\\'):
        if col == len(d.lines[lno])-1:
            lno += 1
            col = 0
        else:
            col += 1
    return lno, col


def find_targ(d, poseq):
    """Find position of last char of target (annotation goes here)."""
    lno, col = poseq
    while (d.lines[lno][col].isspace() or d.lines[lno][col] in '=\\'):
        if col == 0:
            lno -= 1
            col = len(d.lines[lno])-1
        else:
            col -= 1
    return lno, col+1


def trim(new_lines, string, ltarg, poseq, lcom, ccom):
    """Remove None or Ellipsis from assignment value.

    Also remove parens if one has (None), (...) etc.
    string -- 'None' or '...'
    ltarg -- line where last char of target is located
    poseq -- position of equal sign
    lcom, ccom -- position of type comment
    """
    nopars = lambda s: s.replace('(', '').replace(')', '')
    leq, ceq = poseq
    end = ccom if leq == lcom else len(new_lines[leq])
    subline = new_lines[leq][:ceq]
    if leq == ltarg:
        subline = subline.rstrip()
    new_lines[leq] = subline + (new_lines[leq][end:] if leq == lcom
                                else new_lines[leq][ceq+1:end])

    for lno in range(leq+1,lcom):
        new_lines[lno] = nopars(new_lines[lno])

    if lcom != leq:
        subline = nopars(new_lines[lcom][:ccom]).replace(string, '')
        if (not subline.isspace()):
            subline = subline.rstrip()
        new_lines[lcom] = subline + new_lines[lcom][ccom:]


def _com2ann(d, drop_None, drop_Ellipsis):
    new_lines = d.lines[:]
    for lcom, line in enumerate(d.lines):
        match = re.search(TYPE_COM, line)
        if match:
            # strip " #  type  :  annotation  \n" -> "annotation  \n"
            tp = match.group().lstrip()[1:].lstrip()[4:].lstrip()[1:].lstrip()
            submatch = re.search(TRAIL_OR_COM, tp)
            subcom = ''
            if submatch and submatch.group():
                subcom = submatch.group()
                tp = tp[:submatch.start()]
            if tp == 'ignore':
                continue
            ccom = match.start()
            if not any(d.tokens[i].exact_type == tokenize.COMMENT
                   for i in d.ttab[lcom + 1]):
                d.fail.append(lcom)
                continue # type comment inside string
            lstart = find_start(d, lcom)
            stmt_str = dedent(''.join(d.lines[lstart:lcom+1]))
            try:
                stmt = ast.parse(stmt_str)
            except SyntaxError:
                d.fail.append(lcom)
                continue # for or with statements
            if not check_target(stmt):
                d.fail.append(lcom)
                continue

            d.success.append(lcom)
            val = stmt.body[0].value

            # writing output now
            poseq = find_eq(d, lstart)
            lval, cval = find_val(d, poseq)
            ltarg, ctarg = find_targ(d, poseq)

            op_par = ''
            cl_par = ''
            if isinstance(val, ast.Tuple):
                if d.lines[lval][cval] != '(':
                    op_par = '('
                    cl_par = ')'
            # write the comment first
            new_lines[lcom] = d.lines[lcom][:ccom].rstrip() + cl_par + subcom
            ccom = len(d.lines[lcom][:ccom].rstrip())

            string = False
            if isinstance(val, ast.Tuple):
            # t = 1, 2 -> t = (1, 2); only latter is allowed with annotation
                free_place = int(new_lines[lval][cval-2:cval] == '  ')
                new_lines[lval] = (new_lines[lval][:cval-free_place] +
                                       op_par + new_lines[lval][cval:])
            elif isinstance(val, ast.Ellipsis) and drop_Ellipsis:
                string = '...'
            elif (isinstance(val, ast.NameConstant) and
                        val.value is None and drop_None):
                string = 'None'
            if string:
                trim(new_lines, string, ltarg, poseq, lcom, ccom)

            # finally write an annotation
            new_lines[ltarg] = (new_lines[ltarg][:ctarg] +
                              ': ' + tp + new_lines[ltarg][ctarg:])
    return ''.join(new_lines)


def com2ann(code, *, drop_None=False, drop_Ellipsis=False, silent=False):
    """Translate type comments to type annotations in code.

    Take code as string and return this string where::

      variable = value # type: annotation # real comment

    is translated to::

      variable: annotation = value # real comment

    For unsupported syntax cases, the type comments are
    left intact. If drop_None is True or if drop_Ellipsis
    is True translate correcpondingly::

      variable = None # type: annotation
      variable = ... # type: annotation

    into::

      variable: annotation

    The tool tries to preserve code formatting as much as
    possible, but an exact translation is not guarateed.
    A summary of translated comments id printed by default.
    """
    try:
        ast.parse(code) # we want to work only with file without syntax errors
    except SyntaxError:
        return None
    lines = code.splitlines(keepends=True)
    rl = BytesIO(code.encode('utf-8')).readline
    tokens = list(tokenize.tokenize(rl))

    data = _Data(lines, tokens)
    new_code = _com2ann(data, drop_None, drop_Ellipsis)

    if not silent:
        if data.success:
            print('Comments translated on lines:',
                  ', '.join(str(lno+1) for lno in data.success))
        if data.fail:
            print('Comments rejected on lines:',
                  ', '.join(str(lno+1) for lno in data.fail))
        if not data.success and not data.fail:
            print('No type comments found')

    return new_code


def translate_file(infile, outfile, dnone, dell, silent):
    try:
        descr = tokenize.open(infile)
    except SyntaxError:
        print("Cannot open", infile)
        return
    with descr as f:
        code = f.read()
        enc = f.encoding
    if not silent:
        print('File:', infile)
    new_code = com2ann(code, drop_None=dnone,
                             drop_Ellipsis=dell,
                             silent=silent)
    if new_code is None:
        print("SyntaxError in", infile)
        return
    with open(outfile, 'wb') as f:
        f.write((new_code).encode(enc))


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-o", "--outfile",
                        help="output file, will be overwritten if exists,\n"
                             "defaults to input file")
    parser.add_argument("infile",
                        help="input file or directory for translation, must\n"
                             "contain no syntax errors, for directory\n"
                             "the outfile is ignored and translation is\n"
                             "made in place")
    parser.add_argument("-s", "--silent",
                        help="Do not print summary for line numbers of\n"
                             "translated and rejected comments",
                        action="store_true")
    parser.add_argument("-n", "--drop-none",
                   help="drop any None as assignment value during\n"
                        "translation if it is annotated by a type coment",
                   action="store_true")
    parser.add_argument("-e", "--drop-ellipsis",
                   help="drop any Ellipsis (...) as assignment value during\n"
                        "translation if it is annotated by a type coment",
                   action="store_true")
    args = parser.parse_args()
    if args.outfile is None:
        args.outfile = args.infile

    if os.path.isfile(args.infile):
        translate_file(args.infile, args.outfile,
                       args.drop_none, args.drop_ellipsis, args.silent)
    else:
        for root, dirs, files in os.walk(args.infile):
            for afile in files:
                _, ext = os.path.splitext(afile)
                if ext == '.py' or ext == '.pyi':
                    fname = os.path.join(root, afile)
                    translate_file(fname, fname,
                                   args.drop_none, args.drop_ellipsis,
                                   args.silent)
