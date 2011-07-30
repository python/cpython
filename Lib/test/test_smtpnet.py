#!/usr/bin/env python3

import unittest
from test import support
import smtplib

ssl = support.import_module("ssl")

support.requires("network")


class SmtpTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 25
    context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)

    def test_connect_starttls(self):
        support.get_attribute(smtplib, 'SMTP_SSL')
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP(self.testServer, self.remotePort)
            try:
                server.starttls(context=self.context)
            except smtplib.SMTPException as e:
                if e.args[0] == 'STARTTLS extension not supported by server.':
                    unittest.skip(e.args[0])
                else:
                    raise
            server.ehlo()
            server.quit()


class SmtpSSLTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 465
    context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)

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

    def test_connect_using_sslcontext(self):
        support.get_attribute(smtplib, 'SMTP_SSL')
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP_SSL(self.testServer, self.remotePort, context=self.context)
            server.ehlo()
            server.quit()


def test_main():
    support.run_unittest(SmtpTest, SmtpSSLTest)

if __name__ == "__main__":
    test_main()
