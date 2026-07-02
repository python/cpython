"""Pure functions for vi motion operations.

These functions implement vi-style cursor motions without depending on Reader state.
"""

def _is_vi_word_char(c: str) -> bool:
    return c.isalnum() or c == '_'


def vi_eow(buffer: list[str], pos: int) -> int:
    """Return the 0-based index of the last character of the word
    following pos most immediately (vi 'e' semantics).

    Vi has three character classes: word chars (alnum + _), punctuation
    (non-word, non-whitespace), and whitespace. 'e' moves to the end
    of the current or next word/punctuation sequence."""
    b = buffer
    p = pos

    if not b:
        return 0

    # Helper to check if at end of current sequence
    def at_sequence_end(pos: int) -> bool:
        if pos >= len(b) - 1:
            return True
        curr_is_word = _is_vi_word_char(b[pos])
        next_is_word = _is_vi_word_char(b[pos + 1])
        curr_is_space = b[pos].isspace()
        next_is_space = b[pos + 1].isspace()
        if curr_is_word:
            return not next_is_word
        elif not curr_is_space:
            # Punctuation - at end if next is word or whitespace
            return next_is_word or next_is_space
        return True

    # If already at end of a word/punctuation, move forward
    if p < len(b) and at_sequence_end(p):
        p += 1

    # Skip whitespace
    while p < len(b) and b[p].isspace():
        p += 1

    if p >= len(b):
        return len(b) - 1

    # Move to end of current word or punctuation sequence
    if _is_vi_word_char(b[p]):
        while p + 1 < len(b) and _is_vi_word_char(b[p + 1]):
            p += 1
    else:
        # Punctuation sequence
        while p + 1 < len(b) and not _is_vi_word_char(b[p + 1]) and not b[p + 1].isspace():
            p += 1

    return min(p, len(b) - 1)


def vi_forward_word(buffer: list[str], pos: int) -> int:
    """Return the 0-based index of the first character of the next word
    (vi 'w' semantics).

    Vi has three character classes: word chars (alnum + _), punctuation
    (non-word, non-whitespace), and whitespace. 'w' moves to the start
    of the next word or punctuation sequence."""
    b = buffer
    p = pos

    if not b or p >= len(b):
        return max(0, len(b) - 1) if b else 0

    # Skip current word or punctuation sequence
    if _is_vi_word_char(b[p]):
        # On a word char - skip word chars
        while p < len(b) and _is_vi_word_char(b[p]):
            p += 1
    elif not b[p].isspace():
        # On punctuation - skip punctuation
        while p < len(b) and not _is_vi_word_char(b[p]) and not b[p].isspace():
            p += 1

    # Skip whitespace to find next word or punctuation
    while p < len(b) and b[p].isspace():
        p += 1

    # Clamp to valid buffer range
    return min(p, len(b) - 1) if b else 0


def vi_forward_word_ws(buffer: list[str], pos: int) -> int:
    """Return the 0-based index of the first character of the next WORD
    (vi 'W' semantics).

    Treats white space as the only separator."""
    b = buffer
    p = pos

    if not b or p >= len(b):
        return max(0, len(b) - 1) if b else 0

    # Skip all non-whitespace (the current WORD)
    while p < len(b) and not b[p].isspace():
        p += 1

    # Skip whitespace to find next WORD
    while p < len(b) and b[p].isspace():
        p += 1

    # Clamp to valid buffer range
    return min(p, len(b) - 1) if b else 0


def vi_bow(buffer: list[str], pos: int) -> int:
    """Return the 0-based index of the beginning of the word preceding pos
    (vi 'b' semantics).

    Vi has three character classes: word chars (alnum + _), punctuation
    (non-word, non-whitespace), and whitespace. 'b' moves to the start
    of the current or previous word/punctuation sequence."""
    b = buffer
    p = pos

    if not b or p <= 0:
        return 0

    p -= 1

    # Skip whitespace going backward
    while p >= 0 and b[p].isspace():
        p -= 1

    if p < 0:
        return 0

    # Now skip the word or punctuation sequence we landed in
    if _is_vi_word_char(b[p]):
        while p > 0 and _is_vi_word_char(b[p - 1]):
            p -= 1
    else:
        # Punctuation sequence
        while p > 0 and not _is_vi_word_char(b[p - 1]) and not b[p - 1].isspace():
            p -= 1

    return p


def vi_bow_ws(buffer: list[str], pos: int) -> int:
    """Return the 0-based index of the beginning of the WORD preceding pos
    (vi 'B' semantics).

    Treats white space as the only separator."""
    b = buffer
    p = pos

    if not b or p <= 0:
        return 0

    p -= 1

    # Skip whitespace going backward
    while p >= 0 and b[p].isspace():
        p -= 1

    if p < 0:
        return 0

    # Now skip the WORD we landed in
    while p > 0 and not b[p - 1].isspace():
        p -= 1

    return p


def find_char_forward(buffer: list[str], pos: int, char: str) -> int | None:
    """Find next occurrence of char after pos. Returns index or None."""
    for i in range(pos + 1, len(buffer)):
        if buffer[i] == char:
            return i
    return None


def find_char_backward(buffer: list[str], pos: int, char: str) -> int | None:
    """Find previous occurrence of char before pos. Returns index or None."""
    for i in range(pos - 1, -1, -1):
        if buffer[i] == char:
            return i
    return None
