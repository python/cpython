import itertools


def generate_tokens(tokens):
    numbers = itertools.count(0)
    for line in tokens:
        line = line.strip()

        if not line:
            continue
        if line.strip().startswith('#'):
            continue

        name = line.split()[0]
        yield (name, next(numbers))

    yield ('N_TOKENS', next(numbers))
    yield ('NT_OFFSET', 256)


def generate_opmap(tokens):
    for line in tokens:
        line = line.strip()

        if not line:
            continue
        if line.strip().startswith('#'):
            continue

        pieces = line.split()

        if len(pieces) != 2:
            continue

        name, op = pieces
        yield (op.strip("'"), name)

    # Yield independently <>. This is needed so it does not collide
    # with the token generation in "generate_tokens" because if this
    # symbol is included in Grammar/Tokens, it will collide with !=
    # as it has the same name (NOTEQUAL).
    yield ('<>', 'NOTEQUAL')
