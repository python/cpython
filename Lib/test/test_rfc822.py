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

    out = m.getdate('date')
    if out:
        if verbose:
            print 'Date:', m.getheader('date')
        if out == (1999, 1, 13, 23, 57, 35, 0, 0, 0):
            if verbose:
                print '    [matched]'
        else:
            if verbose:
                print '    [no match]'
            print 'Date conversion failed:', out

# Note: all test cases must have the same date (in various formats),
# or no date!

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
Date: Wednesday, January 13 1999 23:57:35 -0500

test''', [('Guido: the Barbarian', 'guido@python.org'),
          ])

test('''From: Barry <bwarsaw@python.org
To: guido@python.org (Guido: the Barbarian)
Cc: "Guido: the Madman" <guido@python.org>
Date:  13-Jan-1999 23:57:35 EST

test''', [('Guido: the Barbarian', 'guido@python.org'),
          ('Guido: the Madman', 'guido@python.org')
          ])

test('''To: "The monster with
     the very long name: Guido" <guido@python.org>
Date:    Wed, 13 Jan 1999 23:57:35 -0500

test''', [('The monster with\n     the very long name: Guido',
           'guido@python.org')])

test('''To: "Amit J. Patel" <amitp@Theory.Stanford.EDU>
CC: Mike Fletcher <mfletch@vrtelecom.com>,
        "'string-sig@python.org'" <string-sig@python.org>
Cc: fooz@bat.com, bart@toof.com
Cc: goit@lip.com
Date:    Wed, 13 Jan 1999 23:57:35 -0500

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
Date:    Wed, 13 Jan 1999 23:57:35 -0500

test''', [('', ''),
          ('', 'dd47@mail.xxx.edu'),
          ('', '_at_hmhq@hdq-mdm1-imgout.companay.com')
          ])

# This exercises the old commas-in-a-full-name bug, which should be doing the
# right thing in recent versions of the module.
test('''To: "last, first" <userid@foo.net>

test''', [('last, first', 'userid@foo.net'),
          ])

test('''To: (Comment stuff) "Quoted name"@somewhere.com

test''', [('Comment stuff', '"Quoted name"@somewhere.com'),
          ])

test('''To: :
Cc: goit@lip.com
Date:    Wed, 13 Jan 1999 23:57:35 -0500

test''', [('', 'goit@lip.com')])


test('''To: guido@[132.151.1.21]

foo''', [('', 'guido@[132.151.1.21]')])
