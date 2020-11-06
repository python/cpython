import os.path
import re

from c_common.clsutil import classonly
from c_parser.info import (
    KIND,
    DeclID,
    Declaration,
    TypeDeclaration,
    TypeDef,
    Struct,
    Member,
    FIXED_TYPE,
)
from c_parser.match import (
    is_type_decl,
    is_pots,
    is_funcptr,
)
from c_analyzer.match import (
    is_system_type,
    is_process_global,
    is_fixed_type,
    is_immutable,
)
import c_analyzer as _c_analyzer
import c_analyzer.info as _info
import c_analyzer.datafiles as _datafiles
from . import _parser, REPO_ROOT


_DATA_DIR = os.path.dirname(__file__)
KNOWN_FILE = os.path.join(_DATA_DIR, 'known.tsv')
IGNORED_FILE = os.path.join(_DATA_DIR, 'ignored.tsv')
KNOWN_IN_DOT_C = {
    'struct _odictobject': False,
    'PyTupleObject': False,
    'struct _typeobject': False,
    'struct _arena': True,  # ???
    'struct _frame': False,
    'struct _ts': True,  # ???
    'struct PyCodeObject': False,
    'struct _is': True,  # ???
    'PyWideStringList': True,  # ???
    # recursive
    'struct _dictkeysobject': False,
}
# These are loaded from the respective .tsv files upon first use.
_KNOWN = {
    # {(file, ID) | ID => info | bool}
    #'PyWideStringList': True,
}
#_KNOWN = {(Struct(None, typeid.partition(' ')[-1], None)
#           if typeid.startswith('struct ')
#           else TypeDef(None, typeid, None)
#           ): ([], {'unsupported': None if supported else True})
#          for typeid, supported in _KNOWN_IN_DOT_C.items()}
_IGNORED = {
    # {ID => reason}
}

KINDS = frozenset((*KIND.TYPES, KIND.VARIABLE))


def read_known():
    if not _KNOWN:
        # Cache a copy the first time.
        extracols = None  # XXX
        #extracols = ['unsupported']
        known = _datafiles.read_known(KNOWN_FILE, extracols, REPO_ROOT)
        # For now we ignore known.values() (i.e. "extra").
        types, _ = _datafiles.analyze_known(
            known,
            analyze_resolved=analyze_resolved,
        )
        _KNOWN.update(types)
    return _KNOWN.copy()


def write_known():
    raise NotImplementedError
    datafiles.write_known(decls, IGNORED_FILE, ['unsupported'], relroot=REPO_ROOT)


def read_ignored():
    if not _IGNORED:
        _IGNORED.update(_datafiles.read_ignored(IGNORED_FILE))
    return dict(_IGNORED)


def write_ignored():
    raise NotImplementedError
    datafiles.write_ignored(variables, IGNORED_FILE)


def analyze(filenames, *,
            skip_objects=False,
            **kwargs
            ):
    if skip_objects:
        # XXX Set up a filter.
        raise NotImplementedError

    known = read_known()

    decls = iter_decls(filenames)
    results = _c_analyzer.analyze_decls(
        decls,
        known,
        analyze_resolved=analyze_resolved,
    )
    analysis = Analysis.from_results(results)

    return analysis


def iter_decls(filenames, **kwargs):
    decls = _c_analyzer.iter_decls(
        filenames,
        # We ignore functions (and statements).
        kinds=KINDS,
        parse_files=_parser.parse_files,
        **kwargs
    )
    for decl in decls:
        if not decl.data:
            # Ignore forward declarations.
            continue
        yield decl


def analyze_resolved(resolved, decl, types, knowntypes, extra=None):
    if decl.kind not in KINDS:
        # Skip it!
        return None

    typedeps = resolved
    if typedeps is _info.UNKNOWN:
        if decl.kind in (KIND.STRUCT, KIND.UNION):
            typedeps = [typedeps] * len(decl.members)
        else:
            typedeps = [typedeps]
    #assert isinstance(typedeps, (list, TypeDeclaration)), typedeps

    if extra is None:
        extra = {}
    elif 'unsupported' in extra:
        raise NotImplementedError((decl, extra))

    unsupported = _check_unsupported(decl, typedeps, types, knowntypes)
    extra['unsupported'] = unsupported

    return typedeps, extra


def _check_unsupported(decl, typedeps, types, knowntypes):
    if typedeps is None:
        raise NotImplementedError(decl)

    if decl.kind in (KIND.STRUCT, KIND.UNION):
        return _check_members(decl, typedeps, types, knowntypes)
    elif decl.kind is KIND.ENUM:
        if typedeps:
            raise NotImplementedError((decl, typedeps))
        return None
    else:
        return _check_typedep(decl, typedeps, types, knowntypes)


def _check_members(decl, typedeps, types, knowntypes):
    if isinstance(typedeps, TypeDeclaration):
        raise NotImplementedError((decl, typedeps))

    #members = decl.members or ()  # A forward decl has no members.
    members = decl.members
    if not members:
        # A forward decl has no members, but that shouldn't surface here..
        raise NotImplementedError(decl)
    if len(members) != len(typedeps):
        raise NotImplementedError((decl, typedeps))

    unsupported = []
    for member, typedecl in zip(members, typedeps):
        checked = _check_typedep(member, typedecl, types, knowntypes)
        unsupported.append(checked)
    if any(None if v is FIXED_TYPE else v for v in unsupported):
        return unsupported
    elif FIXED_TYPE in unsupported:
        return FIXED_TYPE
    else:
        return None


