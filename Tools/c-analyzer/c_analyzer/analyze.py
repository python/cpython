from c_parser.info import (
    KIND,
    TypeDeclaration,
    POTSType,
    FuncPtr,
)
from c_parser.match import (
    is_pots,
    is_funcptr,
)
from .info import (
    IGNORED,
    UNKNOWN,
    SystemType,
)
from .match import (
    is_system_type,
)


def get_typespecs(typedecls):
    typespecs = {}
    for decl in typedecls:
        if decl.shortkey not in typespecs:
            typespecs[decl.shortkey] = [decl]
        else:
            typespecs[decl.shortkey].append(decl)
    return typespecs


def analyze_decl(decl, typespecs, knowntypespecs, types, knowntypes, *,
                 analyze_resolved=None):
    resolved = resolve_decl(decl, typespecs, knowntypespecs, types)
    if resolved is None:
        # The decl is supposed to be skipped or ignored.
        return None
    if analyze_resolved is None:
        return resolved, None
    return analyze_resolved(resolved, decl, types, knowntypes)

# This alias helps us avoid name collisions.
_analyze_decl = analyze_decl


def analyze_type_decls(types, analyze_decl, handle_unresolved=True):
    unresolved = set(types)
    while unresolved:
        updated = []
        for decl in unresolved:
            resolved = analyze_decl(decl)
            if resolved is None:
                # The decl should be skipped or ignored.
                types[decl] = IGNORED
                updated.append(decl)
                continue
            typedeps, _ = resolved
            if typedeps is None:
                raise NotImplementedError(decl)
            if UNKNOWN in typedeps:
                # At least one dependency is unknown, so this decl
                # is not resolvable.
                types[decl] = UNKNOWN
                updated.append(decl)
                continue
            if None in typedeps:
                # XXX
                # Handle direct recursive types first.
                nonrecursive = 1
                if decl.kind is KIND.STRUCT or decl.kind is KIND.UNION:
                    nonrecursive = 0
                    i = 0
                    for member, dep in zip(decl.members, typedeps):
                        if dep is None:
                            if member.vartype.typespec != decl.shortkey:
                                nonrecursive += 1
                            else:
                                typedeps[i] = decl
                        i += 1
                if nonrecursive:
                    # We don't have all dependencies resolved yet.
                    continue
            types[decl] = resolved
            updated.append(decl)
        if updated:
            for decl in updated:
                unresolved.remove(decl)
        else:
            # XXX
            # Handle indirect recursive types.
            ...
            # We couldn't resolve the rest.
            # Let the caller deal with it!
            break
    if unresolved and handle_unresolved:
        if handle_unresolved is True:
            handle_unresolved = _handle_unresolved
        handle_unresolved(unresolved, types, analyze_decl)


def resolve_decl(decl, typespecs, knowntypespecs, types):
    if decl.kind is KIND.ENUM:
        typedeps = []
    else:
        if decl.kind is KIND.VARIABLE:
            vartypes = [decl.vartype]
        elif decl.kind is KIND.FUNCTION:
            vartypes = [decl.signature.returntype]
        elif decl.kind is KIND.TYPEDEF:
            vartypes = [decl.vartype]
        elif decl.kind is KIND.STRUCT or decl.kind is KIND.UNION:
            vartypes = [m.vartype for m in decl.members]
        else:
            # Skip this one!
            return None

        typedeps = []
        for vartype in vartypes:
            typespec = vartype.typespec
            if is_pots(typespec):
                typedecl = POTSType(typespec)
            elif is_system_type(typespec):
                typedecl = SystemType(typespec)
            elif is_funcptr(vartype):
                typedecl = FuncPtr(vartype)
            else:
                typedecl = find_typedecl(decl, typespec, typespecs)
                if typedecl is None:
                    typedecl = find_typedecl(decl, typespec, knowntypespecs)
                elif not isinstance(typedecl, TypeDeclaration):
                    raise NotImplementedError(repr(typedecl))
                if typedecl is None:
                    # We couldn't find it!
                    typedecl = UNKNOWN
                elif typedecl not in types:
                    # XXX How can this happen?
                    typedecl = UNKNOWN
                elif types[typedecl] is UNKNOWN:
                    typedecl = UNKNOWN
                elif types[typedecl] is IGNORED:
                    # We don't care if it didn't resolve.
                    pass
                elif types[typedecl] is None:
                    # The typedecl for the typespec hasn't been resolved yet.
                    typedecl = None
            typedeps.append(typedecl)
    return typedeps


