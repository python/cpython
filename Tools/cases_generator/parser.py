from parsing import InstDef, Macro, Pseudo, Family, Parser, Context, CacheEffect, StackEffect, OpName
from formatting import prettify_filename


BEGIN_MARKER = "// BEGIN BYTECODES //"
END_MARKER = "// END BYTECODES //"

AstNode = InstDef | Macro | Pseudo | Family

def parse_file(filename: str) -> None:
    with open(filename) as file:
        src = file.read()

    psr = Parser(src, filename=prettify_filename(filename))

    # Skip until begin marker
    while tkn := psr.next(raw=True):
        if tkn.text == BEGIN_MARKER:
            break
    else:
        raise psr.make_syntax_error(
            f"Couldn't find {BEGIN_MARKER!r} in {psr.filename}"
        )
    start = psr.getpos()

    # Find end marker, then delete everything after it
    while tkn := psr.next(raw=True):
        if tkn.text == END_MARKER:
            break
    del psr.tokens[psr.getpos() - 1 :]

    # Parse from start
    psr.setpos(start)
    thing_first_token = psr.peek()
    result: list[AstNode] = []
    while node := psr.definition():
        result.append(node)
    if not psr.eof():
        raise psr.make_syntax_error(f"Extra stuff at the end of {filename}")
    return result
