import re

from ._regexes import (
    LOCAL as _LOCAL,
    LOCAL_STATICS as _LOCAL_STATICS,
)
from ._common import (
    log_match,
    parse_var_decl,
    set_capture_groups,
    match_paren,
)
from ._compound_decl_body import DECL_BODY_PARSERS


LOCAL = set_capture_groups(_LOCAL, (
    'EMPTY',
    'INLINE_LEADING',
    'INLINE_PRE',
    'INLINE_KIND',
    'INLINE_NAME',
    'STORAGE',
    'VAR_DECL',
    'VAR_INIT',
    'VAR_ENDING',
    'COMPOUND_BARE',
    'COMPOUND_LABELED',
    'COMPOUND_PAREN',
    'BLOCK_LEADING',
    'BLOCK_OPEN',
    'SIMPLE_STMT',
    'SIMPLE_ENDING',
    'BLOCK_CLOSE',
))
LOCAL_RE = re.compile(rf'^ \s* {LOCAL}', re.VERBOSE)


# Note that parse_function_body() still has trouble with a few files
# in the CPython codebase.

def parse_function_body(source, name, anon_name):
    # XXX
    raise NotImplementedError


def parse_function_body(name, text, resolve, source, anon_name, parent):
    raise NotImplementedError
    # For now we do not worry about locals declared in for loop "headers".
    depth = 1;
    while depth > 0:
        m = LOCAL_RE.match(text)
        while not m:
            text, resolve = continue_text(source, text or '{', resolve)
            m = LOCAL_RE.match(text)
        text = text[m.end():]
        (
         empty,
         inline_leading, inline_pre, inline_kind, inline_name,
         storage, decl,
         var_init, var_ending,
         compound_bare, compound_labeled, compound_paren,
         block_leading, block_open,
         simple_stmt, simple_ending,
         block_close,
         ) = m.groups()

        if empty:
            log_match('', m)
            resolve(None, None, None, text)
            yield None, text
        elif inline_kind:
            log_match('', m)
            kind = inline_kind
            name = inline_name or anon_name('inline-')
            data = []  # members
            # We must set the internal "text" from _iter_source() to the
            # start of the inline compound body,
            # Note that this is effectively like a forward reference that
            # we do not emit.
            resolve(kind, None, name, text, None)
            _parse_body = DECL_BODY_PARSERS[kind]
            before = []
            ident = f'{kind} {name}'
            for member, inline, text in _parse_body(text, resolve, source, anon_name, ident):
                if member:
                    data.append(member)
                if inline:
                    yield from inline
            # un-inline the decl.  Note that it might not actually be inline.
            # We handle the case in the "maybe_inline_actual" branch.
            text = f'{inline_leading or ""} {inline_pre or ""} {kind} {name} {text}'
            # XXX Should "parent" really be None for inline type decls?
            yield resolve(kind, data, name, text, None), text
        elif block_close:
            log_match('', m)
            depth -= 1
            resolve(None, None, None, text)
            # XXX This isn't great.  Calling resolve() should have
            # cleared the closing bracket.  However, some code relies
            # on the yielded value instead of the resolved one.  That
            # needs to be fixed.
            yield None, text
        elif compound_bare:
            log_match('', m)
            yield resolve('statement', compound_bare, None, text, parent), text
        elif compound_labeled:
            log_match('', m)
            yield resolve('statement', compound_labeled, None, text, parent), text
        elif compound_paren:
            log_match('', m)
            try:
                pos = match_paren(text)
            except ValueError:
                text = f'{compound_paren} {text}'
                #resolve(None, None, None, text)
                text, resolve = continue_text(source, text, resolve)
                yield None, text
            else:
                head = text[:pos]
                text = text[pos:]
                if compound_paren == 'for':
                    # XXX Parse "head" as a compound statement.
                    stmt1, stmt2, stmt3 = head.split(';', 2)
                    data = {
                        'compound': compound_paren,
                        'statements': (stmt1, stmt2, stmt3),
                    }
                else:
                    data = {
                        'compound': compound_paren,
                        'statement': head,
                    }
                yield resolve('statement', data, None, text, parent), text
        elif block_open:
            log_match('', m)
            depth += 1
            if block_leading:
                # An inline block: the last evaluated expression is used
                # in place of the block.
                # XXX Combine it with the remainder after the block close.
                stmt = f'{block_open}{{<expr>}}...;'
                yield resolve('statement', stmt, None, text, parent), text
            else:
                resolve(None, None, None, text)
                yield None, text
        elif simple_ending:
            log_match('', m)
            yield resolve('statement', simple_stmt, None, text, parent), text
        elif var_ending:
            log_match('', m)
            kind = 'variable'
            _, name, vartype = parse_var_decl(decl)
            data = {
                'storage': storage,
                'vartype': vartype,
            }
            after = ()
            if var_ending == ',':
                # It was a multi-declaration, so queue up the next one.
                _, qual, typespec, _ = vartype.values()
                text = f'{storage or ""} {qual or ""} {typespec} {text}'
            yield resolve(kind, data, name, text, parent), text
            if var_init:
                _data = f'{name} = {var_init.strip()}'
                yield resolve('statement', _data, None, text, parent), text
        else:
            # This should be unreachable.
            raise NotImplementedError


