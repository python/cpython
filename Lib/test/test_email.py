# Copyright (C) 2001,2002 Python Software Foundation
# email package unit tests

# The specific tests now live in Lib/email/test
from email.test.test_email import suite
from email.test.test_email_renamed import suite as suite2
from test import test_support

def test_main():
    test_support.run_unittest(suite())
    test_support.run_unittest(suite2())

if __name__ == '__main__':
    test_main()
