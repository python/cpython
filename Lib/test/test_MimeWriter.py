"""Test program for MimeWriter module.

The test program was too big to comfortably fit in the MimeWriter
class, so it's here in its own file.

This should generate Barry's example, modulo some quotes and newlines.

"""

import unittest, StringIO
from test.test_support import run_unittest, import_module

import_module("MimeWriter", deprecated=True)
from MimeWriter import MimeWriter

SELLER = '''\
INTERFACE Seller-1;

TYPE Seller = OBJECT
    DOCUMENTATION "A simple Seller interface to test ILU"
    METHODS
            price():INTEGER,
    END;
'''

BUYER = '''\
class Buyer:
    def __setup__(self, maxprice):
        self._maxprice = maxprice

    def __main__(self, kos):
        """Entry point upon arrival at a new KOS."""
        broker = kos.broker()
        # B4 == Barry's Big Bass Business :-)
        seller = broker.lookup('Seller_1.Seller', 'B4')
        if seller:
            price = seller.price()
            print 'Seller wants $', price, '... '
            if price > self._maxprice:
                print 'too much!'
            else:
                print "I'll take it!"
        else:
            print 'no seller found here'
'''                                     # Don't ask why this comment is here

STATE = '''\
# instantiate a buyer instance and put it in a magic place for the KOS
# to find.
__kp__ = Buyer()
__kp__.__setup__(500)
'''

SIMPLE_METADATA = [
        ("Interpreter", "python"),
        ("Interpreter-Version", "1.3"),
        ("Owner-Name", "Barry Warsaw"),
        ("Owner-Rendezvous", "bwarsaw@cnri.reston.va.us"),
        ("Home-KSS", "kss.cnri.reston.va.us"),
        ("Identifier", "hdl://cnri.kss/my_first_knowbot"),
        ("Launch-Date", "Mon Feb 12 16:39:03 EST 1996"),
        ]

COMPLEX_METADATA = [
        ("Metadata-Type", "complex"),
        ("Metadata-Key", "connection"),
        ("Access", "read-only"),
        ("Connection-Description", "Barry's Big Bass Business"),
        ("Connection-Id", "B4"),
        ("Connection-Direction", "client"),
        ]

EXTERNAL_METADATA = [
        ("Metadata-Type", "complex"),
        ("Metadata-Key", "generic-interface"),
        ("Access", "read-only"),
        ("Connection-Description", "Generic Interface for All Knowbots"),
        ("Connection-Id", "generic-kp"),
        ("Connection-Direction", "client"),
        ]


OUTPUT = '''\
From: bwarsaw@cnri.reston.va.us
Date: Mon Feb 12 17:21:48 EST 1996
To: kss-submit@cnri.reston.va.us
MIME-Version: 1.0
Content-Type: multipart/knowbot;
    boundary="801spam999";
    version="0.1"

This is a multi-part message in MIME format.

--801spam999
Content-Type: multipart/knowbot-metadata;
    boundary="802spam999"


--802spam999
Content-Type: message/rfc822
KP-Metadata-Type: simple
KP-Access: read-only

KPMD-Interpreter: python
KPMD-Interpreter-Version: 1.3
KPMD-Owner-Name: Barry Warsaw
KPMD-Owner-Rendezvous: bwarsaw@cnri.reston.va.us
KPMD-Home-KSS: kss.cnri.reston.va.us
KPMD-Identifier: hdl://cnri.kss/my_first_knowbot
KPMD-Launch-Date: Mon Feb 12 16:39:03 EST 1996

--802spam999
Content-Type: text/isl
KP-Metadata-Type: complex
KP-Metadata-Key: connection
KP-Access: read-only
KP-Connection-Description: Barry's Big Bass Business
KP-Connection-Id: B4
KP-Connection-Direction: client

INTERFACE Seller-1;

TYPE Seller = OBJECT
    DOCUMENTATION "A simple Seller interface to test ILU"
    METHODS
            price():INTEGER,
    END;

--802spam999
Content-Type: message/external-body;
    access-type="URL";
    URL="hdl://cnri.kss/generic-knowbot"

Content-Type: text/isl
KP-Metadata-Type: complex
KP-Metadata-Key: generic-interface
KP-Access: read-only
KP-Connection-Description: Generic Interface for All Knowbots
KP-Connection-Id: generic-kp
KP-Connection-Direction: client


--802spam999--

--801spam999
Content-Type: multipart/knowbot-code;
    boundary="803spam999"


--803spam999
Content-Type: text/plain
KP-Module-Name: BuyerKP

class Buyer:
    def __setup__(self, maxprice):
        self._maxprice = maxprice

    def __main__(self, kos):
        """Entry point upon arrival at a new KOS."""
        broker = kos.broker()
        # B4 == Barry's Big Bass Business :-)
        seller = broker.lookup('Seller_1.Seller', 'B4')
        if seller:
            price = seller.price()
            print 'Seller wants $', price, '... '
            if price > self._maxprice:
                print 'too much!'
            else:
                print "I'll take it!"
        else:
            print 'no seller found here'

--803spam999--

--801spam999
Content-Type: multipart/knowbot-state;
    boundary="804spam999"
KP-Main-Module: main


--804spam999
Content-Type: text/plain
KP-Module-Name: main

# instantiate a buyer instance and put it in a magic place for the KOS
# to find.
__kp__ = Buyer()
__kp__.__setup__(500)

--804spam999--

--801spam999--
'''