#############################
# static local variables

LOCAL_STATICS = set_capture_groups(_LOCAL_STATICS, (
    'INLINE_LEADING',
    'INLINE_PRE',
    'INLINE_KIND',
    'INLINE_NAME',
    'STATIC_DECL',
    'STATIC_INIT',
    'STATIC_ENDING',
    'DELIM_LEADING',
    'BLOCK_OPEN',
    'BLOCK_CLOSE',
    'STMT_END',
))
LOCAL_STATICS_RE = re.compile(rf'^ \s* {LOCAL_STATICS}', re.VERBOSE)


def parse_function_statics(source, func, anon_name):
    # For now we do not worry about locals declared in for loop "headers".
    depth = 1;
    while depth > 0:
        for srcinfo in source:
            m = LOCAL_STATICS_RE.match(srcinfo.text)
            if m:
                break
        else:
            # We ran out of lines.
            if srcinfo is not None:
                srcinfo.done()
            return
        for item, depth in _parse_next_local_static(m, srcinfo,
                                                    anon_name, func, depth):
            if callable(item):
                parse_body = item
                yield from parse_body(source)
            elif item is not None:
                yield item


def _parse_next_local_static(m, srcinfo, anon_name, func, depth):
    (inline_leading, inline_pre, inline_kind, inline_name,
     static_decl, static_init, static_ending,
     _delim_leading,
     block_open,
     block_close,
     stmt_end,
     ) = m.groups()
    remainder = srcinfo.text[m.end():]

    if inline_kind:
        log_match('func inline', m)
        kind = inline_kind
        name = inline_name or anon_name('inline-')
        # Immediately emit a forward declaration.
        yield srcinfo.resolve(kind, name=name, data=None), depth

        # un-inline the decl.  Note that it might not actually be inline.
        # We handle the case in the "maybe_inline_actual" branch.
        srcinfo.nest(
            remainder,
            f'{inline_leading or ""} {inline_pre or ""} {kind} {name}'
        )
        def parse_body(source):
            _parse_body = DECL_BODY_PARSERS[kind]

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
        yield parse_body, depth

    elif static_decl:
        log_match('local variable', m)
        _, name, data = parse_var_decl(static_decl)

        yield srcinfo.resolve('variable', data, name, parent=func), depth

        if static_init:
            srcinfo.advance(f'{name} {static_init} {remainder}')
        elif static_ending == ',':
            # It was a multi-declaration, so queue up the next one.
            _, qual, typespec, _ = data.values()
            srcinfo.advance(f'static {qual or ""} {typespec} {remainder}')
        else:
            srcinfo.advance('')

    else:
        log_match('func other', m)
        if block_open:
            depth += 1
        elif block_close:
            depth -= 1
        elif stmt_end:
            pass
        else:
            # This should be unreachable.
            raise NotImplementedError
        srcinfo.advance(remainder)
        yield None, depth
