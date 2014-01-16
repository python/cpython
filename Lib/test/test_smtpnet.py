import unittest
from test import support
import smtplib
import socket

ssl = support.import_module("ssl")

support.requires("network")

def check_ssl_verifiy(host, port):
    context = ssl.create_default_context()
    with socket.create_connection((host, port)) as sock:
        try:
            sock = context.wrap_socket(sock, server_hostname=host)
        except Exception:
            return False
        else:
            sock.close()
            return True


class SmtpTest(unittest.TestCase):
    testServer = 'smtp.gmail.com'
    remotePort = 25

    def test_connect_starttls(self):
        support.get_attribute(smtplib, 'SMTP_SSL')
        context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP(self.testServer, self.remotePort)
            try:
                server.starttls(context=context)
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
    can_verify = check_ssl_verifiy(testServer, remotePort)

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
        context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        support.get_attribute(smtplib, 'SMTP_SSL')
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP_SSL(self.testServer, self.remotePort, context=context)
            server.ehlo()
            server.quit()

    @unittest.skipUnless(can_verify, "SSL certificate can't be verified")
    def test_connect_using_sslcontext_verified(self):
        support.get_attribute(smtplib, 'SMTP_SSL')
        context = ssl.create_default_context()
        with support.transient_internet(self.testServer):
            server = smtplib.SMTP_SSL(self.testServer, self.remotePort, context=context)
            server.ehlo()
            server.quit()


def test_main():
    support.run_unittest(SmtpTest, SmtpSSLTest)

if __name__ == "__main__":
    test_main()
