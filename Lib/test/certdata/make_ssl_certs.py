"""Make the custom certificate and private key files used by test_ssl
and friends."""

import argparse
import os
import pprint
import shutil
import tempfile
from subprocess import *

startdate = "20180829142316Z"
enddate_default = "25251028142316Z"
days_default = "140000"

req_template = """
    [ default ]
    base_url               = http://testca.pythontest.net/testca

    [req]
    distinguished_name     = req_distinguished_name
    prompt                 = no

    [req_distinguished_name]
    C                      = XY
    L                      = Castle Anthrax
    O                      = Python Software Foundation
    CN                     = {hostname}

    [req_x509_extensions_nosan]

    [req_x509_extensions_simple]
    subjectAltName         = @san

    [req_x509_extensions_full]
    subjectAltName         = @san
    keyUsage               = critical,keyEncipherment,digitalSignature
    extendedKeyUsage       = serverAuth,clientAuth
    basicConstraints       = critical,CA:false
    subjectKeyIdentifier   = hash
    authorityKeyIdentifier = keyid:always,issuer:always
    authorityInfoAccess    = @issuer_ocsp_info
    crlDistributionPoints  = @crl_info

    [ issuer_ocsp_info ]
    caIssuers;URI.0        = $base_url/pycacert.cer
    OCSP;URI.0             = $base_url/ocsp/

    [ crl_info ]
    URI.0                  = $base_url/revocation.crl

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
    startdate = {startdate}
    default_startdate = {startdate}
    enddate = {enddate}
    default_enddate = {enddate}
    default_days = {days}
    default_crl_days = {days}
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
    basicConstraints = critical, CA:true
    keyUsage = critical, digitalSignature, keyCertSign, cRLSign

    """

here = os.path.abspath(os.path.dirname(__file__))


def make_cert_key(cmdlineargs, hostname, sign=False, extra_san='',
                  ext='req_x509_extensions_full', key='rsa:3072'):
    print("creating cert for " + hostname)
    tempnames = []
    for i in range(3):
        with tempfile.NamedTemporaryFile(delete=False) as f:
            tempnames.append(f.name)
    req_file, cert_file, key_file = tempnames
    try:
        req = req_template.format(
            hostname=hostname,
            extra_san=extra_san,
            startdate=startdate,
            enddate=cmdlineargs.enddate,
            days=cmdlineargs.days
        )
        with open(req_file, 'w') as f:
            f.write(req)
        args = ['req', '-new', '-nodes', '-days', cmdlineargs.days,
                '-newkey', key, '-keyout', key_file,
                '-config', req_file]
        if sign:
            with tempfile.NamedTemporaryFile(delete=False) as f:
                tempnames.append(f.name)
                reqfile = f.name
            args += ['-out', reqfile ]

        else:
            args += ['-extensions', ext, '-x509', '-out', cert_file ]
        check_call(['openssl'] + args)

        if sign:
            args = [
                'ca',
                '-config', req_file,
                '-extensions', ext,
                '-out', cert_file,
                '-outdir', 'cadir',
                '-policy', 'policy_anything',
                '-batch', '-infiles', reqfile
            ]
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

def make_ca(cmdlineargs):
    os.mkdir(TMP_CADIR)
    with open(os.path.join('cadir','index.txt'),'a+') as f:
        pass # empty file
    with open(os.path.join('cadir','crl.txt'),'a+') as f:
        f.write("00")
    with open(os.path.join('cadir','index.txt.attr'),'w+') as f:
        f.write('unique_subject = no')
    # random start value for serial numbers
    with open(os.path.join('cadir','serial'), 'w') as f:
        f.write('CB2D80995A69525B\n')

    with tempfile.NamedTemporaryFile("w") as t:
        req = req_template.format(
            hostname='our-ca-server',
            extra_san='',
            startdate=startdate,
            enddate=cmdlineargs.enddate,
            days=cmdlineargs.days
        )
        t.write(req)
        t.flush()
        with tempfile.NamedTemporaryFile() as f:
            args = ['req', '-config', t.name, '-new',
                    '-nodes',
                    '-newkey', 'rsa:3072',
                    '-keyout', 'pycakey.pem',
                    '-out', f.name,
                    '-subj', '/C=XY/L=Castle Anthrax/O=Python Software Foundation CA/CN=our-ca-server']
            check_call(['openssl'] + args)
            args = ['ca', '-config', t.name,
                    '-out', 'pycacert.pem', '-batch', '-outdir', TMP_CADIR,
                    '-keyfile', 'pycakey.pem',
                    '-selfsign', '-extensions', 'v3_ca', '-infiles', f.name ]
            check_call(['openssl'] + args)
            args = ['ca', '-config', t.name, '-gencrl', '-out', 'revocation.crl']
            check_call(['openssl'] + args)

    # capath hashes depend on subject!
    check_call([
        'openssl', 'x509', '-in', 'pycacert.pem', '-out', 'capath/ceff1710.0'
    ])
    shutil.copy('capath/ceff1710.0', 'capath/b1930218.0')


