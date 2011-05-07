#!/usr/bin/env python

import unittest
from test import support
import smtplib

support.requires("network")

class SmtpSSLTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 465

    def test_connect(self):
        support.get_attribute(smtplib, 'SMTP_SSL')
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP_SSL(self.testServer, self.remotePort)
        server.ehlo()
        server.quit()

    def test_connect_default_port(self):
        support.get_attribute(smtplib, 'SMTP_SSL')
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP_SSL(self.testServer)
        server.ehlo()
        server.quit()

def test_main():
    support.run_unittest(SmtpSSLTest)

if __name__ == "__main__":
    test_main()
