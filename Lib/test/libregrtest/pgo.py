# Set of tests run by default if --pgo is specified.  The tests below were
# chosen based on the following criteria: either they exercise a commonly used
# C extension module or type, or they run some relatively typical Python code.
# Long running tests should be avoided because the PGO instrumented executable
# runs slowly.
PGO_TESTS = [
    'test_array',
    'test_base64',
    'test_binascii',
    'test_binop',
    'test_bisect',
    'test_bytes',
    'test_bz2',
    'test_cmath',
    'test_codecs',
    'test_collections',
    'test_complex',
    'test_dataclasses',
    'test_datetime',
    'test_decimal',
    'test_difflib',
    'test_embed',
    'test_float',
    'test_fstring',
    'test_functools',
    'test_generators',
    'test_hashlib',
    'test_heapq',
    'test_int',
    'test_itertools',
    'test_json',
    'test_long',
    'test_lzma',
    'test_math',
    'test_memoryview',
    'test_operator',
    'test_ordered_dict',
    'test_patma',
    'test_pickle',
    'test_pprint',
    'test_re',
    'test_set',
    'test_sqlite3',
    'test_statistics',
    'test_str',
    'test_struct',
    'test_tabnanny',
    'test_time',
    'test_xml_etree',
    'test_xml_etree_c',
]

def setup_pgo_tests(cmdline_args, pgo_extended: bool):
    if not cmdline_args and not pgo_extended:
        # run default set of tests for PGO training
        cmdline_args[:] = PGO_TESTS[:]
