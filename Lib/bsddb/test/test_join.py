"""
TestCases for using the DB.join and DBCursor.join_item methods.
"""

import sys, os, string
import tempfile
from pprint import pprint
import unittest

from bsddb import db

from test.test_support import verbose
