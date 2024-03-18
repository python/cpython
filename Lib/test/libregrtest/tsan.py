# Set of tests run by default if --tsan is specified.  The tests below were
# chosen because they use threads and run in a reasonable amount of time.

TSAN_TESTS = [
    'test_capi',
    'test_code',
    'test_enum',
    'test_functools',
    'test_httpservers',
    'test_imaplib',
    'test_importlib',
    'test_io',
    'test_logging',
    'test_queue',
    'test_signal',
    'test_socket',
    'test_sqlite3',
    'test_ssl',
    'test_syslog',
    'test_thread',
    'test_threadedtempfile',
    'test_threading',
    'test_threading_local',
    'test_threadsignals',
    'test_weakref',
]


def setup_tsan_tests(cmdline_args):
    if not cmdline_args:
        cmdline_args[:] = TSAN_TESTS[:]
