# Copyright (C) 2001-2007 Python Software Foundation
# email package unit tests

# The specific tests now live in Lib/email/test
from email.test.test_email import suite
from email.test.test_email_codecs import suite as codecs_suite
from test import support

def test_main():
    support.run_unittest(suite())
    support.run_unittest(codecs_suite())

if __name__ == '__main__':
    test_main()
