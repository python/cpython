# Regular expression patterns for C syntax.
#
# None of these patterns has any capturing.  However, a number of them
# have capturing markers compatible with utils.set_capture_groups().

import textwrap


def _ind(text, level=1, edges='both'):
    indent = '    ' * level
    text = textwrap.indent(text, indent)
    if edges == 'pre' or edges == 'both':
        text = '\n' + indent + text.lstrip()
    if edges == 'post' or edges == 'both':
        text = text.rstrip() + '\n' + '    ' * (level - 1)
    return text


#######################################
# general

HEX = r'(?: [0-9a-zA-Z] )'

STRING_LITERAL = textwrap.dedent(rf'''
    (?:
        # character literal
        (?:
            ['] [^'] [']
            |
            ['] \\ . [']
            |
            ['] \\x{HEX}{HEX} [']
            |
            ['] \\0\d\d [']
            |
            (?:
                ['] \\o[01]\d\d [']
                |
                ['] \\o2[0-4]\d [']
                |
                ['] \\o25[0-5] [']
             )
         )
        |
        # string literal
        (?:
            ["] (?: [^"\\]* \\ . )* [^"\\]* ["]
         )
        # end string literal
     )
    ''')

_KEYWORD = textwrap.dedent(r'''
    (?:
        \b
        (?:
            auto |
            extern |
            register |
            static |
            typedef |

            const |
            volatile |

            signed |
            unsigned |
            char |
            short |
            int |
            long |
            float |
            double |
            void |

            struct |
            union |
            enum |

            goto |
            return |
            sizeof |
            break |
            continue |
            if |
            else |
            for |
            do |
            while |
            switch |
            case |
            default |
            entry
         )
        \b
     )
    ''')
KEYWORD = rf'''
    # keyword
    {_KEYWORD}
    # end keyword
    '''
_KEYWORD = ''.join(_KEYWORD.split())

IDENTIFIER = r'(?: [a-zA-Z_][a-zA-Z0-9_]* )'
# We use a negative lookahead to filter out keywords.
STRICT_IDENTIFIER = rf'(?: (?! {_KEYWORD} ) \b {IDENTIFIER} \b )'
ANON_IDENTIFIER = rf'(?: (?! {_KEYWORD} ) \b {IDENTIFIER} (?: - \d+ )? \b )'


#######################################
# types

SIMPLE_TYPE = textwrap.dedent(rf'''
    # simple type
    (?:
        \b
        (?:
            void
            |
            (?: signed | unsigned )  # implies int
            |
            (?:
                (?: (?: signed | unsigned ) \s+ )?
                (?: (?: long | short ) \s+ )?
                (?: char | short | int | long | float | double )
             )
         )
        \b
     )
    # end simple type
    ''')

COMPOUND_TYPE_KIND = r'(?: \b (?: struct | union | enum ) \b )'


#######################################
# variable declarations

STORAGE_CLASS = r'(?: \b (?: auto | register | static | extern ) \b )'
TYPE_QUALIFIER = r'(?: \b (?: const | volatile ) \b )'
PTR_QUALIFIER = rf'(?: [*] (?: \s* {TYPE_QUALIFIER} )? )'

TYPE_SPEC = textwrap.dedent(rf'''
    # type spec
    (?:
        {_ind(SIMPLE_TYPE, 2)}
        |
        (?:
            [_]*typeof[_]*
            \s* [(]
            (?: \s* [*&] )*
            \s* {STRICT_IDENTIFIER}
            \s* [)]
         )
        |
        # reference to a compound type
        (?:
            {COMPOUND_TYPE_KIND}
            (?: \s* {ANON_IDENTIFIER} )?
         )
        |
        # reference to a typedef
        {STRICT_IDENTIFIER}
     )
    # end type spec
    ''')

DECLARATOR = textwrap.dedent(rf'''
    # declarator  (possibly abstract)
    (?:
        (?: {PTR_QUALIFIER} \s* )*
        (?:
            (?:
                (?:  # <IDENTIFIER>
                    {STRICT_IDENTIFIER}
                )
                (?: \s* \[ (?: \s* [^\]]+ \s* )? [\]] )*  # arrays
             )
            |
            (?:
                [(] \s*
                (?:  # <WRAPPED_IDENTIFIER>
                    {STRICT_IDENTIFIER}
                )
                (?: \s* \[ (?: \s* [^\]]+ \s* )? [\]] )*  # arrays
                \s* [)]
             )
            |
            # func ptr
            (?:
                [(] (?: \s* {PTR_QUALIFIER} )? \s*
                (?:  # <FUNC_IDENTIFIER>
                    {STRICT_IDENTIFIER}
                )
                (?: \s* \[ (?: \s* [^\]]+ \s* )? [\]] )*  # arrays
                \s* [)]
                # We allow for a single level of paren nesting in parameters.
                \s* [(] (?: [^()]* [(] [^)]* [)] )* [^)]* [)]
             )
         )
     )
    # end declarator
    ''')

