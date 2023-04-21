#!/usr/bin/env python3
#
# fetch the certificate that the server(s) are providing in PEM form
#
# args are HOST:PORT [, HOST:PORT...]
#
# By Bill Janssen.

import re
import os
import sys
import tempfile


def fetch_server_certificate (host, port):

    def subproc(cmd):
        from subprocess import Popen, PIPE, STDOUT, DEVNULL
        proc = Popen(cmd, stdout=PIPE, stderr=STDOUT, stdin=DEVNULL)
        status = proc.wait()
        output = proc.stdout.read()
        return status, output

    def strip_to_x509_cert(certfile_contents, outfile=None):
        m = re.search(br"^([-]+BEGIN CERTIFICATE[-]+[\r]*\n"
                      br".*[\r]*^[-]+END CERTIFICATE[-]+)$",
                      certfile_contents, re.MULTILINE | re.DOTALL)
        if not m:
            return None
        else:
            tn = tempfile.mktemp()
            with open(tn, "wb") as fp:
                fp.write(m.group(1) + b"\n")
            try:
                tn2 = (outfile or tempfile.mktemp())
                cmd = ['openssl', 'x509', '-in', tn, '-out', tn2]
                status, output = subproc(cmd)
                if status != 0:
                    raise RuntimeError('OpenSSL x509 failed with status %s and '
                                       'output: %r' % (status, output))
                with open(tn2, 'rb') as fp:
                    data = fp.read()
                os.unlink(tn2)
                return data
            finally:
                os.unlink(tn)

    cmd = ['openssl', 's_client', '-connect', '%s:%s' % (host, port), '-showcerts']
    status, output = subproc(cmd)

    if status != 0:
        raise RuntimeError('OpenSSL connect failed with status %s and '
                           'output: %r' % (status, output))
    certtext = strip_to_x509_cert(output)
    if not certtext:
        raise ValueError("Invalid response received from server at %s:%s" %
                         (host, port))
    return certtext


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.stderr.write(
            "Usage:  %s HOSTNAME:PORTNUMBER [, HOSTNAME:PORTNUMBER...]\n" %
            sys.argv[0])
        sys.exit(1)
    for arg in sys.argv[1:]:
        host, port = arg.split(":")
        sys.stdout.buffer.write(fetch_server_certificate(host, int(port)))
    sys.exit(0)
