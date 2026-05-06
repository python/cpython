lazy import re as _re


def is_valid_name(name):
    """Test whether a string is a valid element or attribute name."""
    # https://www.w3.org/TR/xml/#NT-Name
    return _re.fullmatch(
        # NameStartChar
        '['
            ':A-Z_a-z'
            '\xC0-\xD6\xD8-\xF6\xF8-\u02FF\u0370-\u037D\u037F-\u1FFF'
            '\u200C\u200D'
            '\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF'
            '\uF900-\uFDCF\uFDF0-\uFFFD\U00010000-\U000EFFFF'
        ']'
        # NameChar
        '['
            r'\-.0-9:A-Z_a-z'
            '\xB7'
            '\xC0-\xD6\xD8-\xF6\xF8-\u037D\u037F-\u1FFF'
            '\u200C\u200D\u203F\u2040'
            '\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF'
            '\uF900-\uFDCF\uFDF0-\uFFFD\U00010000-\U000EFFFF'
        ']*+',
        name) is not None
