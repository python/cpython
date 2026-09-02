#!/usr/bin/env python3
#
# Compute tables for longobject.c long_from_non_binary_base().  They are used
# for conversions of strings to integers with a non-binary base.

import math
import textwrap


def format_array(name, values):
    values = [str(v) for v in values]
    values = ', '.join(values)
    result = f'{name} = {{{values}}};'
    result = textwrap.wrap(
        result,
        initial_indent=' ' * 4,
        subsequent_indent=' ' * 8,
    )
    return '\n'.join(result)


def conv_tables(long_bits):
    PyLong_BASE = 1 << long_bits
    log_base_BASE = [0.0] * 37
    convmultmax_base = [0] * 37
    convwidth_base = [0] * 37
    for base in range(2, 37):
        is_binary_base = (base & (base - 1)) == 0
        if is_binary_base:
            continue  # don't need, leave as zero
        convmax = base
        i = 1
        log_base_BASE[base] = math.log(base) / math.log(PyLong_BASE)
        while True:
            next = convmax * base
            if next > PyLong_BASE:
                break
            convmax = next
            i += 1
        convmultmax_base[base] = convmax
        assert i > 0
        convwidth_base[base] = i
    return '\n'.join(
        [
            format_array(
                'static const double log_base_BASE[37]', log_base_BASE
            ),
            format_array(
                'static const int convwidth_base[37]', convwidth_base
            ),
            format_array(
                'static const twodigits convmultmax_base[37]',
                convmultmax_base,
            ),
        ]
    )


def main():
    print(
        f'''\
// Tables are computed by Tools/scripts/long_conv_tables.py
#if PYLONG_BITS_IN_DIGIT == 15
{conv_tables(15)}
#elif PYLONG_BITS_IN_DIGIT == 30
{conv_tables(30)}
#else
    #error "invalid PYLONG_BITS_IN_DIGIT value"
#endif
'''
    )


if __name__ == '__main__':
    main()
