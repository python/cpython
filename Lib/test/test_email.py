# Copyright (C) 2001-2007 Python Software Foundation
# email package unit tests

# The specific tests now live in Lib/email/test
from email.test.test_email import suite
from test import test_support

def test_main():
    test_support.run_unittest(suite())

if __name__ == '__main__':
    test_main()
