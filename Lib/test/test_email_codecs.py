# Copyright (C) 2002 Python Software Foundation
# email package unit tests for (optional) Asian codecs

# The specific tests now live in Lib/email/test
from email.test import test_email_codecs
from test import test_support

def test_main():
    test_support.run_suite(test_email_codecs.suite())

if __name__ == '__main__':
    test_main()
