# Tooling to generate interpreters

What's currently here:

- lexer.py: lexer for C, originally written by Mark Shannon
- plexer.py: OO interface on top of lexer.py; main class: `PLexer`
- parser.py: Parser for instruction definition DSL; main class `Parser`
- `generate_cases.py`: driver script to read `Python/bytecodes.c` and
  write `Python/generated_cases.c.h`

The DSL for the instruction definitions in `Python/bytecodes.c` is described
[here](https://github.com/faster-cpython/ideas/blob/main/3.12/interpreter_definition.md).
Note that there is some dummy C code at the top and bottom of the file
to fool text editors like VS Code into believing this is valid C code.

## A bit about the parser

The parser class uses a pretty standard recursive descent scheme,
but with unlimited backtracking.
The `PLexer` class tokenizes the entire input before parsing starts.
We do not run the C preprocessor.
Each parsing method returns either an AST node (a `Node` instance)
or `None`, or raises `SyntaxError` (showing the error in the C source).

Most parsing methods are decorated with `@contextual`, which automatically
resets the tokenizer input position when `None` is returned.
Parsing methods may also raise `SyntaxError`, which is irrecoverable.
When a parsing method returns `None`, it is possible that after backtracking
a different parsing method returns a valid AST.

Neither the lexer nor the parsers are complete or fully correct.
Most known issues are tersely indicated by `# TODO:` comments.
We plan to fix issues as they become relevant.
