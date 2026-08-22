import sys


def info(msg, *args):
    print(msg % args, file=sys.stderr)

def fail(msg, *args):
    info(msg, *args)
    sys.exit(2)

# ASCII characters that can appear in a well-formed XML document
# except the characters "$@\^`{}~".
compulsory_chars = (set(b'\t\n\r') | set(range(32, 127))) - set(br'$@\^`{}~')

def check_compatibility(encoding):
    for i in compulsory_chars:
        c = chr(i)
        b = bytes([i])
        try:
            decoded = b.decode(encoding)
        except UnicodeDecodeError:
            info('Cannot decode byte %#04x (%a)', i, c)
            return False
        if decoded != c:
            info('Incompatible encoding: %#04x (%a) -> %a', i, c, decoded)
            return False
    return True

def create_table(encoding):
    mapping = [-1] * 256

    non_bmp = None
    for i in range(0x110000):
        char = chr(i)
        try:
            encoded = char.encode(encoding)
        except UnicodeEncodeError:
            continue
        if i >= 0x10000 and non_bmp is None:
            non_bmp = i
            info('Non-BMP character: %r (U+%04X)', char, i)
        length = len(encoded)
        k = encoded[0]
        v = mapping[k]
        if length == 1:
            if v == -1:
                mapping[k] = i
            elif v < 0:
                fail('Ambiguous mapping for %#04x: '
                    '%r (U+%04X) and %d-byte sequence',
                    k, c, i, -v)
            else:
                info('Ambiguous mapping for %#04x: '
                    '%r (U+%04X) and %r (U+%04X)',
                    k, chr(v), v, char, i)
        else:
            if length > 4:
                fail('Too long encoding for %r (U+%04X): %r',
                    char, i, encoded)
            if v == -1:
                mapping[k] = -length
            elif v != -length:
                if v < 0:
                    fail('Ambiguous mapping for %#04x: '
                        '%d-byte sequence and %d-byte sequence %r',
                        k, -v, length, encoded)
                else:
                    fail('Ambiguous mapping for %#04x: '
                        '%r (U+%04X) and %d-byte sequence %r',
                        k, chr(v), v, length, encoded)
    return mapping

def print_table(mapping):
    print(' '*8 + '_expat_decoding_table=(', end='')
    if mapping[:128] == list(range(128)):
        print('*range(128),')
        start = 128
    else:
        print()
        start = 0
    line = ''
    pos = 0
    i = start
    while True:
        if i % 8 == 0:
            if len(line) > 68:
                print(' '*12 + line[:pos].rstrip())
                line = line[pos:]
            pos = len(line)
        if i == 256:
            break
        v = mapping[i]
        if v >= 0:
            v = hex(v)
        line += f'{v}, '
        i += 1
    print(' '*12 + line.rstrip().rstrip(',') + '),')


encoding = sys.argv[1]

if not check_compatibility(encoding):
    print(' '*8 + '_expat_decoding_table=False,')
    sys.exit(1)

mapping = create_table(encoding)
print_table(mapping)
