#!/usr/bin/env python

import unittest
from test import support
import smtplib

support.requires(
    "network",
    "use of network resource is not enabled and "
    "test requires Internet access for communication with smtp.gmail.com:465",
    )

class SmtpSSLTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 465

    def test_connect(self):
        server = smtplib.SMTP_SSL(self.testServer, self.remotePort)
        server.ehlo()
        server.quit()

def test_main():
    support.run_unittest(SmtpSSLTest)

if __name__ == "__main__":
    test_main()
