# Copyright (C) 2001,2002 Python Software Foundation
# email package unit tests

import unittest
# The specific tests now live in Lib/email/test
from email.test.test_email import suite
from test.test_support import run_suite

def test_main():
    run_suite(suite())

if __name__ == '__main__':
    test_main()
