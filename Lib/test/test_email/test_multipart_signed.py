import hashlib
import unittest

from email.encoders import encode_7or8bit
from email.mime.application import MIMEApplication
from email.mime.signed import MIMEMultipartSigned
from email.mime.text import MIMEText
from email.policy import SMTP


def bogus_signer(msg, msgtexts):
    if len(msgtexts) != 2:
      raise ValueError("msgtexts should contain 2 items, got %d", len(msgtexts))
    if not isinstance(msgtexts[0], bytes):
      log.warning("sign_fun probably called from msg.as_string")
      return
    digest = hashlib.sha256(msgtexts[0]).hexdigest().encode('ascii')
    msgtexts[1] = msgtexts[1].replace(b'DIGEST_PLACEHOLDER', digest)


class TestMultipartSigned(unittest.TestCase):
    def test_happy_flow(self):
        main = MIMEMultipartSigned(sign_fun=bogus_signer, policy=SMTP, protocol="bogus", boundary="ZYXXYZ")
        main.attach(MIMEText("Hello audience. Please sign below the fold.\n---\u2702---"))
        signature_placeholder = "Bogus hash: DIGEST_PLACEHOLDER"
        main.attach(MIMEApplication(signature_placeholder, "bogus-signature", encode_7or8bit))
        serialized = main.as_bytes()
        expected_serialized = (b'Content-Type: multipart/signed; protocol="bogus"; boundary="ZYXXYZ"\r\n'
                               b'MIME-Version: 1.0\r\n\r\n--ZYXXYZ\r\nContent-Type: text/plain; charset="utf-8"\r\n'
                               b'MIME-Version: 1.0\r\nContent-Transfer-Encoding: base64\r\n\r\n'
                               b'SGVsbG8gYXVkaWVuY2UuIFBsZWFzZSBzaWduIGJlbG93IHRoZSBmb2xkLgotLS3inIItLS0=\r\n'
                               b'\r\n--ZYXXYZ\r\nContent-Type: application/bogus-signature\r\nMIME-Version: 1.0\r\n'
                               b'Content-Transfer-Encoding: 7bit\r\n'
                               b'\r\nBogus hash: d93f584571f90aa80a49ab83a9b1b64cee8e8c70f147f0b2a8eef8fa6efbf435\r\n'
                               b'--ZYXXYZ--\r\n')
        self.assertEqual(serialized, expected_serialized)

        # check that the original message object did not get modified
        new_signature_placeholder = main.get_payload()[1].get_payload()
        self.assertEqual(new_signature_placeholder, signature_placeholder)

    def test_no_sign_fun(self):
        main = MIMEMultipartSigned(policy=SMTP, protocol="bogus", boundary="ZYXXYZ")
        main.attach(MIMEText("Hello audience. Please sign below the fold.\n---\u2702---"))
        main.attach(MIMEApplication("Bogus hash: DIGEST_PLACEHOLDER",
                                    "bogus-signature",
                                    encode_7or8bit))
        serialized = main.as_bytes()
        expected_serialized = (b'Content-Type: multipart/signed; protocol="bogus"; boundary="ZYXXYZ"\r\n'
                               b'MIME-Version: 1.0\r\n\r\n--ZYXXYZ\r\nContent-Type: text/plain; charset="utf-8"\r\n'
                               b'MIME-Version: 1.0\r\nContent-Transfer-Encoding: base64\r\n\r\n'
                               b'SGVsbG8gYXVkaWVuY2UuIFBsZWFzZSBzaWduIGJlbG93IHRoZSBmb2xkLgotLS3inIItLS0=\r\n\r\n'
                               b'--ZYXXYZ\r\nContent-Type: application/bogus-signature\r\nMIME-Version: 1.0\r\n'
                               b'Content-Transfer-Encoding: 7bit\r\n\r\n'
                               b'Bogus hash: DIGEST_PLACEHOLDER\r\n--ZYXXYZ--\r\n')
        self.assertEqual(serialized, expected_serialized)

    def test_no_payload_is_ok(self):
        main = MIMEMultipartSigned(policy=SMTP, protocol="bogus", boundary="ZYXXYZ")
        serialized = main.as_bytes()
        expected_serialized = (b'Content-Type: multipart/signed; protocol="bogus"; boundary="ZYXXYZ"\r\n'
                               b'MIME-Version: 1.0\r\n\r\n--ZYXXYZ\r\n\r\n--ZYXXYZ--\r\n')
        self.assertEqual(serialized, expected_serialized)
