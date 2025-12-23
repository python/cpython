from parsing import (  # noqa: F401
    InstDef,
    Macro,
    Pseudo,
    Family,
    LabelDef,
    Parser,
    Context,
    CacheEffect,
    StackEffect,
    InputEffect,
    OpName,
    AstNode,
    Stmt,
    SimpleStmt,
    IfStmt,
    ForStmt,
    WhileStmt,
    BlockStmt,
    MacroIfStmt,
)

import pprint

CodeDef = InstDef | LabelDef

def prettify_filename(filename: str) -> str:
    # Make filename more user-friendly and less platform-specific,
    # it is only used for error reporting at this point.
    filename = filename.replace("\\", "/")
    if filename.startswith("./"):
        filename = filename[2:]
    if filename.endswith(".new"):
        filename = filename[:-4]
    return filename


BEGIN_MARKER = "// BEGIN BYTECODES //"
END_MARKER = "// END BYTECODES //"


def parse_files(filenames: list[str]) -> list[AstNode]:
    result: list[AstNode] = []
    for filename in filenames:
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
        while node := psr.definition():
            assert node is not None
            result.append(node)  # type: ignore[arg-type]
        if not psr.eof():
            pprint.pprint(result)
            psr.backup()
            raise psr.make_syntax_error(
                f"Extra stuff at the end of {filename}", psr.next(True)
            )
    return result
