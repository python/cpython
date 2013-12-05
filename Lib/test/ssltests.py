# Convenience test module to run all of the SSL-related tests in the
# standard library.

import sys
import subprocess

TESTS = ['test_asyncio', 'test_ftplib', 'test_hashlib', 'test_httplib',
         'test_imaplib', 'test_nntplib', 'test_poplib', 'test_smtplib',
         'test_smtpnet', 'test_urllib2_localnet', 'test_venv']

def run_regrtests(*extra_args):
    args = [sys.executable, "-m", "test"]
    if not extra_args:
        args.append("-unetwork")
    else:
        args.extend(extra_args)
    args.extend(TESTS)
    result = subprocess.call(args)
    sys.exit(result)

if __name__ == '__main__':
    run_regrtests(*sys.argv[1:])
