import re

from ._regexes import (
    GLOBAL as _GLOBAL,
)
from ._common import (
    log_match,
    parse_var_decl,
    set_capture_groups,
)
from ._compound_decl_body import DECL_BODY_PARSERS
#from ._func_body import parse_function_body
from ._func_body import parse_function_statics as parse_function_body


GLOBAL = set_capture_groups(_GLOBAL, (
    'EMPTY',
    'COMPOUND_LEADING',
    'COMPOUND_KIND',
    'COMPOUND_NAME',
    'FORWARD_KIND',
    'FORWARD_NAME',
    'MAYBE_INLINE_ACTUAL',
    'TYPEDEF_DECL',
    'TYPEDEF_FUNC_PARAMS',
    'VAR_STORAGE',
    'FUNC_INLINE',
    'VAR_DECL',
    'FUNC_PARAMS',
    'FUNC_DELIM',
    'FUNC_LEGACY_PARAMS',
    'VAR_INIT',
    'VAR_ENDING',
))
GLOBAL_RE = re.compile(rf'^ \s* {GLOBAL}', re.VERBOSE)


def parse_globals(source, anon_name):
    for srcinfo in source:
        m = GLOBAL_RE.match(srcinfo.text)
        if not m:
            # We need more text.
            continue
        for item in _parse_next(m, srcinfo, anon_name):
            if callable(item):
                parse_body = item
                yield from parse_body(source)
            else:
                yield item
    else:
        # We ran out of lines.
        if srcinfo is not None:
            srcinfo.done()
        return


def _parse_next(m, srcinfo, anon_name):
    (
     empty,
     # compound type decl (maybe inline)
     compound_leading, compound_kind, compound_name,
     forward_kind, forward_name, maybe_inline_actual,
     # typedef
     typedef_decl, typedef_func_params,
     # vars and funcs
     storage, func_inline, decl,
     func_params, func_delim, func_legacy_params,
     var_init, var_ending,
     ) = m.groups()
    remainder = srcinfo.text[m.end():]

    if empty:
        log_match('global empty', m)
        srcinfo.advance(remainder)

    elif maybe_inline_actual:
        log_match('maybe_inline_actual', m)
        # Ignore forward declarations.
        # XXX Maybe return them too (with an "isforward" flag)?
        if not maybe_inline_actual.strip().endswith(';'):
            remainder = maybe_inline_actual + remainder
        yield srcinfo.resolve(forward_kind, None, forward_name)
        if maybe_inline_actual.strip().endswith('='):
            # We use a dummy prefix for a fake typedef.
            # XXX Ideally this case would not be caught by MAYBE_INLINE_ACTUAL.
            _, name, data = parse_var_decl(f'{forward_kind} {forward_name} fake_typedef_{forward_name}')
            yield srcinfo.resolve('typedef', data, name, parent=None)
            remainder = f'{name} {remainder}'
        srcinfo.advance(remainder)

    elif compound_kind:
        kind = compound_kind
        name = compound_name or anon_name('inline-')
        # Immediately emit a forward declaration.
        yield srcinfo.resolve(kind, name=name, data=None)

        # un-inline the decl.  Note that it might not actually be inline.
        # We handle the case in the "maybe_inline_actual" branch.
        srcinfo.nest(
            remainder,
            f'{compound_leading or ""} {compound_kind} {name}',
        )
        def parse_body(source):
            _parse_body = DECL_BODY_PARSERS[compound_kind]

            data = []  # members
            ident = f'{kind} {name}'
            for item in _parse_body(source, anon_name, ident):
                if item.kind == 'field':
                    data.append(item)
                else:
                    yield item
            # XXX Should "parent" really be None for inline type decls?
            yield srcinfo.resolve(kind, data, name, parent=None)

            srcinfo.resume()
        yield parse_body

    elif typedef_decl:
        log_match('typedef', m)
        kind = 'typedef'
        _, name, data = parse_var_decl(typedef_decl)
        if typedef_func_params:
            return_type = data
            # This matches the data for func declarations.
            data = {
                'storage': None,
                'inline': None,
                'params': f'({typedef_func_params})',
                'returntype': return_type,
                'isforward': True,
            }
        yield srcinfo.resolve(kind, data, name, parent=None)
        srcinfo.advance(remainder)

    elif func_delim or func_legacy_params:
        log_match('function', m)
        kind = 'function'
        _, name, return_type = parse_var_decl(decl)
        func_params = func_params or func_legacy_params
        data = {
            'storage': storage,
            'inline': func_inline,
            'params': f'({func_params})',
            'returntype': return_type,
            'isforward': func_delim == ';',
        }

        yield srcinfo.resolve(kind, data, name, parent=None)
        srcinfo.advance(remainder)

        if func_delim == '{' or func_legacy_params:
            def parse_body(source):
                yield from parse_function_body(source, name, anon_name)
            yield parse_body

    elif var_ending:
        log_match('global variable', m)
        kind = 'variable'
        _, name, vartype = parse_var_decl(decl)
        data = {
            'storage': storage,
            'vartype': vartype,
        }
        yield srcinfo.resolve(kind, data, name, parent=None)

        if var_ending == ',':
            # It was a multi-declaration, so queue up the next one.
            _, qual, typespec, _ = vartype.values()
            remainder = f'{storage or ""} {qual or ""} {typespec} {remainder}'
        srcinfo.advance(remainder)

        if var_init:
            _data = f'{name} = {var_init.strip()}'
            yield srcinfo.resolve('statement', _data, name=None)

    else:
        # This should be unreachable.
        raise NotImplementedError
