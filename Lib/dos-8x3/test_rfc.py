from test_support import verbose
import rfc822, sys
try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

def test(msg, results):
    fp = StringIO()
    fp.write(msg)
    fp.seek(0)
    m = rfc822.Message(fp)
    i = 0
    for n, a in m.getaddrlist('to') + m.getaddrlist('cc'):
        if verbose:
            print 'name:', repr(n), 'addr:', repr(a)
        try:
            mn, ma = results[i][0], results[i][1]
        except IndexError:
            print 'extra parsed address:', repr(n), repr(a)
            continue
        i = i + 1
        if mn == n and ma == a:
            if verbose:
                print '    [matched]'
        else:
            if verbose:
                print '    [no match]'
            print 'not found:', repr(n), repr(a)

test('''Date:    Wed, 13 Jan 1999 23:57:35 -0500
From:    Guido van Rossum <guido@CNRI.Reston.VA.US>
To:      "Guido van
	 : Rossum" <guido@python.org>
Subject: test2

test2
''', [('Guido van\n	 : Rossum', 'guido@python.org')])

test('''From: Barry <bwarsaw@python.org
To: guido@python.org (Guido: the Barbarian)
Subject: nonsense

test''', [('Guido: the Barbarian', 'guido@python.org'),
          ])

test('''From: Barry <bwarsaw@python.org
To: guido@python.org (Guido: the Barbarian)
Cc: "Guido: the Madman" <guido@python.org>

test''', [('Guido: the Barbarian', 'guido@python.org'),
          ('Guido: the Madman', 'guido@python.org')
          ])

test('''To: "The monster with
     the very long name: Guido" <guido@python.org>

test''', [('The monster with\n     the very long name: Guido',
           'guido@python.org')])

test('''To: "Amit J. Patel" <amitp@Theory.Stanford.EDU>
CC: Mike Fletcher <mfletch@vrtelecom.com>,
        "'string-sig@python.org'" <string-sig@python.org>
Cc: fooz@bat.com, bart@toof.com
Cc: goit@lip.com

test''', [('Amit J. Patel', 'amitp@Theory.Stanford.EDU'),
          ('Mike Fletcher', 'mfletch@vrtelecom.com'),
          ("'string-sig@python.org'", 'string-sig@python.org'),
          ('', 'fooz@bat.com'),
          ('', 'bart@toof.com'),
          ('', 'goit@lip.com'),
          ])

# This one is just twisted.  I don't know what the proper result should be,
# but it shouldn't be to infloop, which is what used to happen!
test('''To: <[smtp:dd47@mail.xxx.edu]_at_hmhq@hdq-mdm1-imgout.companay.com>

test''', [('', ''),
          ('', 'dd47@mail.xxx.edu'),
          ('', '_at_hmhq@hdq-mdm1-imgout.companay.com')
          ])
