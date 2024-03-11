# Set of tests run by default if --tsan is specified.  The tests below were
# chosen to run a thread sanitizer in a reasonable executable time.

TSAN_TESTS = [
    'test_asyncio',
    'test_capi',
    'test_code',
    'test_compilerall',
    'test_concurrent_futures',
    'test_enum',
    'test_fork1',
    'test_functools',
    'test_httpservers',
    'test_imaplib',
    'test_import',
    'test_importlib',
    'test_io',
    'test_logging',
    'test_multiprocessing_forkserver',
    'test_multiprocessing_spawn',
    'test_pickle',
    'test_queue',
    'test_list',
    'test_dict',
    'test_set',
    'test_tuple',
    'test_smtpnet',
    'test_socketserver',
    'test_ssl',
    'test_syslog',
    'test_thread',
    'test_threadedtempfile',
    'test_threading',
    'test_threading_local',
    'test_threadsignals',
    'test_urllib2net',
    'test_weakref'
]


def setup_tsan_tests(cmdline_args):
    if not cmdline_args:
        cmdline_args[:] = TSAN_TESTS[:]