def write_cert_reference(path):
    import _ssl
    refdata = pprint.pformat(_ssl._test_decode_cert(path))
    print(refdata)
    with open(path + '.reference', 'w') as f:
        print(refdata, file=f)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Make the custom certificate and private key files used by test_ssl and friends.')
    parser.add_argument('--days', default=days_default)
    parser.add_argument('--enddate', default=enddate_default)
    cmdlineargs = parser.parse_args()

    os.chdir(here)
    cert, key = make_cert_key(cmdlineargs, 'localhost', ext='req_x509_extensions_simple')
    with open('ssl_cert.pem', 'w') as f:
        f.write(cert)
    with open('ssl_key.pem', 'w') as f:
        f.write(key)
    print("password protecting ssl_key.pem in ssl_key.passwd.pem")
    check_call(['openssl','pkey','-in','ssl_key.pem','-out','ssl_key.passwd.pem','-aes256','-passout','pass:somepass'])
    check_call(['openssl','pkey','-in','ssl_key.pem','-out','keycert.passwd.pem','-aes256','-passout','pass:somepass'])

    with open('keycert.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    with open('keycert.passwd.pem', 'a+') as f:
        f.write(cert)

    # For certificate matching tests
    make_ca(cmdlineargs)
    cert, key = make_cert_key(cmdlineargs, 'fakehostname', ext='req_x509_extensions_simple')
    with open('keycert2.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    cert, key = make_cert_key(cmdlineargs, 'localhost', sign=True)
    with open('keycert3.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    check_call(['openssl', 'x509', '-outform', 'pem', '-in', 'keycert3.pem', '-out', 'cert3.pem'])

    cert, key = make_cert_key(cmdlineargs, 'fakehostname', sign=True)
    with open('keycert4.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    cert, key = make_cert_key(
        cmdlineargs, 'localhost-ecc', sign=True, key='param:secp384r1.pem'
    )
    with open('keycertecc.pem', 'w') as f:
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

    cert, key = make_cert_key(cmdlineargs, 'allsans', sign=True, extra_san='\n'.join(extra_san))
    with open('allsans.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    extra_san = [
        # könig (king)
        'DNS.2 = xn--knig-5qa.idn.pythontest.net',
        # königsgäßchen (king's alleyway)
        'DNS.3 = xn--knigsgsschen-lcb0w.idna2003.pythontest.net',
        'DNS.4 = xn--knigsgchen-b4a3dun.idna2008.pythontest.net',
        # βόλοσ (marble)
        'DNS.5 = xn--nxasmq6b.idna2003.pythontest.net',
        'DNS.6 = xn--nxasmm1c.idna2008.pythontest.net',
    ]

    # IDN SANS, signed
    cert, key = make_cert_key(cmdlineargs, 'idnsans', sign=True, extra_san='\n'.join(extra_san))
    with open('idnsans.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    cert, key = make_cert_key(cmdlineargs, 'nosan', sign=True, ext='req_x509_extensions_nosan')
    with open('nosan.pem', 'w') as f:
        f.write(key)
        f.write(cert)

    unmake_ca()
    print("Writing out reference data for Lib/test/test_ssl.py and Lib/test/test_asyncio/utils.py")
    write_cert_reference('keycert.pem')
    write_cert_reference('keycert3.pem')