def _check_typedep(decl, typedecl, types, knowntypes):
    if not isinstance(typedecl, TypeDeclaration):
        if hasattr(type(typedecl), '__len__'):
            if len(typedecl) == 1:
                typedecl, = typedecl
    if typedecl is None:
        # XXX Fail?
        return 'typespec (missing)'
    elif typedecl is _info.UNKNOWN:
        # XXX Is this right?
        return 'typespec (unknown)'
    elif not isinstance(typedecl, TypeDeclaration):
        raise NotImplementedError((decl, typedecl))

    if isinstance(decl, Member):
        return _check_vartype(decl, typedecl, types, knowntypes)
    elif not isinstance(decl, Declaration):
        raise NotImplementedError(decl)
    elif decl.kind is KIND.TYPEDEF:
        return _check_vartype(decl, typedecl, types, knowntypes)
    elif decl.kind is KIND.VARIABLE:
        if not is_process_global(decl):
            return None
        checked = _check_vartype(decl, typedecl, types, knowntypes)
        return 'mutable' if checked is FIXED_TYPE else checked
    else:
        raise NotImplementedError(decl)


def _check_vartype(decl, typedecl, types, knowntypes):
    """Return failure reason."""
    checked = _check_typespec(decl, typedecl, types, knowntypes)
    if checked:
        return checked
    if is_immutable(decl.vartype):
        return None
    if is_fixed_type(decl.vartype):
        return FIXED_TYPE
    return 'mutable'


def _check_typespec(decl, typedecl, types, knowntypes):
    typespec = decl.vartype.typespec
    if typedecl is not None:
        found = types.get(typedecl)
        if found is None:
            found = knowntypes.get(typedecl)

        if found is not None:
            _, extra = found
            if extra is None:
                # XXX Under what circumstances does this happen?
                extra = {}
            unsupported = extra.get('unsupported')
            if unsupported is FIXED_TYPE:
                unsupported = None
            return 'typespec' if unsupported else None
    # Fall back to default known types.
    if is_pots(typespec):
        return None
    elif is_system_type(typespec):
        return None
    elif is_funcptr(decl.vartype):
        return None
    return 'typespec'


class Analyzed(_info.Analyzed):

    @classonly
    def is_target(cls, raw):
        if not super().is_target(raw):
            return False
        if raw.kind not in KINDS:
            return False
        return True

    #@classonly
    #def _parse_raw_result(cls, result, extra):
    #    typedecl, extra = super()._parse_raw_result(result, extra)
    #    if typedecl is None:
    #        return None, extra
    #    raise NotImplementedError

    def __init__(self, item, typedecl=None, *, unsupported=None, **extra):
        if 'unsupported' in extra:
            raise NotImplementedError((item, typedecl, unsupported, extra))
        if not unsupported:
            unsupported = None
        elif isinstance(unsupported, (str, TypeDeclaration)):
            unsupported = (unsupported,)
        elif unsupported is not FIXED_TYPE:
            unsupported = tuple(unsupported)
        self.unsupported = unsupported
        extra['unsupported'] = self.unsupported  # ...for __repr__(), etc.
        if self.unsupported is None:
            #self.supported = None
            self.supported = True
        elif self.unsupported is FIXED_TYPE:
            if item.kind is KIND.VARIABLE:
                raise NotImplementedError(item, typedecl, unsupported)
            self.supported = True
        else:
            self.supported = not self.unsupported
        super().__init__(item, typedecl, **extra)

    def render(self, fmt='line', *, itemonly=False):
        if fmt == 'raw':
            yield repr(self)
            return
        rendered = super().render(fmt, itemonly=itemonly)
        # XXX ???
        #if itemonly:
        #    yield from rendered
        supported = self._supported
        if fmt in ('line', 'brief'):
            rendered, = rendered
            parts = [
                '+' if supported else '-' if supported is False else '',
                rendered,
            ]
            yield '\t'.join(parts)
        elif fmt == 'summary':
            raise NotImplementedError(fmt)
        elif fmt == 'full':
            yield from rendered
            if supported:
                yield f'\tsupported:\t{supported}'
        else:
            raise NotImplementedError(fmt)


class Analysis(_info.Analysis):
    _item_class = Analyzed

    @classonly
    def build_item(cls, info, result=None):
        if not isinstance(info, Declaration) or info.kind not in KINDS:
            raise NotImplementedError((info, result))
        return super().build_item(info, result)


def check_globals(analysis):
    # yield (data, failure)
    ignored = read_ignored()
    for item in analysis:
        if item.kind != KIND.VARIABLE:
            continue
        if item.supported:
            continue
        if item.id in ignored:
            continue
        reason = item.unsupported
        if not reason:
            reason = '???'
        elif not isinstance(reason, str):
            if len(reason) == 1:
                reason, = reason
        reason = f'({reason})'
        yield item, f'not supported {reason:20}\t{item.storage or ""} {item.vartype}'
