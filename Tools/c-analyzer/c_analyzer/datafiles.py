import os.path

import c_common.tables as _tables
import c_common.iterutil as _iterutil
import c_parser.info as _info
from . import analyze as _analyze


#############################
# "known" decls

KNOWN_COLUMNS = [
    'filename',
    'funcname',
    'name',
    'kind',
    #'typedecl',
    'declaration',
]


def analyze_known(known, *,
                  analyze_resolved=None,
                  handle_unresolved=True,
                  ):
    knowntypes = knowntypespecs = {}
    collated = _info.collate_by_kind_group(known)
    types = {decl: None for decl in collated['type']}
    typespecs = _analyze.get_typespecs(types)
    def analyze_decl(decl):
        return _analyze.analyze_decl(
            decl,
            typespecs,
            knowntypespecs,
            types,
            knowntypes,
            analyze_resolved=analyze_resolved,
        )
    _analyze.analyze_type_decls(types, analyze_decl, handle_unresolved)
    return types, typespecs


def get_known(known, extracolumns=None, *,
              analyze_resolved=None,
              handle_unresolved=True,
              relroot=None,
              ):
    if isinstance(known, str):
        known = read_known(known, extracolumns, relroot)
    return analyze_known(
        known,
        handle_unresolved=handle_unresolved,
        analyze_resolved=analyze_resolved,
    )


def read_known(infile, extracolumns=None, relroot=None):
    known = {}
    for info in _iter_known(infile, extracolumns, relroot):
        decl, extra = info
        known[decl] = extra
    return known


def _iter_known(infile, extracolumns, relroot):
    if extracolumns:
        # Always put the declaration last.
        columns = KNOWN_COLUMNS[:-1] + extracolumns + KNOWN_COLUMNS[-1:]
    else:
        columns = KNOWN_COLUMNS
    header = '\t'.join(columns)
    for row in _tables.read_table(infile, header, sep='\t'):
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
        decl = _info.Declaration.from_row(declinfo)
        yield decl, extra


def write_known(rows, outfile, extracolumns=None, *,
                relroot=None,
                backup=True,
                ):
    if extracolumns:
        # Always put the declaration last.
        columns = KNOWN_COLUMNS[:-1] + extracolumns + KNOWN_COLUMNS[-1]
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
        columns = KNOWN_COLUMNS
        render_decl = _render_known_decl
    _tables.write_table(
        outfile,
        header='\t'.join(columns),
        rows=(render_decl(d, relroot) for d in rows),
        sep='\t',
        backup=backup,
    )


def _render_known_decl(row, relroot):
    if isinstance(row, _info.Declaration):
        decl = row
    elif not isinstance(row, _info.Declaration):
        # e.g. Analyzed
        decl = row.decl
    # These match KNOWN_COLUMNS.
    columns = 'filename parent name kind data'.split()
    rowdata = decl.render_rowdata(columns)
    if relroot:
        rowdata['filename'] = os.path.relpath(rowdata['filename'], relroot)
    return [rowdata[c] or '-' for c in columns]
    # XXX
    #return _tables.fix_row(rowdata[c] for c in columns)


#############################
# ignored vars

IGNORED_COLUMNS = [
    'filename',
    'funcname',
    'name',
    'reason',
]
IGNORED_HEADER = '\t'.join(IGNORED_COLUMNS)


def read_ignored(infile):
    return dict(_iter_ignored(infile))


def _iter_ignored(infile):
    for row in _tables.read_table(infile, IGNORED_HEADER, sep='\t'):
        *varidinfo, reason = row
        varid = _info.DeclID.from_row(varidinfo)
        yield varid, reason


def write_ignored(variables, outfile):
    raise NotImplementedError
    reason = '???'
    #if not isinstance(varid, DeclID):
    #    varid = getattr(varid, 'parsed', varid).id
    _tables.write_table(
        outfile,
        IGNORED_HEADER,
        sep='\t',
        rows=(r.render_rowdata() + (reason,) for r in decls),
    )
