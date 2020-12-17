from __future__ import print_function

import ssl
import sys

import OpenSSL.SSL
import cffi
import cryptography

from . import version


_env_info = u"""\
pyOpenSSL: {pyopenssl}
cryptography: {cryptography}
cffi: {cffi}
cryptography's compiled against OpenSSL: {crypto_openssl_compile}
cryptography's linked OpenSSL: {crypto_openssl_link}
Python's OpenSSL: {python_openssl}
Python executable: {python}
Python version: {python_version}
Platform: {platform}
sys.path: {sys_path}""".format(
    pyopenssl=version.__version__,
    crypto_openssl_compile=OpenSSL._util.ffi.string(
        OpenSSL._util.lib.OPENSSL_VERSION_TEXT,
    ).decode("ascii"),
    crypto_openssl_link=OpenSSL.SSL.SSLeay_version(
        OpenSSL.SSL.SSLEAY_VERSION
    ).decode("ascii"),
    python_openssl=getattr(ssl, "OPENSSL_VERSION", "n/a"),
    cryptography=cryptography.__version__,
    cffi=cffi.__version__,
    python=sys.executable,
    python_version=sys.version,
    platform=sys.platform,
    sys_path=sys.path,
)


if __name__ == "__main__":
    print(_env_info)
