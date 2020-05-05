import os
import sys
sys.path.append(os.path.abspath("../Parser/"))

from pygments.lexer import RegexLexer, bygroups, include, words
from pygments.token import (Comment, Generic, Keyword, Name, Operator,
                            Punctuation, Text)

from asdl import builtin_types
from sphinx.highlighting import lexers

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
                bygroups(Keyword, Text, Name.Class),
            ),
            (
                r"(\w+)(\*\s|\?\s|\s)(\w+)",
                bygroups(Name.Variable, Generic.Strong, Name.Tag),
            ),
            (words(builtin_types), Keyword.Type),
            (r"attributes", Name.Builtin),
            (
                _name + _text_ws + "(=)",
                bygroups(Name.Variable, Text, Operator),
            ),
            (_name, Name.Function),
            (r"\|", Operator),
            (r"{|}|\(|\)", Punctuation),
            (r".", Text),
        ],
    }


def setup(app):
    lexers["asdl"] = ASDLLexer()
    return {'version': '1.0', 'parallel_read_safe': True}