def find_typedecl(decl, typespec, typespecs):
    specdecls = typespecs.get(typespec)
    if not specdecls:
        return None

    filename = decl.filename

    if len(specdecls) == 1:
        typedecl, = specdecls
        if '-' in typespec and typedecl.filename != filename:
            # Inlined types are always in the same file.
            return None
        return typedecl

    # Decide which one to return.
    candidates = []
    samefile = None
    for typedecl in specdecls:
        type_filename = typedecl.filename
        if type_filename == filename:
            if samefile is not None:
                # We expect type names to be unique in a file.
                raise NotImplementedError((decl, samefile, typedecl))
            samefile = typedecl
        elif filename.endswith('.c') and not type_filename.endswith('.h'):
            # If the decl is in a source file then we expect the
            # type to be in the same file or in a header file.
            continue
        candidates.append(typedecl)
    if not candidates:
        return None
    elif len(candidates) == 1:
        winner, = candidates
        # XXX Check for inline?
    elif '-' in typespec:
        # Inlined types are always in the same file.
        winner = samefile
    elif samefile is not None:
        # Favor types in the same file.
        winner = samefile
    else:
        # We don't know which to return.
        raise NotImplementedError((decl, candidates))

    return winner


#############################
# handling unresolved decls

class Skipped(TypeDeclaration):
    def __init__(self):
        _file = _name = _data = _parent = None
        super().__init__(_file, _name, _data, _parent, _shortkey='<skipped>')
_SKIPPED = Skipped()
del Skipped


def _handle_unresolved(unresolved, types, analyze_decl):
    #raise NotImplementedError(unresolved)

    dump = True
    dump = False
    if dump:
        print()
    for decl in types:  # Preserve the original order.
        if decl not in unresolved:
            assert types[decl] is not None, decl
            if types[decl] in (UNKNOWN, IGNORED):
                unresolved.add(decl)
                if dump:
                    _dump_unresolved(decl, types, analyze_decl)
                    print()
            else:
                assert types[decl][0] is not None, (decl, types[decl])
                assert None not in types[decl][0], (decl, types[decl])
        else:
            assert types[decl] is None
            if dump:
                _dump_unresolved(decl, types, analyze_decl)
                print()
    #raise NotImplementedError

    for decl in unresolved:
        types[decl] = ([_SKIPPED], None)

    for decl in types:
        assert types[decl]


def _dump_unresolved(decl, types, analyze_decl):
    if isinstance(decl, str):
        typespec = decl
        decl, = (d for d in types if d.shortkey == typespec)
    elif type(decl) is tuple:
        filename, typespec = decl
        if '-' in typespec:
            found = [d for d in types
                     if d.shortkey == typespec and d.filename == filename]
            #if not found:
            #    raise NotImplementedError(decl)
            decl, = found
        else:
            found = [d for d in types if d.shortkey == typespec]
            if not found:
                print(f'*** {typespec} ???')
                return
                #raise NotImplementedError(decl)
            else:
                decl, = found
    resolved = analyze_decl(decl)
    if resolved:
        typedeps, _ = resolved or (None, None)

    if decl.kind is KIND.STRUCT or decl.kind is KIND.UNION:
        print(f'*** {decl.shortkey} {decl.filename}')
        for member, mtype in zip(decl.members, typedeps):
            typespec = member.vartype.typespec
            if typespec == decl.shortkey:
                print(f'     ~~~~: {typespec:20} - {member!r}')
                continue
            status = None
            if is_pots(typespec):
                mtype = typespec
                status = 'okay'
            elif is_system_type(typespec):
                mtype = typespec
                status = 'okay'
            elif mtype is None:
                if '-' in member.vartype.typespec:
                    mtype, = [d for d in types
                              if d.shortkey == member.vartype.typespec
                              and d.filename == decl.filename]
                else:
                    found = [d for d in types
                             if d.shortkey == typespec]
                    if not found:
                        print(f' ???: {typespec:20}')
                        continue
                    mtype, = found
            if status is None:
                status = 'okay' if types.get(mtype) else 'oops'
            if mtype is _SKIPPED:
                status = 'okay'
                mtype = '<skipped>'
            elif isinstance(mtype, FuncPtr):
                status = 'okay'
                mtype = str(mtype.vartype)
            elif not isinstance(mtype, str):
                if hasattr(mtype, 'vartype'):
                    if is_funcptr(mtype.vartype):
                        status = 'okay'
                mtype = str(mtype).rpartition('(')[0].rstrip()
            status = '    okay' if status == 'okay' else f'--> {status}'
            print(f' {status}: {typespec:20} - {member!r} ({mtype})')
    else:
        print(f'*** {decl} ({decl.vartype!r})')
        if decl.vartype.typespec.startswith('struct ') or is_funcptr(decl):
            _dump_unresolved(
                (decl.filename, decl.vartype.typespec),
                types,
                analyze_decl,
            )
