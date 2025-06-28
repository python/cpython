from pygments.lexer import RegexLexer, bygroups, include
from pygments.token import Comment, Keyword, Name, Operator, Punctuation, Text


class ASDLLexer(RegexLexer):
    name = "ASDL"
    aliases = ["asdl"]
    filenames = ["*.asdl"]
    _name = r"([^\W\d]\w*)"
    _text_ws = r"(\s*)"

    tokens = {
        "ws": [
            (r"\n", Text),
            (r"\s+", Text),
            (r"--.*?$", Comment.Singleline),
        ],
        "root": [
            include("ws"),
            (
                r"(module)" + _text_ws + _name,
                bygroups(Keyword, Text, Name.Tag),
            ),
            (
                r"(\w+)(\*\s|\?\s|\s)(\w+)",
                bygroups(Name.Builtin.Pseudo, Operator, Name),
            ),
            # Keep in line with ``builtin_types`` from Parser/asdl.py.
            # ASDL's 4 builtin types are
            # constant, identifier, int, string
            ("constant|identifier|int|string", Name.Builtin),
            (r"attributes", Name.Builtin),
            (
                _name + _text_ws + "(=)",
                bygroups(Name, Text, Operator),
            ),
            (_name, Name.Class),
            (r"\|", Operator),
            (r"{|}|\(|\)", Punctuation),
            (r".", Text),
        ],
    }
