"""Make the custom certificate and private key files used by test_ssl
and friends."""

import os
import sys
import tempfile
from subprocess import *

req_template = """
    [req]
    distinguished_name     = req_distinguished_name
    x509_extensions        = req_x509_extensions
    prompt                 = no

    [req_distinguished_name]
    C                      = XY
    L                      = Castle Anthrax
    O                      = Python Software Foundation
    CN                     = {hostname}

    [req_x509_extensions]
    subjectAltName         = DNS:{hostname}
    """

here = os.path.abspath(os.path.dirname(__file__))

def make_cert_key(hostname):
    tempnames = []
    for i in range(3):
        with tempfile.NamedTemporaryFile(delete=False) as f:
            tempnames.append(f.name)
    req_file, cert_file, key_file = tempnames
    try:
        with open(req_file, 'w') as f:
            f.write(req_template.format(hostname=hostname))
        args = ['req', '-new', '-days', '3650', '-nodes', '-x509',
                '-newkey', 'rsa:1024', '-keyout', key_file,
                '-out', cert_file, '-config', req_file]
        check_call(['openssl'] + args)
        with open(cert_file, 'r') as f:
            cert = f.read()
        with open(key_file, 'r') as f:
            key = f.read()
        return cert, key
    finally:
        for name in tempnames:
            os.remove(name)


if __name__ == '__main__':
    os.chdir(here)
    cert, key = make_cert_key('localhost')
    with open('ssl_cert.pem', 'w') as f:
        f.write(cert)
    with open('ssl_key.pem', 'w') as f:
        f.write(key)
    with open('keycert.pem', 'w') as f:
        f.write(key)
        f.write(cert)
    # For certificate matching tests
    cert, key = make_cert_key('fakehostname')
    with open('keycert2.pem', 'w') as f:
        f.write(key)
        f.write(cert)
