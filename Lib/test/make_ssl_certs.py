"""Make the custom certificate and private key files used by test_ssl
and friends."""

import os
import shutil
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
    subjectAltName         = @san

    [san]
    DNS.1 = {hostname}
    {extra_san}

    [dir_sect]
    C                      = XY
    L                      = Castle Anthrax
    O                      = Python Software Foundation
    CN                     = dirname example

    [princ_name]
    realm = EXP:0, GeneralString:KERBEROS.REALM
    principal_name = EXP:1, SEQUENCE:principal_seq

    [principal_seq]
    name_type = EXP:0, INTEGER:1
    name_string = EXP:1, SEQUENCE:principals

    [principals]
    princ1 = GeneralString:username

    [ ca ]
    default_ca      = CA_default

    [ CA_default ]
    dir = cadir
    database  = $dir/index.txt
    crlnumber = $dir/crl.txt
    default_md = sha256
    default_days = 3600
    default_crl_days = 3600
    certificate = pycacert.pem
    private_key = pycakey.pem
    serial    = $dir/serial
    RANDFILE  = $dir/.rand

    policy          = policy_match

    [ policy_match ]
    countryName             = match
    stateOrProvinceName     = optional
    organizationName        = match
    organizationalUnitName  = optional
    commonName              = supplied
    emailAddress            = optional

    [ policy_anything ]
    countryName   = optional
    stateOrProvinceName = optional
    localityName    = optional
    organizationName  = optional
    organizationalUnitName  = optional
    commonName    = supplied
    emailAddress    = optional


    [ v3_ca ]

    subjectKeyIdentifier=hash
    authorityKeyIdentifier=keyid:always,issuer
    basicConstraints = CA:true

    """

here = os.path.abspath(os.path.dirname(__file__))


def make_cert_key(hostname, sign=False, extra_san='',
                  ext='req_x509_extensions_full', key='rsa:3072'):
    print("creating cert for " + hostname)
    tempnames = []
    for i in range(3):
        with tempfile.NamedTemporaryFile(delete=False) as f:
            tempnames.append(f.name)
    req_file, cert_file, key_file = tempnames
    try:
        req = req_template.format(hostname=hostname, extra_san=extra_san)
        with open(req_file, 'w') as f:
            f.write(req)
        args = ['req', '-new', '-days', '3650', '-nodes',
                '-newkey', 'rsa:1024', '-keyout', key_file,
                '-config', req_file]
        if sign:
            with tempfile.NamedTemporaryFile(delete=False) as f:
                tempnames.append(f.name)
                reqfile = f.name
            args += ['-out', reqfile ]

        else:
            args += ['-x509', '-out', cert_file ]
        check_call(['openssl'] + args)

        if sign:
            args = ['ca', '-config', req_file, '-out', cert_file, '-outdir', 'cadir',
                    '-policy', 'policy_anything', '-batch', '-infiles', reqfile ]
            check_call(['openssl'] + args)


        with open(cert_file, 'r') as f:
            cert = f.read()
        with open(key_file, 'r') as f:
            key = f.read()
        return cert, key
    finally:
        for name in tempnames:
            os.remove(name)

TMP_CADIR = 'cadir'

def unmake_ca():
    shutil.rmtree(TMP_CADIR)

def make_ca():
    os.mkdir(TMP_CADIR)
    with open(os.path.join('cadir','index.txt'),'a+') as f:
        pass # empty file
    with open(os.path.join('cadir','crl.txt'),'a+') as f:
        f.write("00")
    with open(os.path.join('cadir','index.txt.attr'),'w+') as f:
        f.write('unique_subject = no')

    with tempfile.NamedTemporaryFile("w") as t:
        t.write(req_template.format(hostname='our-ca-server', extra_san=''))
        t.flush()
        with tempfile.NamedTemporaryFile() as f:
            args = ['req', '-new', '-days', '3650', '-extensions', 'v3_ca', '-nodes',
                    '-newkey', 'rsa:3072', '-keyout', 'pycakey.pem',
                    '-out', f.name,
                    '-subj', '/C=XY/L=Castle Anthrax/O=Python Software Foundation CA/CN=our-ca-server']
            check_call(['openssl'] + args)
            args = ['ca', '-config', t.name, '-create_serial',
                    '-out', 'pycacert.pem', '-batch', '-outdir', TMP_CADIR,
                    '-keyfile', 'pycakey.pem', '-days', '3650',
                    '-selfsign', '-extensions', 'v3_ca', '-infiles', f.name ]
            check_call(['openssl'] + args)
            args = ['ca', '-config', t.name, '-gencrl', '-out', 'revocation.crl']
            check_call(['openssl'] + args)

if __name__ == '__main__':
    os.chdir(here)
    cert, key = make_cert_key('localhost')
    with open('ssl_cert.pem', 'w') as f:
        f.write(cert)
    with open('ssl_key.pem', 'w') as f:
        f.write(key)
    print("password protecting ssl_key.pem in ssl_key.passwd.pem")
    check_call(['openssl','rsa','-in','ssl_key.pem','-out','ssl_key.passwd.pem','-des3','-passout','pass:somepass'])
    check_call(['openssl','rsa','-in','ssl_key.pem','-out','keycert.passwd.pem','-des3','-passout','pass:somepass'])

    with open('keycert.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    with open('keycert.passwd.pem', 'a+') as f:
        f.write(cert)

    # For certificate matching tests
    make_ca()
    cert, key = make_cert_key('fakehostname')
    with open('keycert2.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    cert, key = make_cert_key('localhost', True)
    with open('keycert3.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    cert, key = make_cert_key('fakehostname', True)
    with open('keycert4.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    extra_san = [
        'otherName.1 = 1.2.3.4;UTF8:some other identifier',
        'otherName.2 = 1.3.6.1.5.2.2;SEQUENCE:princ_name',
        'email.1 = user@example.org',
        'DNS.2 = www.example.org',
        # GEN_X400
        'dirName.1 = dir_sect',
        # GEN_EDIPARTY
        'URI.1 = https://www.python.org/',
        'IP.1 = 127.0.0.1',
        'IP.2 = ::1',
        'RID.1 = 1.2.3.4.5',
    ]

    cert, key = make_cert_key('allsans', extra_san='\n'.join(extra_san))
    with open('allsans.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    unmake_ca()
    print("\n\nPlease change the values in test_ssl.py, test_parse_cert function related to notAfter,notBefore and serialNumber")
    check_call(['openssl','x509','-in','keycert.pem','-dates','-serial','-noout'])
