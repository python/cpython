#!/usr/bin/env python3
#
# Compute tables for longobject.c integer conversion.

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
    steps = 0
    PyLong_BASE = 1 << long_bits
    log_base_BASE = [0.0] * 37
    convmultmax_base = [0] * 37
    convwidth_base = [0] * 37
    for base in range(2, 37):
        convmax = base
        i = 1
        log_base_BASE[base] = math.log(float(base)) / math.log(PyLong_BASE)
        while True:
            steps += 1
            next = convmax * base
            if next > PyLong_BASE:
                break
            convmax = next
            i += 1
        convmultmax_base[base] = convmax
        assert i > 0
        convwidth_base[base] = i
    # print('steps', steps)
    return '\n'.join(
        [
            format_array('static double log_base_BASE[37]', log_base_BASE),
            format_array('static int convmultmax_base[37]', convmultmax_base),
            format_array(
                'static twodigits convwidth_base[37]', convwidth_base
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
