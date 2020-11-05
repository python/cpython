
f'''
    struct {ANON_IDENTIFIER};
    struct {{ ... }}
    struct {IDENTIFIER} {{ ... }}

    union {ANON_IDENTIFIER};
    union {{ ... }}
    union {IDENTIFIER} {{ ... }}

    enum {ANON_IDENTIFIER};
    enum {{ ... }}
    enum {IDENTIFIER} {{ ... }}

    typedef {VARTYPE} {IDENTIFIER};
    typedef {IDENTIFIER};
    typedef {IDENTIFIER};
    typedef {IDENTIFIER};
'''


def parse(srclines):
    if isinstance(srclines, str):  # a filename
        raise NotImplementedError

    

# This only handles at most 10 nested levels.
#MATCHED_PARENS = textwrap.dedent(rf'''
#    # matched parens
#    (?:
#        [(]  # level 0
#        (?:
#            [^()]*
#            [(]  # level 1
#            (?:
#                [^()]*
#                [(]  # level 2
#                (?:
#                    [^()]*
#                    [(]  # level 3
#                    (?:
#                        [^()]*
#                        [(]  # level 4
#                        (?:
#                            [^()]*
#                            [(]  # level 5
#                            (?:
#                                [^()]*
#                                [(]  # level 6
#                                (?:
#                                    [^()]*
#                                    [(]  # level 7
#                                    (?:
#                                        [^()]*
#                                        [(]  # level 8
#                                        (?:
#                                            [^()]*
#                                            [(]  # level 9
#                                            (?:
#                                                [^()]*
#                                                [(]  # level 10
#                                                [^()]*
#                                                [)]
#                                             )*
#                                            [^()]*
#                                            [)]
#                                         )*
#                                        [^()]*
#                                        [)]
#                                     )*
#                                    [^()]*
#                                    [)]
#                                 )*
#                                [^()]*
#                                [)]
#                             )*
#                            [^()]*
#                            [)]
#                         )*
#                        [^()]*
#                        [)]
#                     )*
#                    [^()]*
#                    [)]
#                 )*
#                [^()]*
#                [)]
#             )*
#            [^()]*
#            [)]
#         )*
#        [^()]*
#        [)]
#     )
#    # end matched parens
#    ''')

'''
        # for loop
        (?:
            \s* \b for
            \s* [(]
            (
                [^;]* ;
                [^;]* ;
                .*?
             )  # <header>
            [)]
            \s*
            (?:
                (?:
                    (
                        {_ind(SIMPLE_STMT, 6)}
                     )  # <stmt>
                    ;
                 )
                |
                ( {{ )  # <open>
             )
         )
        |



            (
                (?:
                    (?:
                        (?:
                            {_ind(SIMPLE_STMT, 6)}
                         )?
                        return \b \s*
                        {_ind(INITIALIZER, 5)}
                     )
                    |
                    (?:
                        (?:
                            {IDENTIFIER} \s*
                            (?: . | -> ) \s*
                         )*
                        {IDENTIFIER}
                        \s* = \s*
                        {_ind(INITIALIZER, 5)}
                     )
                    |
                    (?:
                        {_ind(SIMPLE_STMT, 5)}
                     )
                 )
                |
                # cast compound literal
                (?:
                    (?:
                        [^'"{{}};]*
                        {_ind(STRING_LITERAL, 5)}
                     )*
                    [^'"{{}};]*?
                    [^'"{{}};=]
                    =
                    \s* [(] [^)]* [)]
                    \s* {{ [^;]* }}
                 )
             )  # <stmt>



        # compound statement
        (?:
            (
                (?:

                    # "for" statements are handled separately above.
                    (?: (?: else \s+ )? if | switch | while ) \s*
                    {_ind(COMPOUND_HEAD, 5)}
                 )
                |
                (?: else | do )
                # We do not worry about compound statements for labels,
                # "case", or "default".
             )?  # <header>
            \s*
            ( {{ )  # <open>
         )



            (
                (?:
                    [^'"{{}};]*
                    {_ind(STRING_LITERAL, 5)}
                 )*
                [^'"{{}};]*
                # Presumably we will not see "== {{".
                [^\s='"{{}};]
             )?  # <header>



            (
                \b
                (?:
                    # We don't worry about labels with a compound statement.
                    (?:
                        switch \s* [(] [^{{]* [)]
                     )
                    |
                    (?:
                        case \b \s* [^:]+ [:]
                     )
                    |
                    (?:
                        default \s* [:]
                     )
                    |
                    (?:
                        do
                     )
                    |
                    (?:
                        while \s* [(] [^{{]* [)]
                     )
                    |
                    #(?:
                    #    for \s* [(] [^{{]* [)]
                    # )
                    #|
                    (?:
                        if \s* [(]
                        (?: [^{{]* [^)] \s* {{ )* [^{{]*
                        [)]
                     )
                    |
                    (?:
                        else
                        (?:
                            \s*
                            if \s* [(]
                            (?: [^{{]* [^)] \s* {{ )* [^{{]*
                            [)]
                         )?
                     )
                 )
             )?  # <header>
'''
