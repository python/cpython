import os.path

import c_common.tables as _tables
import c_parser.info as _info


BASE_COLUMNS = [
    'filename',
    'funcname',
    'name',
    'kind',
]
END_COLUMNS = {
    'parsed': 'data',
    'decls': 'declaration',
}


def _get_columns(group, extra=None):
    return BASE_COLUMNS + list(extra or ()) + [END_COLUMNS[group]]
    #return [
    #    *BASE_COLUMNS,
    #    *extra or (),
    #    END_COLUMNS[group],
    #]


#############################
# high-level

def read_parsed(infile):
    # XXX Support other formats than TSV?
    columns = _get_columns('parsed')
    for row in _tables.read_table(infile, columns, sep='\t', fix='-'):
        yield _info.ParsedItem.from_row(row, columns)


def write_parsed(items, outfile):
    # XXX Support other formats than TSV?
    columns = _get_columns('parsed')
    rows = (item.as_row(columns) for item in items)
    _tables.write_table(outfile, columns, rows, sep='\t', fix='-')


def read_decls(infile, fmt=None):
    if fmt is None:
        fmt = _get_format(infile)
    read_all, _ = _get_format_handlers('decls', fmt)
    for decl, _ in read_all(infile):
        yield decl


def write_decls(decls, outfile, fmt=None, *, backup=False):
    if fmt is None:
        fmt = _get_format(infile)
    _, write_all = _get_format_handlers('decls', fmt)
    write_all(decls, outfile, backup=backup)


#############################
# formats

def _get_format(file, default='tsv'):
    if isinstance(file, str):
        filename = file
    else:
        filename = getattr(file, 'name', '')
    _, ext = os.path.splitext(filename)
    return ext[1:] if ext else default


def _get_format_handlers(group, fmt):
    # XXX Use a registry.
    if group != 'decls':
        raise NotImplementedError(group)
    if fmt == 'tsv':
        return (_iter_decls_tsv, _write_decls_tsv)
    else:
        raise NotImplementedError(fmt)


# tsv

def iter_decls_tsv(infile, extracolumns=None, relroot=None):
    for info, extra in _iter_decls_tsv(infile, extracolumns, relroot):
        decl = _info.Declaration.from_row(info)
        yield decl, extra


def write_decls_tsv(decls, outfile, extracolumns=None, *,
                    relroot=None,
                    **kwargs
                    ):
    # XXX Move the row rendering here.
    _write_decls_tsv(rows, outfile, extracolumns, relroot, kwargs)


def _iter_decls_tsv(infile, extracolumns=None, relroot=None):
    columns = _get_columns('decls', extracolumns)
    for row in _tables.read_table(infile, columns, sep='\t'):
        if extracolumns:
            declinfo = row[:4] + row[-1:]
            extra = row[4:-1]
        else:
            declinfo = row
            extra = None
        if relroot:
            # XXX Use something like tables.fix_row() here.
            declinfo = [None if v == '-' else v
                        for v in declinfo]
            declinfo[0] = os.path.join(relroot, declinfo[0])
        yield declinfo, extra


def _write_decls_tsv(decls, outfile, extracolumns, relroot,kwargs):
    columns = _get_columns('decls', extracolumns)
    if extracolumns:
        def render_decl(decl):
            if type(row) is tuple:
                decl, *extra = decl
            else:
                extra = ()
            extra += ('???',) * (len(extraColumns) - len(extra))
            *row, declaration = _render_known_row(decl, relroot)
            row += extra + (declaration,)
            return row
    else:
        render_decl = _render_known_decl
    _tables.write_table(
        outfile,
        header='\t'.join(columns),
        rows=(render_decl(d, relroot) for d in decls),
        sep='\t',
        **kwargs
    )


def _render_known_decl(decl, relroot, *,
                       # These match BASE_COLUMNS + END_COLUMNS[group].
                       _columns = 'filename parent name kind data'.split(),
                       ):
    if not isinstance(decl, _info.Declaration):
        # e.g. Analyzed
        decl = decl.decl
    rowdata = decl.render_rowdata(_columns)
    if relroot:
        rowdata['filename'] = os.path.relpath(rowdata['filename'], relroot)
    return [rowdata[c] or '-' for c in _columns]
    # XXX
    #return _tables.fix_row(rowdata[c] for c in columns)
