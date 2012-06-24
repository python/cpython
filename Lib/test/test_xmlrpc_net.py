#!/usr/bin/env python3

import collections
import errno
import socket
import sys
import unittest
from test import support

import xmlrpc.client as xmlrpclib

class CurrentTimeTest(unittest.TestCase):

    def test_current_time(self):
        # Get the current time from xmlrpc.com.  This code exercises
        # the minimal HTTP functionality in xmlrpclib.
        self.skipTest("time.xmlrpc.com is unreliable")
        server = xmlrpclib.ServerProxy("http://time.xmlrpc.com/RPC2")
        try:
            t0 = server.currentTime.getCurrentTime()
        except socket.error as e:
            self.skipTest("network error: %s" % e)
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
        self.assertTrue(delta.days <= 1)

    def test_python_builders(self):
        # Get the list of builders from the XMLRPC buildbot interface at
        # python.org.
        server = xmlrpclib.ServerProxy("http://buildbot.python.org/all/xmlrpc/")
        try:
            builders = server.getAllBuilders()
        except socket.error as e:
            self.skipTest("network error: %s" % e)
            return
        self.addCleanup(lambda: server('close')())

        # Perform a minimal sanity check on the result, just to be sure
        # the request means what we think it means.
        self.assertIsInstance(builders, collections.Sequence)
        self.assertTrue([x for x in builders if "3.x" in x], builders)


def test_main():
    support.requires("network")
    support.run_unittest(CurrentTimeTest)

if __name__ == "__main__":
    test_main()
