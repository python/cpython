from . import preprocessor


def iter_clean_lines(lines):
    incomment = False
    for line in lines:
        # Deal with comments.
        if incomment:
            _, sep, line = line.partition('*/')
            if sep:
                incomment = False
            continue
        line, _, _ = line.partition('//')
        line, sep, remainder = line.partition('/*')
        if sep:
            _, sep, after = remainder.partition('*/')
            if not sep:
                incomment = True
                continue
            line += ' ' + after

        # Ignore blank lines and leading/trailing whitespace.
        line = line.strip()
        if not line:
            continue

        yield line


def iter_lines(filename, *,
               preprocess=preprocessor.run,
               ):
    content = preprocess(filename)
    return iter(content.splitlines())
