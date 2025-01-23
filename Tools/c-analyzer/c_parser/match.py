import re

from . import info as _info
from .parser._regexes import SIMPLE_TYPE


_KIND = _info.KIND


def match_storage(decl, expected):
    default = _info.get_default_storage(decl)
    #assert default
    if expected is None:
        expected = {default}
    elif isinstance(expected, str):
        expected = {expected or default}
    elif not expected:
        expected = _info.STORAGE
    else:
        expected = {v or default for v in expected}
    storage = _info.get_effective_storage(decl, default=default)
    return storage in expected


##################################
# decl matchers

def is_type_decl(item):
    return _KIND.is_type_decl(item.kind)


def is_decl(item):
    return _KIND.is_decl(item.kind)


def is_pots(typespec, *,
            _regex=re.compile(rf'^{SIMPLE_TYPE}$', re.VERBOSE),
            ):

    if not typespec:
        return None
    if type(typespec) is not str:
        _, _, _, typespec, _ = _info.get_parsed_vartype(typespec)
    return _regex.match(typespec) is not None


def is_funcptr(vartype):
    if not vartype:
        return None
    _, _, _, _, abstract = _info.get_parsed_vartype(vartype)
    return _is_funcptr(abstract)


def _is_funcptr(declstr):
    if not declstr:
        return None
    # XXX Support "(<name>*)(".
    return '(*)(' in declstr.replace(' ', '')


def is_forward_decl(decl):
    if decl.kind is _KIND.TYPEDEF:
        return False
    elif is_type_decl(decl):
        return not decl.data
    elif decl.kind is _KIND.FUNCTION:
        # XXX This doesn't work with ParsedItem.
        return decl.signature.isforward
    elif decl.kind is _KIND.VARIABLE:
        # No var decls are considered forward (or all are...).
        return False
    else:
        raise NotImplementedError(decl)


def can_have_symbol(decl):
    return decl.kind in (_KIND.VARIABLE, _KIND.FUNCTION)


def has_external_symbol(decl):
    if not can_have_symbol(decl):
        return False
    if _info.get_effective_storage(decl) != 'extern':
        return False
    if decl.kind is _KIND.FUNCTION:
        return not decl.signature.isforward
    else:
        # It must be a variable, which can only be implicitly extern here.
        return decl.storage != 'extern'


def has_internal_symbol(decl):
    if not can_have_symbol(decl):
        return False
    return _info.get_actual_storage(decl) == 'static'


def is_external_reference(decl):
    if not can_have_symbol(decl):
        return False
    # We have to check the declared storage rather tnan the effective.
    if decl.storage != 'extern':
        return False
    if decl.kind is _KIND.FUNCTION:
        return decl.signature.isforward
    # Otherwise it's a variable.
    return True


def is_local_var(decl):
    if not decl.kind is _KIND.VARIABLE:
        return False
    return True if decl.parent else False


def is_global_var(decl):
    if not decl.kind is _KIND.VARIABLE:
        return False
    return False if decl.parent else True


##################################
# filtering with matchers

def filter_by_kind(items, kind):
    if kind == 'type':
        kinds = _KIND._TYPE_DECLS
    elif kind == 'decl':
        kinds = _KIND._TYPE_DECLS
    try:
        okay = kind in _KIND
    except TypeError:
        kinds = set(kind)
    else:
        kinds = {kind} if okay else set(kind)
    for item in items:
        if item.kind in kinds:
            yield item


##################################
# grouping with matchers

def group_by_category(decls, categories, *, ignore_non_match=True):
    collated = {}
    for decl in decls:
        # Matchers should be mutually exclusive.  (First match wins.)
        for category, match in categories.items():
            if match(decl):
                if category not in collated:
                    collated[category] = [decl]
                else:
                    collated[category].append(decl)
                break
        else:
            if not ignore_non_match:
                raise Exception(f'no match for {decl!r}')
    return collated


def group_by_kind(items):
    collated = {kind: [] for kind in _KIND}
    for item in items:
        try:
            collated[item.kind].append(item)
        except KeyError:
            raise ValueError(f'unsupported kind in {item!r}')
    return collated


def group_by_kinds(items):
    # Collate into kind groups (decl, type, etc.).
    collated = {_KIND.get_group(k): [] for k in _KIND}
    for item in items:
        group = _KIND.get_group(item.kind)
        collated[group].append(item)
    return collated