VAR_DECL = textwrap.dedent(rf'''
    # var decl (and typedef and func return type)
    (?:
        (?:
            (?:  # <STORAGE>
                {STORAGE_CLASS}
            )
            \s*
        )?
        (?:
            (?:  # <TYPE_QUAL>
                {TYPE_QUALIFIER}
            )
            \s*
         )?
        (?:
            (?:  # <TYPE_SPEC>
                {_ind(TYPE_SPEC, 4)}
            )
         )
        \s*
        (?:
            (?:  # <DECLARATOR>
                {_ind(DECLARATOR, 4)}
            )
         )
     )
    # end var decl
    ''')

INITIALIZER = textwrap.dedent(rf'''
    # initializer
    (?:
        (?:
            [(]
            # no nested parens (e.g. func ptr)
            [^)]*
            [)]
            \s*
         )?
        (?:
            # a string literal
            (?:
                (?: {_ind(STRING_LITERAL, 4)} \s* )*
                {_ind(STRING_LITERAL, 4)}
             )
            |

            # a simple initializer
            (?:
                (?:
                    [^'",;{{]*
                    {_ind(STRING_LITERAL, 4)}
                 )*
                [^'",;{{]*
             )
            |

            # a struct/array literal
            (?:
                # We only expect compound initializers with
                # single-variable declarations.
                {{
                (?:
                    [^'";]*?
                    {_ind(STRING_LITERAL, 5)}
                 )*
                [^'";]*?
                }}
                (?= \s* ; )  # Note this lookahead.
             )
         )
     )
    # end initializer
    ''')


#######################################
# compound type declarations

STRUCT_MEMBER_DECL = textwrap.dedent(rf'''
    (?:
        # inline compound type decl
        (?:
            (?:  # <COMPOUND_TYPE_KIND>
                {COMPOUND_TYPE_KIND}
             )
            (?:
                \s+
                (?:  # <COMPOUND_TYPE_NAME>
                    {STRICT_IDENTIFIER}
                 )
             )?
            \s* {{
         )
        |
        (?:
            # typed member
            (?:
                # Technically it doesn't have to have a type...
                (?:  # <SPECIFIER_QUALIFIER>
                    (?: {TYPE_QUALIFIER} \s* )?
                    {_ind(TYPE_SPEC, 5)}
                 )
                (?:
                    # If it doesn't have a declarator then it will have
                    # a size and vice versa.
                    \s*
                    (?:  # <DECLARATOR>
                        {_ind(DECLARATOR, 6)}
                     )
                 )?
            )

            # sized member
            (?:
                \s* [:] \s*
                (?:  # <SIZE>
                    \d+
                 )
             )?
            \s*
            (?:  # <ENDING>
                [,;]
             )
         )
        |
        (?:
            \s*
            (?:  # <CLOSE>
                }}
             )
         )
     )
    ''')

ENUM_MEMBER_DECL = textwrap.dedent(rf'''
    (?:
        (?:
            \s*
            (?:  # <CLOSE>
                }}
             )
         )
        |
        (?:
            \s*
            (?:  # <NAME>
                {IDENTIFIER}
             )
            (?:
                \s* = \s*
                (?:  # <INIT>
                    {_ind(STRING_LITERAL, 4)}
                    |
                    [^'",}}]+
                 )
             )?
            \s*
            (?:  # <ENDING>
                , | }}
             )
         )
     )
    ''')


#######################################
# statements

SIMPLE_STMT_BODY = textwrap.dedent(rf'''
    # simple statement body
    (?:
        (?:
            [^'"{{}};]*
            {_ind(STRING_LITERAL, 3)}
         )*
        [^'"{{}};]*
        #(?= [;{{] )  # Note this lookahead.
     )
    # end simple statement body
    ''')
