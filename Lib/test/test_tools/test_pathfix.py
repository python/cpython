import pytest, sys
from pathfix import fixline, parse_shebang

test_cases = [
    (b'#!/usr/bin/python'       , b'#! /usr/bin/python -f\n'),
    (b'#!python -f'             , b'#! /usr/bin/python -f\n'),
    (b'#! /usr/bin/python -s'    , b'#! /usr/bin/python -fs\n'),
    (b'#!/usr/bin/python -f sfj', b'#! /usr/bin/python -f sfj\n'),
    (b'#!/usr/python -s sfj'    , b'#! /usr/bin/python -fs sfj\n'),
]

test_cases_for_parsing = [
    (b'#!/usr/bin/python'             , (b'',b'')),
    (b'#! /usr/bin/python -f'          , (b'f',b'')),
    (b'#!/usr/bin/python -f sfj'      , (b'f',b'sfj')),
    (b'#!/usr/bin/python -f sfj af bg', (b'f', b'sfj af bg')),
]

test_cases_without_f_option = [
    b'#!/usr/bin/python',
    b'#!python -f',
    b'#! /usr/bin/python -s',
    b'#!/usr/bin/python -f sfj',
    b'#!/usr/python -s sfj',
]


@pytest.mark.parametrize("line,output", test_cases_for_parsing)
def test_parse_shebangs(line, output):
    assert parse_shebang(line) == output


@pytest.mark.parametrize("line,output", test_cases)
def test_fixline(line, output):
    this = sys.modules['pathfix']
    this.add_flag = b'f'
    this.new_interpreter = b'/usr/bin/python'
    testing_output = fixline(line)
    assert testing_output == output


@pytest.mark.parametrize("line", test_cases_without_f_option)
def test_fixline_without_f_option(line):
    this = sys.modules['pathfix']
    this.new_interpreter = b'/usr/bin/python'
    this.add_flag = None
    testing_output = fixline(line)
    assert testing_output == b'#! ' + this.new_interpreter + b'\n'
