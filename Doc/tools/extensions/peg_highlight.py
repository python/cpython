from pygments.lexer import RegexLexer, bygroups, include
from pygments.token import Comment, Keyword, Name, Operator, Punctuation, Text

from sphinx.highlighting import lexers


class PEGLexer(RegexLexer):
    """Pygments Lexer for PEG grammar (.gram) files

    This lexer strips the following elements from the grammar:

        - Meta-tags
        - Variable assignments
        - Actions
        - Lookaheads
        - Rule types
        - Rule options
        - Rules named `invalid_*` or `incorrect_*`
    """

    name = "PEG"
    aliases = ["peg"]
    filenames = ["*.gram"]
    _name = r"([^\W\d]\w*)"
    _text_ws = r"(\s*)"

    tokens = {
        "ws": [(r"\n", Text), (r"\s+", Text), (r"#.*$", Comment.Singleline),],
        "lookaheads": [
            # Forced tokens
            (r"(&&)(?=\w+\s?)", bygroups(None)),
            (r"(&&)(?='.+'\s?)", bygroups(None)),
            (r'(&&)(?=".+"\s?)', bygroups(None)),
            (r"(&&)(?=\(.+\)\s?)", bygroups(None)),

            (r"(?<=\|\s)(&\w+\s?)", bygroups(None)),
            (r"(?<=\|\s)(&'.+'\s?)", bygroups(None)),
            (r'(?<=\|\s)(&".+"\s?)', bygroups(None)),
            (r"(?<=\|\s)(&\(.+\)\s?)", bygroups(None)),
        ],
        "metas": [
            (r"(@\w+ '''(.|\n)+?''')", bygroups(None)),
            (r"^(@.*)$", bygroups(None)),
        ],
        "actions": [
            (r"{(.|\n)+?}", bygroups(None)),
        ],
        "strings": [
            (r"'\w+?'", Keyword),
            (r'"\w+?"', Keyword),
            (r"'\W+?'", Text),
            (r'"\W+?"', Text),
        ],
        "variables": [
            (_name + _text_ws + "(=)", bygroups(None, None, None),),
            (_name + _text_ws + r"(\[[\w\d_\*]+?\])" + _text_ws + "(=)", bygroups(None, None, None, None, None),),
        ],
        "invalids": [
            (r"^(\s+\|\s+.*invalid_\w+.*\n)", bygroups(None)),
            (r"^(\s+\|\s+.*incorrect_\w+.*\n)", bygroups(None)),
            (r"^(#.*invalid syntax.*(?:.|\n)*)", bygroups(None),),
        ],
        "root": [
            include("invalids"),
            include("ws"),
            include("lookaheads"),
            include("metas"),
            include("actions"),
            include("strings"),
            include("variables"),
            (r"\b(?!(NULL|EXTRA))([A-Z_]+)\b\s*(?!\()", Text,),
            (
                r"^\s*" + _name + r"\s*" + r"(\[.*\])?" + r"\s*" + r"(\(.+\))?" + r"\s*(:)",
                bygroups(Name.Function, None, None, Punctuation),
            ),
            (_name, Name.Function),
            (r"[\||\.|\+|\*|\?]", Operator),
            (r"{|}|\(|\)|\[|\]", Punctuation),
            (r".", Text),
        ],
    }


def setup(app):
    lexers["peg"] = PEGLexer()
    return {"version": "1.0", "parallel_read_safe": True}
