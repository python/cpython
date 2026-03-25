Cross-language method suggestions are now shown for :exc:`AttributeError` on
builtin types when the existing Levenshtein-based suggestions find no match.
For example, ``[].push()`` now suggests ``append`` (JavaScript), and
``"".toUpperCase()`` suggests ``upper``. The ``list.add()`` case suggests
using a set instead, following feedback from the community discussion.
