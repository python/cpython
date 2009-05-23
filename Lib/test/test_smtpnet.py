#!/usr/bin/env python

import unittest
from test import support
import smtplib

support.requires("network")

class SmtpSSLTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 465

    def test_connect(self):
        #Skip test silently if no SSL; in 3.1 we use SkipTest
        if not hasattr(smtplib, 'SMTP_SSL'): return
        server = smtplib.SMTP_SSL(self.testServer, self.remotePort)
        server.ehlo()
        server.quit()

def test_main():
    support.run_unittest(SmtpSSLTest)

if __name__ == "__main__":
    test_main()
