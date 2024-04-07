import os.path

from c_common import fsutil
import c_common.tables as _tables
import c_parser.info as _info
import c_parser.match as _match
import c_parser.datafiles as _parser
from . import analyze as _analyze


#############################
# "known" decls

EXTRA_COLUMNS = [
    #'typedecl',
]


def get_known(known, extracolumns=None, *,
              analyze_resolved=None,
              handle_unresolved=True,
              relroot=fsutil.USE_CWD,
              ):
    if isinstance(known, str):
        known = read_known(known, extracolumns, relroot)
    return analyze_known(
        known,
        handle_unresolved=handle_unresolved,
        analyze_resolved=analyze_resolved,
    )


def read_known(infile, extracolumns=None, relroot=fsutil.USE_CWD):
    extracolumns = EXTRA_COLUMNS + (
        list(extracolumns) if extracolumns else []
    )
    known = {}
    for decl, extra in _parser.iter_decls_tsv(infile, extracolumns, relroot):
        known[decl] = extra
    return known


def analyze_known(known, *,
                  analyze_resolved=None,
                  handle_unresolved=True,
                  ):
    knowntypes = knowntypespecs = {}
    collated = _match.group_by_kinds(known)
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


def write_known(rows, outfile, extracolumns=None, *,
                relroot=fsutil.USE_CWD,
                backup=True,
                ):
    extracolumns = EXTRA_COLUMNS + (
        list(extracolumns) if extracolumns else []
    )
    _parser.write_decls_tsv(
        rows,
        outfile,
        extracolumns,
        relroot=relroot,
        backup=backup,
    )


#############################
# ignored vars

IGNORED_COLUMNS = [
    'filename',
    'funcname',
    'name',
    'reason',
]
IGNORED_HEADER = '\t'.join(IGNORED_COLUMNS)


def read_ignored(infile, relroot=fsutil.USE_CWD):
    return dict(_iter_ignored(infile, relroot))


def _iter_ignored(infile, relroot):
    if relroot and relroot is not fsutil.USE_CWD:
        relroot = os.path.abspath(relroot)
    bogus = {_tables.EMPTY, _tables.UNKNOWN}
    for row in _tables.read_table(infile, IGNORED_HEADER, sep='\t'):
        *varidinfo, reason = row
        if _tables.EMPTY in varidinfo or _tables.UNKNOWN in varidinfo:
            varidinfo = tuple(None if v in bogus else v
                              for v in varidinfo)
        if reason in bogus:
            reason = None
        varid = _info.DeclID.from_row(varidinfo)
        varid = varid.fix_filename(relroot, formatted=False, fixroot=False)
        yield varid, reason


def write_ignored(variables, outfile, relroot=fsutil.USE_CWD):
    raise NotImplementedError
    if relroot and relroot is not fsutil.USE_CWD:
        relroot = os.path.abspath(relroot)
    reason = '???'
    #if not isinstance(varid, DeclID):
    #    varid = getattr(varid, 'parsed', varid).id
    decls = (d.fix_filename(relroot, fixroot=False) for d in decls)
    _tables.write_table(
        outfile,
        IGNORED_HEADER,
        sep='\t',
        rows=(r.render_rowdata() + (reason,) for r in decls),
    )
