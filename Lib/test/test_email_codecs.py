# Copyright (C) 2002 Python Software Foundation
# email package unit tests for (optional) Asian codecs

import unittest
# The specific tests now live in Lib/email/test
from email.test.test_email_codecs import suite



if __name__ == '__main__':
    unittest.main(defaultTest='suite')
