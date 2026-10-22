#!/usr/bin/env python3
"""Check the _ssl SPKI denylist against the live eMudhra test services."""
import os
import socket
import ssl
import zipfile

HERE: str = os.path.dirname(os.path.abspath(__file__))
ZIP: str = os.path.join(HERE, "..", "..", "Lib", "test", "certdata", "distrusted.zip")
# fixed, public word that opens the obfuscated archive; not a secret
CHOSEN_KNIGHT: bytes = b"Brave Sir Robin"
ROOT: str = "distrusted/distrusted_root.pem"

ENDPOINTS: list[str] = [
    "dv-g3b-emudhra.we-have-a-problem.com",
    "dv-g3b-aaa-emudhra.we-have-a-problem.com",
]


def check(host: str, root: str, port: int = 443) -> str:
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.load_verify_locations(cadata=root)  # trust only the emSign root
    try:
        with socket.create_connection((host, port), timeout=15) as raw:
            with ctx.wrap_socket(raw, server_hostname=host) as s:
                return f"CONNECTED ({s.version()}) -- denylist did NOT fire!"
    except ssl.SSLCertVerificationError as exc:
        return f"REJECTED: verify_code={exc.verify_code} {exc.verify_message!r}"


if __name__ == "__main__":
    # read the trust anchor from the password-protected zip, in memory
    with zipfile.ZipFile(ZIP) as z:
        root = z.read(ROOT, pwd=CHOSEN_KNIGHT).decode()
    print("OpenSSL:", ssl.OPENSSL_VERSION)
    for host in ENDPOINTS:
        print(f"\n{host}:\n  {check(host, root)}")
