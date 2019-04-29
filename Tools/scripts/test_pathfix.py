import pytest, sys
from pathfix import fixline, parse_shebang

test_cases = [
    (b'#!\\usr\\bin\\python'       , b'#! \\usr\\bin\\python -f'),
    (b'#!python -f'                , b'#! \\usr\\bin\\python -f'),
    (b'#!\\usr\\bin\\python -s'    , b'#! \\usr\\bin\\python -fs'),
    (b'#!\\usr\\bin\\python -f sfj', b'#! \\usr\\bin\\python -f sfj'),
    (b'#!\\usr\\python -s sfj'     , b'#! \\usr\\bin\\python -fs sfj'),
]

test_cases_for_parsing = [
    (b'#!\\usr\\bin\\python'             , (b'',b'')),
    (b'#!\\usr\\bin\\python -f'          , (b'f',b'')),
    (b'#!\\usr\\bin\\python -f sfj'      , (b'f',b'sfj')),
    (b'#!\\usr\\bin\\python -f sfj af bg', (b'f', b'sfj af bg')),
]

@pytest.mark.parametrize("line,output", test_cases_for_parsing)
def test_parse_shebangs(line, output):
    assert parse_shebang(line) == output

# I don't know how to fix these to test properly

@pytest.mark.parametrize("line,output", test_cases)
def test_fixline(line, output):
    this = sys.modules['pathfix']
    this.add_flag = b'f'
    this.new_interpreter = b'\\usr\\bin\\python'
    testing_output = fixline(line)
    assert testing_output == output

