#!/usr/bin/env python

import errno
import socket
import sys
import unittest
from test import test_support

import xmlrpclib

class CurrentTimeTest(unittest.TestCase):

    def test_current_time(self):
        # Get the current time from xmlrpc.com.  This code exercises
        # the minimal HTTP functionality in xmlrpclib.
        server = xmlrpclib.ServerProxy("http://time.xmlrpc.com/RPC2")
        try:
            t0 = server.currentTime.getCurrentTime()
        except socket.error as e:
            print("    test_current_time: skipping test, got error: %s" % e,
                  file=sys.stderr)
            return

        # Perform a minimal sanity check on the result, just to be sure
        # the request means what we think it means.
        t1 = xmlrpclib.DateTime()

        dt0 = xmlrpclib._datetime_type(t0.value)
        dt1 = xmlrpclib._datetime_type(t1.value)
        if dt0 > dt1:
            delta = dt0 - dt1
        else:
            delta = dt1 - dt0
        # The difference between the system time here and the system
        # time on the server should not be too big.
        self.assert_(delta.days <= 1)


def test_main():
    test_support.requires("network")
    test_support.run_unittest(CurrentTimeTest)

if __name__ == "__main__":
    test_main()
