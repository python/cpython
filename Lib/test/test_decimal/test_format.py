import unittest
import locale
from test.support import run_with_locale, warnings_helper
from . import (C, P, load_tests_for_base_classes,
               setUpModule, tearDownModule)


class FormatTest:
    '''Unit tests for the format function.'''

    def test_formatting(self):
        Decimal = self.decimal.Decimal

        # triples giving a format, a Decimal, and the expected result
        test_values = [
            ('e', '0E-15', '0e-15'),
            ('e', '2.3E-15', '2.3e-15'),
            ('e', '2.30E+2', '2.30e+2'), # preserve significant zeros
            ('e', '2.30000E-15', '2.30000e-15'),
            ('e', '1.23456789123456789e40', '1.23456789123456789e+40'),
            ('e', '1.5', '1.5e+0'),
            ('e', '0.15', '1.5e-1'),
            ('e', '0.015', '1.5e-2'),
            ('e', '0.0000000000015', '1.5e-12'),
            ('e', '15.0', '1.50e+1'),
            ('e', '-15', '-1.5e+1'),
            ('e', '0', '0e+0'),
            ('e', '0E1', '0e+1'),
            ('e', '0.0', '0e-1'),
            ('e', '0.00', '0e-2'),
            ('.6e', '0E-15', '0.000000e-9'),
            ('.6e', '0', '0.000000e+6'),
            ('.6e', '9.999999', '9.999999e+0'),
            ('.6e', '9.9999999', '1.000000e+1'),
            ('.6e', '-1.23e5', '-1.230000e+5'),
            ('.6e', '1.23456789e-3', '1.234568e-3'),
            ('f', '0', '0'),
            ('f', '0.0', '0.0'),
            ('f', '0E-2', '0.00'),
            ('f', '0.00E-8', '0.0000000000'),
            ('f', '0E1', '0'), # loses exponent information
            ('f', '3.2E1', '32'),
            ('f', '3.2E2', '320'),
            ('f', '3.20E2', '320'),
            ('f', '3.200E2', '320.0'),
            ('f', '3.2E-6', '0.0000032'),
            ('.6f', '0E-15', '0.000000'), # all zeros treated equally
            ('.6f', '0E1', '0.000000'),
            ('.6f', '0', '0.000000'),
            ('.0f', '0', '0'), # no decimal point
            ('.0f', '0e-2', '0'),
            ('.0f', '3.14159265', '3'),
            ('.1f', '3.14159265', '3.1'),
            ('.01f', '3.14159265', '3.1'), # leading zero in precision
            ('.4f', '3.14159265', '3.1416'),
            ('.6f', '3.14159265', '3.141593'),
            ('.7f', '3.14159265', '3.1415926'), # round-half-even!
            ('.8f', '3.14159265', '3.14159265'),
            ('.9f', '3.14159265', '3.141592650'),

            ('g', '0', '0'),
            ('g', '0.0', '0.0'),
            ('g', '0E1', '0e+1'),
            ('G', '0E1', '0E+1'),
            ('g', '0E-5', '0.00000'),
            ('g', '0E-6', '0.000000'),
            ('g', '0E-7', '0e-7'),
            ('g', '-0E2', '-0e+2'),
            ('.0g', '3.14159265', '3'),  # 0 sig fig -> 1 sig fig
            ('.0n', '3.14159265', '3'),  # same for 'n'
            ('.1g', '3.14159265', '3'),
            ('.2g', '3.14159265', '3.1'),
            ('.5g', '3.14159265', '3.1416'),
            ('.7g', '3.14159265', '3.141593'),
            ('.8g', '3.14159265', '3.1415926'), # round-half-even!
            ('.9g', '3.14159265', '3.14159265'),
            ('.10g', '3.14159265', '3.14159265'), # don't pad

            ('%', '0E1', '0%'),
            ('%', '0E0', '0%'),
            ('%', '0E-1', '0%'),
            ('%', '0E-2', '0%'),
            ('%', '0E-3', '0.0%'),
            ('%', '0E-4', '0.00%'),

            ('.3%', '0', '0.000%'), # all zeros treated equally
            ('.3%', '0E10', '0.000%'),
            ('.3%', '0E-10', '0.000%'),
            ('.3%', '2.34', '234.000%'),
            ('.3%', '1.234567', '123.457%'),
            ('.0%', '1.23', '123%'),

            ('e', 'NaN', 'NaN'),
            ('f', '-NaN123', '-NaN123'),
            ('+g', 'NaN456', '+NaN456'),
            ('.3e', 'Inf', 'Infinity'),
            ('.16f', '-Inf', '-Infinity'),
            ('.0g', '-sNaN', '-sNaN'),

            ('', '1.00', '1.00'),

            # test alignment and padding
            ('6', '123', '   123'),
            ('<6', '123', '123   '),
            ('>6', '123', '   123'),
            ('^6', '123', ' 123  '),
            ('=+6', '123', '+  123'),
            ('#<10', 'NaN', 'NaN#######'),
            ('#<10', '-4.3', '-4.3######'),
            ('#<+10', '0.0130', '+0.0130###'),
            ('#< 10', '0.0130', ' 0.0130###'),
            ('@>10', '-Inf', '@-Infinity'),
            ('#>5', '-Inf', '-Infinity'),
            ('?^5', '123', '?123?'),
            ('%^6', '123', '%123%%'),
            (' ^6', '-45.6', '-45.6 '),
            ('/=10', '-45.6', '-/////45.6'),
            ('/=+10', '45.6', '+/////45.6'),
            ('/= 10', '45.6', ' /////45.6'),
            ('\x00=10', '-inf', '-\x00Infinity'),
            ('\x00^16', '-inf', '\x00\x00\x00-Infinity\x00\x00\x00\x00'),
            ('\x00>10', '1.2345', '\x00\x00\x00\x001.2345'),
            ('\x00<10', '1.2345', '1.2345\x00\x00\x00\x00'),

            # thousands separator
            (',', '1234567', '1,234,567'),
            (',', '123456', '123,456'),
            (',', '12345', '12,345'),
            (',', '1234', '1,234'),
            (',', '123', '123'),
            (',', '12', '12'),
            (',', '1', '1'),
            (',', '0', '0'),
            (',', '-1234567', '-1,234,567'),
            (',', '-123456', '-123,456'),
            ('7,', '123456', '123,456'),
            ('8,', '123456', ' 123,456'),
            ('08,', '123456', '0,123,456'), # special case: extra 0 needed
            ('+08,', '123456', '+123,456'), # but not if there's a sign
            ('008,', '123456', '0,123,456'), # leading zero in width
            (' 08,', '123456', ' 123,456'),
            ('08,', '-123456', '-123,456'),
            ('+09,', '123456', '+0,123,456'),
            # ... with fractional part...
            ('07,', '1234.56', '1,234.56'),
            ('08,', '1234.56', '1,234.56'),
            ('09,', '1234.56', '01,234.56'),
            ('010,', '1234.56', '001,234.56'),
            ('011,', '1234.56', '0,001,234.56'),
            ('012,', '1234.56', '0,001,234.56'),
            ('08,.1f', '1234.5', '01,234.5'),
            # no thousands separators in fraction part
            (',', '1.23456789', '1.23456789'),
            (',%', '123.456789', '12,345.6789%'),
            (',e', '123456', '1.23456e+5'),
            (',E', '123456', '1.23456E+5'),
            # ... with '_' instead
            ('_', '1234567', '1_234_567'),
            ('07_', '1234.56', '1_234.56'),
            ('_', '1.23456789', '1.23456789'),
            ('_%', '123.456789', '12_345.6789%'),
            # and now for something completely different...
            ('.,', '1.23456789', '1.234,567,89'),
            ('._', '1.23456789', '1.234_567_89'),
            ('.6_f', '12345.23456789', '12345.234_568'),
            (',._%', '123.456789', '12,345.678_9%'),
            (',._e', '123456', '1.234_56e+5'),
            (',.4_e', '123456', '1.234_6e+5'),
            (',.3_e', '123456', '1.235e+5'),
            (',._E', '123456', '1.234_56E+5'),

            # negative zero: default behavior
            ('.1f', '-0', '-0.0'),
            ('.1f', '-.0', '-0.0'),
            ('.1f', '-.01', '-0.0'),

            # negative zero: z option
            ('z.1f', '0.', '0.0'),
            ('z6.1f', '0.', '   0.0'),
            ('z6.1f', '-1.', '  -1.0'),
            ('z.1f', '-0.', '0.0'),
            ('z.1f', '.01', '0.0'),
            ('z.1f', '-.01', '0.0'),
            ('z.2f', '0.', '0.00'),
            ('z.2f', '-0.', '0.00'),
            ('z.2f', '.001', '0.00'),
            ('z.2f', '-.001', '0.00'),

            ('z.1e', '0.', '0.0e+1'),
            ('z.1e', '-0.', '0.0e+1'),
            ('z.1E', '0.', '0.0E+1'),
            ('z.1E', '-0.', '0.0E+1'),

            ('z.2e', '-0.001', '-1.00e-3'),  # tests for mishandled rounding
            ('z.2g', '-0.001', '-0.001'),
            ('z.2%', '-0.001', '-0.10%'),

            ('zf', '-0.0000', '0.0000'),  # non-normalized form is preserved

            ('z.1f', '-00000.000001', '0.0'),
            ('z.1f', '-00000.', '0.0'),
            ('z.1f', '-.0000000000', '0.0'),

            ('z.2f', '-00000.000001', '0.00'),
            ('z.2f', '-00000.', '0.00'),
            ('z.2f', '-.0000000000', '0.00'),

            ('z.1f', '.09', '0.1'),
            ('z.1f', '-.09', '-0.1'),

            (' z.0f', '-0.', ' 0'),
            ('+z.0f', '-0.', '+0'),
            ('-z.0f', '-0.', '0'),
            (' z.0f', '-1.', '-1'),
            ('+z.0f', '-1.', '-1'),
            ('-z.0f', '-1.', '-1'),

            ('z>6.1f', '-0.', 'zz-0.0'),
            ('z>z6.1f', '-0.', 'zzz0.0'),
            ('x>z6.1f', '-0.', 'xxx0.0'),
            ('🖤>z6.1f', '-0.', '🖤🖤🖤0.0'),  # multi-byte fill char
            ('\x00>z6.1f', '-0.', '\x00\x00\x000.0'),  # null fill char

            # issue 114563 ('z' format on F type in cdecimal)
            ('z3,.10F', '-6.24E-323', '0.0000000000'),

            # issue 91060 ('#' format in cdecimal)
            ('#', '0', '0.'),

            # issue 6850
            ('a=-7.0', '0.12345', 'aaaa0.1'),

            # issue 22090
            ('<^+15.20%', 'inf', '<<+Infinity%<<<'),
            ('\x07>,%', 'sNaN1234567', 'sNaN1234567%'),
            ('=10.10%', 'NaN123', '   NaN123%'),
            ]
        for fmt, d, result in test_values:
            self.assertEqual(format(Decimal(d), fmt), result)

        # bytes format argument
        self.assertRaises(TypeError, Decimal(1).__format__, b'-020')

        # precision or fractional part separator should follow after dot
        self.assertRaises(ValueError, format, Decimal(1), '.f')
        self.assertRaises(ValueError, format, Decimal(1), '._6f')

    def test_negative_zero_format_directed_rounding(self):
        with self.decimal.localcontext() as ctx:
            ctx.rounding = P.ROUND_CEILING
            self.assertEqual(format(self.decimal.Decimal('-0.001'), 'z.2f'),
                            '0.00')

    def test_negative_zero_bad_format(self):
        self.assertRaises(ValueError, format, self.decimal.Decimal('1.23'), 'fz')

    def test_n_format(self):
        Decimal = self.decimal.Decimal

        try:
            from locale import CHAR_MAX
        except ImportError:
            self.skipTest('locale.CHAR_MAX not available')

        def make_grouping(lst):
            return ''.join([chr(x) for x in lst]) if self.decimal == C else lst

        def get_fmt(x, override=None, fmt='n'):
            if self.decimal == C:
                return Decimal(x).__format__(fmt, override)
            else:
                return Decimal(x).__format__(fmt, _localeconv=override)

        # Set up some localeconv-like dictionaries
        en_US = {
            'decimal_point' : '.',
            'grouping' : make_grouping([3, 3, 0]),
            'thousands_sep' : ','
            }

        fr_FR = {
            'decimal_point' : ',',
            'grouping' : make_grouping([CHAR_MAX]),
            'thousands_sep' : ''
            }

        ru_RU = {
            'decimal_point' : ',',
            'grouping': make_grouping([3, 3, 0]),
            'thousands_sep' : ' '
            }

        crazy = {
            'decimal_point' : '&',
            'grouping': make_grouping([1, 4, 2, CHAR_MAX]),
            'thousands_sep' : '-'
            }

        dotsep_wide = {
            'decimal_point' : b'\xc2\xbf'.decode('utf-8'),
            'grouping': make_grouping([3, 3, 0]),
            'thousands_sep' : b'\xc2\xb4'.decode('utf-8')
            }

        self.assertEqual(get_fmt(Decimal('12.7'), en_US), '12.7')
        self.assertEqual(get_fmt(Decimal('12.7'), fr_FR), '12,7')
        self.assertEqual(get_fmt(Decimal('12.7'), ru_RU), '12,7')
        self.assertEqual(get_fmt(Decimal('12.7'), crazy), '1-2&7')

        self.assertEqual(get_fmt(123456789, en_US), '123,456,789')
        self.assertEqual(get_fmt(123456789, fr_FR), '123456789')
        self.assertEqual(get_fmt(123456789, ru_RU), '123 456 789')
        self.assertEqual(get_fmt(1234567890123, crazy), '123456-78-9012-3')

        self.assertEqual(get_fmt(123456789, en_US, '.6n'), '1.23457e+8')
        self.assertEqual(get_fmt(123456789, fr_FR, '.6n'), '1,23457e+8')
        self.assertEqual(get_fmt(123456789, ru_RU, '.6n'), '1,23457e+8')
        self.assertEqual(get_fmt(123456789, crazy, '.6n'), '1&23457e+8')

        # zero padding
        self.assertEqual(get_fmt(1234, fr_FR, '03n'), '1234')
        self.assertEqual(get_fmt(1234, fr_FR, '04n'), '1234')
        self.assertEqual(get_fmt(1234, fr_FR, '05n'), '01234')
        self.assertEqual(get_fmt(1234, fr_FR, '06n'), '001234')

        self.assertEqual(get_fmt(12345, en_US, '05n'), '12,345')
        self.assertEqual(get_fmt(12345, en_US, '06n'), '12,345')
        self.assertEqual(get_fmt(12345, en_US, '07n'), '012,345')
        self.assertEqual(get_fmt(12345, en_US, '08n'), '0,012,345')
        self.assertEqual(get_fmt(12345, en_US, '09n'), '0,012,345')
        self.assertEqual(get_fmt(12345, en_US, '010n'), '00,012,345')

        self.assertEqual(get_fmt(123456, crazy, '06n'), '1-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '07n'), '1-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '08n'), '1-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '09n'), '01-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '010n'), '0-01-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '011n'), '0-01-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '012n'), '00-01-2345-6')
        self.assertEqual(get_fmt(123456, crazy, '013n'), '000-01-2345-6')

        # wide char separator and decimal point
        self.assertEqual(get_fmt(Decimal('-1.5'), dotsep_wide, '020n'),
                         '-0\u00b4000\u00b4000\u00b4000\u00b4001\u00bf5')

    def test_deprecated_N_format(self):
        Decimal = self.decimal.Decimal
        h = Decimal('6.62607015e-34')
        if self.decimal == C:
            with self.assertWarns(DeprecationWarning) as cm:
                r = format(h, 'N')
            self.assertEqual(cm.filename, __file__)
            self.assertEqual(r, format(h, 'n').upper())
            with self.assertWarns(DeprecationWarning) as cm:
                r = format(h, '010.3N')
            self.assertEqual(cm.filename, __file__)
            self.assertEqual(r, format(h, '010.3n').upper())
        else:
            self.assertRaises(ValueError, format, h, 'N')
            self.assertRaises(ValueError, format, h, '010.3N')
        with warnings_helper.check_no_warnings(self):
            self.assertEqual(format(h, 'N>10.3'), 'NN6.63E-34')
            self.assertEqual(format(h, 'N>10.3n'), 'NN6.63e-34')
            self.assertEqual(format(h, 'N>10.3e'), 'N6.626e-34')
            self.assertEqual(format(h, 'N>10.3f'), 'NNNNN0.000')
            self.assertRaises(ValueError, format, h, '>Nf')
            self.assertRaises(ValueError, format, h, '10Nf')
            self.assertRaises(ValueError, format, h, 'Nx')

    @run_with_locale('LC_ALL', 'ps_AF', '')
    def test_wide_char_separator_decimal_point(self):
        # locale with wide char separator and decimal point
        Decimal = self.decimal.Decimal

        decimal_point = locale.localeconv()['decimal_point']
        thousands_sep = locale.localeconv()['thousands_sep']
        if decimal_point != '\u066b':
            self.skipTest('inappropriate decimal point separator '
                          '({!a} not {!a})'.format(decimal_point, '\u066b'))
        if thousands_sep != '\u066c':
            self.skipTest('inappropriate thousands separator '
                          '({!a} not {!a})'.format(thousands_sep, '\u066c'))

        self.assertEqual(format(Decimal('100000000.123'), 'n'),
                         '100\u066c000\u066c000\u066b123')

    def test_decimal_from_float_argument_type(self):
        class A(self.decimal.Decimal):
            def __init__(self, a):
                self.a_type = type(a)
        a = A.from_float(42.5)
        self.assertEqual(self.decimal.Decimal, a.a_type)

        a = A.from_float(42)
        self.assertEqual(self.decimal.Decimal, a.a_type)


def load_tests(loader, tests, pattern):
    return load_tests_for_base_classes(loader, tests, [FormatTest])


if __name__ == '__main__':
    unittest.main()