SIMPLE_STMT = textwrap.dedent(rf'''
    # simple statement
    (?:
        (?:  # <SIMPLE_STMT>
            # stmt-inline "initializer"
            (?:
                return \b
                (?:
                    \s*
                    {_ind(INITIALIZER, 5)}
                )?
             )
            |
            # variable assignment
            (?:
                (?: [*] \s* )?
                (?:
                    {STRICT_IDENTIFIER} \s*
                    (?: . | -> ) \s*
                 )*
                {STRICT_IDENTIFIER}
                (?: \s* \[ \s* \d+ \s* \] )?
                \s* = \s*
                {_ind(INITIALIZER, 4)}
             )
            |
            # catchall return statement
            (?:
                return \b
                (?:
                    (?:
                        [^'";]*
                        {_ind(STRING_LITERAL, 6)}
                     )*
                    \s* [^'";]*
                 )?
             )
            |
            # simple statement
            (?:
                {_ind(SIMPLE_STMT_BODY, 4)}
             )
         )
        \s*
        (?:  # <SIMPLE_ENDING>
            ;
         )
     )
    # end simple statement
    ''')
COMPOUND_STMT = textwrap.dedent(rf'''
    # compound statement
    (?:
        \b
        (?:
            (?:
                (?:  # <COMPOUND_BARE>
                    else | do
                 )
                \b
             )
            |
            (?:
                (?:  # <COMPOUND_LABELED>
                    (?:
                        case \b
                        (?:
                            [^'":]*
                            {_ind(STRING_LITERAL, 7)}
                         )*
                        \s* [^'":]*
                     )
                    |
                    default
                    |
                    {STRICT_IDENTIFIER}
                 )
                \s* [:]
             )
            |
            (?:
                (?:  # <COMPOUND_PAREN>
                    for | while | if | switch
                 )
                \s* (?= [(] )  # Note this lookahead.
             )
         )
        \s*
     )
    # end compound statement
    ''')


#######################################
# function bodies

LOCAL = textwrap.dedent(rf'''
    (?:
        # an empty statement
        (?:  # <EMPTY>
            ;
         )
        |
        # inline type decl
        (?:
            (?:
                (?:  # <INLINE_LEADING>
                    [^;{{}}]+?
                 )
                \s*
             )?
            (?:  # <INLINE_PRE>
                (?: {STORAGE_CLASS} \s* )?
                (?: {TYPE_QUALIFIER} \s* )?
             )?  # </INLINE_PRE>
            (?:  # <INLINE_KIND>
                {COMPOUND_TYPE_KIND}
             )
            (?:
                \s+
                (?:  # <INLINE_NAME>
                    {STRICT_IDENTIFIER}
                 )
             )?
            \s* {{
         )
        |
        # var decl
        (?:
            (?:  # <STORAGE>
                {STORAGE_CLASS}
             )?  # </STORAGE>
            (?:
                \s*
                (?:  # <VAR_DECL>
                    {_ind(VAR_DECL, 5)}
                 )
             )
            (?:
                (?:
                    # initializer
                    # We expect only basic initializers.
                    \s* = \s*
                    (?:  # <VAR_INIT>
                        {_ind(INITIALIZER, 6)}
                     )
                 )?
                (?:
                    \s*
                    (?:  # <VAR_ENDING>
                        [,;]
                     )
                 )
             )
         )
        |
        {_ind(COMPOUND_STMT, 2)}
        |
        # start-of-block
        (?:
            (?:  # <BLOCK_LEADING>
                (?:
                    [^'"{{}};]*
                    {_ind(STRING_LITERAL, 5)}
                 )*
                [^'"{{}};]*
                # Presumably we will not see "== {{".
                [^\s='"{{}});]
                \s*
             )?  # </BLOCK_LEADING>
            (?:  # <BLOCK_OPEN>
                {{
             )
         )
        |
        {_ind(SIMPLE_STMT, 2)}
        |
        # end-of-block
        (?:  # <BLOCK_CLOSE>
            }}
         )
     )
    ''')

