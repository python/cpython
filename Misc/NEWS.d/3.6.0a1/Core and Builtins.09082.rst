Issue #26464: Fix str.translate() when string is ASCII and first replacements
removes character, but next replacement uses a non-ASCII character or a
string longer than 1 character. Regression introduced in Python 3.5.0.