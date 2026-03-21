Added cross-language method suggestions to :exc:`AttributeError` messages.
When an attribute name from another programming language is used on a
builtin type (e.g., ``list.push`` from JavaScript, ``str.toUpperCase`` from
Java), Python now suggests the equivalent Python method (e.g., ``append``,
``upper``). This runs as a fallback after the existing Levenshtein-based
suggestion matching.
