"""Export the list of Python keywords (reserved words)."""

# grep '{1, "' ../Python/graminit.c | sed 's/.*"\(.*\)".*/    "\1",/' | sort

keywords = [
    "__assert__",
    "and",
    "break",
    "class",
    "continue",
    "def",
    "del",
    "elif",
    "else",
    "except",
    "exec",
    "finally",
    "for",
    "from",
    "global",
    "if",
    "import",
    "in",
    "is",
    "lambda",
    "not",
    "or",
    "pass",
    "print",
    "raise",
    "return",
    "try",
    "while",
    ]

if __name__ == '__main__':
    for k in keywords: print k