LOCAL_STATICS = textwrap.dedent(rf'''
    (?:
        # inline type decl
        (?:
            (?:
                (?:  # <INLINE_LEADING>
                    [^;{{}}]+?
                 )
                \s*
             )?
            (?:  # <INLINE_PRE>
                (?: {STORAGE_CLASS} \s* )?
                (?: {TYPE_QUALIFIER} \s* )?
             )?
            (?:  # <INLINE_KIND>
                {COMPOUND_TYPE_KIND}
             )
            (?:
                \s+
                (?:  # <INLINE_NAME>
                    {STRICT_IDENTIFIER}
                 )
             )?
            \s* {{
         )
        |
        # var decl
        (?:
            # We only look for static variables.
            (?:  # <STATIC_DECL>
                static \b
                (?: \s* {TYPE_QUALIFIER} )?
                \s* {_ind(TYPE_SPEC, 4)}
                \s* {_ind(DECLARATOR, 4)}
             )
            \s*
            (?:
                (?:  # <STATIC_INIT>
                    = \s*
                    {_ind(INITIALIZER, 4)}
                    \s*
                    [,;{{]
                 )
                |
                (?:  # <STATIC_ENDING>
                    [,;]
                 )
             )
         )
        |
        # everything else
        (?:
            (?:  # <DELIM_LEADING>
                (?:
                    [^'"{{}};]*
                    {_ind(STRING_LITERAL, 4)}
                 )*
                \s* [^'"{{}};]*
             )
            (?:
                (?:  # <BLOCK_OPEN>
                    {{
                 )
                |
                (?:  # <BLOCK_CLOSE>
                    }}
                 )
                |
                (?:  # <STMT_END>
                    ;
                 )
             )
         )
     )
    ''')


#######################################
# global declarations

GLOBAL = textwrap.dedent(rf'''
    (?:
        # an empty statement
        (?:  # <EMPTY>
            ;
         )
        |

        # compound type decl (maybe inline)
        (?:
            (?:
                (?:  # <COMPOUND_LEADING>
                    [^;{{}}]+?
                 )
                 \s*
             )?
            (?:  # <COMPOUND_KIND>
                {COMPOUND_TYPE_KIND}
             )
            (?:
                \s+
                (?:  # <COMPOUND_NAME>
                    {STRICT_IDENTIFIER}
                 )
             )?
            \s* {{
         )
        |
        # bogus inline decl artifact
        # This simplifies resolving the relative syntactic ambiguity of
        # inline structs.
        (?:
            (?:  # <FORWARD_KIND>
                {COMPOUND_TYPE_KIND}
             )
            \s*
            (?:  # <FORWARD_NAME>
                {ANON_IDENTIFIER}
             )
            (?:  # <MAYBE_INLINE_ACTUAL>
                [^=,;({{[*\]]*
                [=,;({{]
             )
         )
        |

        # typedef
        (?:
            \b typedef \b \s*
            (?:  # <TYPEDEF_DECL>
                {_ind(VAR_DECL, 4)}
             )
            (?:
                # We expect no inline type definitions in the parameters.
                \s* [(] \s*
                (?:  # <TYPEDEF_FUNC_PARAMS>
                    [^{{;]*
                 )
                \s* [)]
             )?
            \s* ;
         )
        |

        # func decl/definition & var decls
        # XXX dedicated pattern for funcs (more restricted)?
        (?:
            (?:
                (?:  # <VAR_STORAGE>
                    {STORAGE_CLASS}
                 )
                \s*
             )?
            (?:
                (?:  # <FUNC_INLINE>
                    \b inline \b
                 )
                \s*
             )?
            (?:  # <VAR_DECL>
                {_ind(VAR_DECL, 4)}
             )
            (?:
                # func decl / definition
                (?:
                    (?:
                        # We expect no inline type definitions in the parameters.
                        \s* [(] \s*
                        (?:  # <FUNC_PARAMS>
                            [^{{;]*
                         )
                        \s* [)] \s*
                        (?:  # <FUNC_DELIM>
                            [{{;]
                         )
                     )
                    |
                    (?:
                        # This is some old-school syntax!
                        \s* [(] \s*
                        # We throw away the bare names:
                        {STRICT_IDENTIFIER}
                        (?: \s* , \s* {STRICT_IDENTIFIER} )*
                        \s* [)] \s*

                        # We keep the trailing param declarations:
                        (?:  # <FUNC_LEGACY_PARAMS>
                            # There's at least one!
                            (?: {TYPE_QUALIFIER} \s* )?
                            {_ind(TYPE_SPEC, 7)}
                            \s*
                            {_ind(DECLARATOR, 7)}
                            \s* ;
                            (?:
                                \s*
                                (?: {TYPE_QUALIFIER} \s* )?
                                {_ind(TYPE_SPEC, 8)}
                                \s*
                                {_ind(DECLARATOR, 8)}
                                \s* ;
                             )*
                         )
                        \s* {{
                     )
                 )
                |
                # var / typedef
                (?:
                    (?:
                        # initializer
                        # We expect only basic initializers.
                        \s* = \s*
                        (?:  # <VAR_INIT>
                            {_ind(INITIALIZER, 6)}
                         )
                     )?
                    \s*
                    (?:  # <VAR_ENDING>
                        [,;]
                     )
                 )
             )
         )
     )
    ''')