class MimewriterTest(unittest.TestCase):

    def test(self):
        buf = StringIO.StringIO()

        # Toplevel headers

        toplevel = MimeWriter(buf)
        toplevel.addheader("From", "bwarsaw@cnri.reston.va.us")
        toplevel.addheader("Date", "Mon Feb 12 17:21:48 EST 1996")
        toplevel.addheader("To", "kss-submit@cnri.reston.va.us")
        toplevel.addheader("MIME-Version", "1.0")

        # Toplevel body parts

        f = toplevel.startmultipartbody("knowbot", "801spam999",
                                        [("version", "0.1")], prefix=0)
        f.write("This is a multi-part message in MIME format.\n")

        # First toplevel body part: metadata

        md = toplevel.nextpart()
        md.startmultipartbody("knowbot-metadata", "802spam999")

        # Metadata part 1

        md1 = md.nextpart()
        md1.addheader("KP-Metadata-Type", "simple")
        md1.addheader("KP-Access", "read-only")
        m = MimeWriter(md1.startbody("message/rfc822"))
        for key, value in SIMPLE_METADATA:
            m.addheader("KPMD-" + key, value)
        m.flushheaders()
        del md1

        # Metadata part 2

        md2 = md.nextpart()
        for key, value in COMPLEX_METADATA:
            md2.addheader("KP-" + key, value)
        f = md2.startbody("text/isl")
        f.write(SELLER)
        del md2

        # Metadata part 3

        md3 = md.nextpart()
        f = md3.startbody("message/external-body",
                          [("access-type", "URL"),
                           ("URL", "hdl://cnri.kss/generic-knowbot")])
        m = MimeWriter(f)
        for key, value in EXTERNAL_METADATA:
            md3.addheader("KP-" + key, value)
        md3.startbody("text/isl")
        # Phantom body doesn't need to be written

        md.lastpart()

        # Second toplevel body part: code

        code = toplevel.nextpart()
        code.startmultipartbody("knowbot-code", "803spam999")

        # Code: buyer program source

        buyer = code.nextpart()
        buyer.addheader("KP-Module-Name", "BuyerKP")
        f = buyer.startbody("text/plain")
        f.write(BUYER)

        code.lastpart()

        # Third toplevel body part: state

        state = toplevel.nextpart()
        state.addheader("KP-Main-Module", "main")
        state.startmultipartbody("knowbot-state", "804spam999")

        # State: a bunch of assignments

        st = state.nextpart()
        st.addheader("KP-Module-Name", "main")
        f = st.startbody("text/plain")
        f.write(STATE)

        state.lastpart()

        # End toplevel body parts

        toplevel.lastpart()

        self.assertEqual(buf.getvalue(), OUTPUT)

def test_main():
    run_unittest(MimewriterTest)

if __name__ == '__main__':
    test_main()
