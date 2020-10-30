import os.path

from c_parser import (
    info as _info,
    match as _match,
)


_KIND = _info.KIND


# XXX Use known.tsv for these?
SYSTEM_TYPES = {
    'int8_t',
    'uint8_t',
    'int16_t',
    'uint16_t',
    'int32_t',
    'uint32_t',
    'int64_t',
    'uint64_t',
    'size_t',
    'ssize_t',
    'intptr_t',
    'uintptr_t',
    'wchar_t',
    '',
    # OS-specific
    'pthread_cond_t',
    'pthread_mutex_t',
    'pthread_key_t',
    'atomic_int',
    'atomic_uintptr_t',
    '',
    # lib-specific
    'WINDOW',  # curses
    'XML_LChar',
    'XML_Size',
    'XML_Parser',
    'enum XML_Error',
    'enum XML_Status',
    '',
}


def is_system_type(typespec):
    return typespec in SYSTEM_TYPES


##################################
# decl matchers

def is_public(decl):
    if not decl.filename.endswith('.h'):
        return False
    if 'Include' not in decl.filename.split(os.path.sep):
        return False
    return True


def is_process_global(vardecl):
    kind, storage, _, _, _ = _info.get_parsed_vartype(vardecl)
    if kind is not _KIND.VARIABLE:
        raise NotImplementedError(vardecl)
    if 'static' in (storage or ''):
        return True

    if hasattr(vardecl, 'parent'):
        parent = vardecl.parent
    else:
        parent = vardecl.get('parent')
    return not parent


def is_fixed_type(vardecl):
    if not vardecl:
        return None
    _, _, _, typespec, abstract = _info.get_parsed_vartype(vardecl)
    if 'typeof' in typespec:
        raise NotImplementedError(vardecl)
    elif not abstract:
        return True

    if '*' not in abstract:
        # XXX What about []?
        return True
    elif _match._is_funcptr(abstract):
        return True
    else:
        for after in abstract.split('*')[1:]:
            if not after.lstrip().startswith('const'):
                return False
        else:
            return True


def is_immutable(vardecl):
    if not vardecl:
        return None
    if not is_fixed_type(vardecl):
        return False
    _, _, typequal, _, _ = _info.get_parsed_vartype(vardecl)
    # If there, it can only be "const" or "volatile".
    return typequal == 'const'


def is_public_api(decl):
    if not is_public(decl):
        return False
    if decl.kind is _KIND.TYPEDEF:
        return True
    elif _match.is_type_decl(decl):
        return not _match.is_forward_decl(decl)
    else:
        return _match.is_external_reference(decl)


def is_public_declaration(decl):
    if not is_public(decl):
        return False
    if decl.kind is _KIND.TYPEDEF:
        return True
    elif _match.is_type_decl(decl):
        return _match.is_forward_decl(decl)
    else:
        return _match.is_external_reference(decl)


def is_public_definition(decl):
    if not is_public(decl):
        return False
    if decl.kind is _KIND.TYPEDEF:
        return True
    elif _match.is_type_decl(decl):
        return not _match.is_forward_decl(decl)
    else:
        return not _match.is_external_reference(decl)
