#!/usr/bin/env python

import unittest
from test import test_support
import smtplib

test_support.requires("network")

class SmtpSSLTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 465

    def test_connect(self):
        #Silently skip test if no SSL; in 2.7 we use SkipTest instead.
        if not hasattr(smtplib, 'SMTP_SSL'): return
        server = smtplib.SMTP_SSL(self.testServer, self.remotePort)
        server.ehlo()
        server.quit()

def test_main():
    test_support.run_unittest(SmtpSSLTest)

if __name__ == "__main__":
    test_main()
