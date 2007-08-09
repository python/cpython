"""
   Tests for the mhlib module
   Nick Mathewson
"""

### BUG: This suite doesn't currently test the mime functionality of
###      mhlib.  It should.

import unittest
from test.test_support import run_unittest, TESTFN, TestSkipped
import os
import io
import sys
import mhlib

if (sys.platform.startswith("win") or sys.platform=="riscos" or
      sys.platform.startswith("atheos")):
    # mhlib.updateline() renames a file to the name of a file that already
    # exists.  That causes a reasonable OS <wink> to complain in test_sequence
    # here, like the "OSError: [Errno 17] File exists" raised on Windows.
    # mhlib's listsubfolders() and listallfolders() do something with
    # link counts, and that causes test_listfolders() here to get back
    # an empty list from its call of listallfolders().
    # The other tests here pass on Windows.
    raise TestSkipped("skipped on %s -- " % sys.platform +
                      "too many Unix assumptions")

_mhroot = TESTFN+"_MH"
_mhpath = os.path.join(_mhroot, "MH")
_mhprofile = os.path.join(_mhroot, ".mh_profile")

def normF(f):
    return os.path.join(*f.split('/'))

def writeFile(fname, contents):
    dir = os.path.split(fname)[0]
    if dir and not os.path.exists(dir):
        mkdirs(dir)
    f = open(fname, 'w')
    f.write(contents)
    f.close()

def readFile(fname):
    f = open(fname)
    r = f.read()
    f.close()
    return r

def writeProfile(dict):
    contents = [ "%s: %s\n" % (k, v) for k, v in dict.items() ]
    writeFile(_mhprofile, "".join(contents))

def writeContext(folder):
    folder = normF(folder)
    writeFile(os.path.join(_mhpath, "context"),
              "Current-Folder: %s\n" % folder)

def writeCurMessage(folder, cur):
    folder = normF(folder)
    writeFile(os.path.join(_mhpath, folder, ".mh_sequences"),
              "cur: %s\n"%cur)

def writeMessage(folder, n, headers, body):
    folder = normF(folder)
    headers = "".join([ "%s: %s\n" % (k, v) for k, v in headers.items() ])
    contents = "%s\n%s\n" % (headers,body)
    mkdirs(os.path.join(_mhpath, folder))
    writeFile(os.path.join(_mhpath, folder, str(n)), contents)

def getMH():
    return mhlib.MH(os.path.abspath(_mhpath), _mhprofile)

def sortLines(s):
    lines = s.split("\n")
    lines = [ line.strip() for line in lines if len(line) >= 2 ]
    lines.sort()
    return lines

# These next 2 functions are copied from test_glob.py.
def mkdirs(fname):
    if os.path.exists(fname) or fname == '':
        return
    base, file = os.path.split(fname)
    mkdirs(base)
    os.mkdir(fname)

def deltree(fname):
    if not os.path.exists(fname):
        return
    for f in os.listdir(fname):
        fullname = os.path.join(fname, f)
        if os.path.isdir(fullname):
            deltree(fullname)
        else:
            try:
                os.unlink(fullname)
            except:
                pass
    try:
        os.rmdir(fname)
    except:
        pass

class MhlibTests(unittest.TestCase):
    def setUp(self):
        deltree(_mhroot)
        mkdirs(_mhpath)
        writeProfile({'Path' : os.path.abspath(_mhpath),
                      'Editor': 'emacs',
                      'ignored-attribute': 'camping holiday'})
        # Note: These headers aren't really conformant to RFC822, but
        #  mhlib shouldn't care about that.

        # An inbox with a couple of messages.
        writeMessage('inbox', 1,
                     {'From': 'Mrs. Premise',
                      'To': 'Mrs. Conclusion',
                      'Date': '18 July 2001'}, "Hullo, Mrs. Conclusion!\n")
        writeMessage('inbox', 2,
                     {'From': 'Mrs. Conclusion',
                      'To': 'Mrs. Premise',
                      'Date': '29 July 2001'}, "Hullo, Mrs. Premise!\n")

        # A folder with many messages
        for i in list(range(5, 101))+list(range(101, 201, 2)):
            writeMessage('wide', i,
                         {'From': 'nowhere', 'Subject': 'message #%s' % i},
                         "This is message number %s\n" % i)

        # A deeply nested folder
        def deep(folder, n):
            writeMessage(folder, n,
                         {'Subject': 'Message %s/%s' % (folder, n) },
                         "This is message number %s in %s\n" % (n, folder) )
        deep('deep/f1', 1)
        deep('deep/f1', 2)
        deep('deep/f1', 3)
        deep('deep/f2', 4)
        deep('deep/f2', 6)
        deep('deep', 3)
        deep('deep/f2/f3', 1)
        deep('deep/f2/f3', 2)

    def tearDown(self):
        deltree(_mhroot)

    def test_basic(self):
        writeContext('inbox')
        writeCurMessage('inbox', 2)
        mh = getMH()

        eq = self.assertEquals
        eq(mh.getprofile('Editor'), 'emacs')
        eq(mh.getprofile('not-set'), None)
        eq(mh.getpath(), os.path.abspath(_mhpath))
        eq(mh.getcontext(), 'inbox')

        mh.setcontext('wide')
        eq(mh.getcontext(), 'wide')
        eq(readFile(os.path.join(_mhpath, 'context')),
           "Current-Folder: wide\n")

        mh.setcontext('inbox')

        inbox = mh.openfolder('inbox')
        eq(inbox.getfullname(),
           os.path.join(os.path.abspath(_mhpath), 'inbox'))
        eq(inbox.getsequencesfilename(),
           os.path.join(os.path.abspath(_mhpath), 'inbox', '.mh_sequences'))
        eq(inbox.getmessagefilename(1),
           os.path.join(os.path.abspath(_mhpath), 'inbox', '1'))

    def test_listfolders(self):
        mh = getMH()
        eq = self.assertEquals

        folders = mh.listfolders()
        folders.sort()
        eq(folders, ['deep', 'inbox', 'wide'])

        folders = mh.listallfolders()
        folders.sort()
        tfolders = sorted(map(normF, ['deep', 'deep/f1', 'deep/f2',
                                      'deep/f2/f3', 'inbox', 'wide']))
        eq(folders, tfolders)

        folders = mh.listsubfolders('deep')
        folders.sort()
        eq(folders, list(map(normF, ['deep/f1', 'deep/f2'])))

        folders = mh.listallsubfolders('deep')
        folders.sort()
        eq(folders, list(map(normF, ['deep/f1', 'deep/f2', 'deep/f2/f3'])))
        eq(mh.listsubfolders(normF('deep/f2')), [normF('deep/f2/f3')])

        eq(mh.listsubfolders('inbox'), [])
        eq(mh.listallsubfolders('inbox'), [])

    def test_sequence(self):
        mh = getMH()
        eq = self.assertEquals
        writeCurMessage('wide', 55)

        f = mh.openfolder('wide')
        all = f.listmessages()
        eq(all, list(range(5, 101))+list(range(101, 201, 2)))
        eq(f.getcurrent(), 55)
        f.setcurrent(99)
        eq(readFile(os.path.join(_mhpath, 'wide', '.mh_sequences')),
           'cur: 99\n')

        def seqeq(seq, val):
            eq(f.parsesequence(seq), val)

        seqeq('5-55', list(range(5, 56)))
        seqeq('90-108', list(range(90, 101))+list(range(101, 109, 2)))
        seqeq('90-108', list(range(90, 101))+list(range(101, 109, 2)))

        seqeq('10:10', list(range(10, 20)))
        seqeq('10:+10', list(range(10, 20)))
        seqeq('101:10', list(range(101, 121, 2)))

        seqeq('cur', [99])
        seqeq('.', [99])
        seqeq('prev', [98])
        seqeq('next', [100])
        seqeq('cur:-3', [97, 98, 99])
        seqeq('first-cur', list(range(5, 100)))
        seqeq('150-last', list(range(151, 201, 2)))
        seqeq('prev-next', [98, 99, 100])

        lowprimes = [5, 7, 11, 13, 17, 19, 23, 29]
        lowcompos = [x for x in range(5, 31) if not x in lowprimes ]
        f.putsequences({'cur': [5],
                        'lowprime': lowprimes,
                        'lowcompos': lowcompos})
        seqs = readFile(os.path.join(_mhpath, 'wide', '.mh_sequences'))
        seqs = sortLines(seqs)
        eq(seqs, ["cur: 5",
                  "lowcompos: 6 8-10 12 14-16 18 20-22 24-28 30",
                  "lowprime: 5 7 11 13 17 19 23 29"])

        seqeq('lowprime', lowprimes)
        seqeq('lowprime:1', [5])
        seqeq('lowprime:2', [5, 7])
        seqeq('lowprime:-2', [23, 29])

        ## Not supported
        #seqeq('lowprime:first', [5])
        #seqeq('lowprime:last', [29])
        #seqeq('lowprime:prev', [29])
        #seqeq('lowprime:next', [29])

    def test_modify(self):
        mh = getMH()
        eq = self.assertEquals

        mh.makefolder("dummy1")
        self.assert_("dummy1" in mh.listfolders())
        path = os.path.join(_mhpath, "dummy1")
        self.assert_(os.path.exists(path))

        f = mh.openfolder('dummy1')
        def create(n):
            msg = "From: foo\nSubject: %s\n\nDummy Message %s\n" % (n,n)
            f.createmessage(n, io.StringIO(msg))

        create(7)
        create(8)
        create(9)

        eq(readFile(f.getmessagefilename(9)),
           "From: foo\nSubject: 9\n\nDummy Message 9\n")

        eq(f.listmessages(), [7, 8, 9])
        files = os.listdir(path)
        files.sort()
        eq(files, ['7', '8', '9'])

        f.removemessages(['7', '8'])
        files = os.listdir(path)
        files.sort()
        eq(files, [',7', ',8', '9'])
        eq(f.listmessages(), [9])
        create(10)
        create(11)
        create(12)

        mh.makefolder("dummy2")
        f2 = mh.openfolder("dummy2")
        eq(f2.listmessages(), [])
        f.movemessage(10, f2, 3)
        f.movemessage(11, f2, 5)
        eq(f.listmessages(), [9, 12])
        eq(f2.listmessages(), [3, 5])
        eq(readFile(f2.getmessagefilename(3)),
           "From: foo\nSubject: 10\n\nDummy Message 10\n")

        f.copymessage(9, f2, 4)
        eq(f.listmessages(), [9, 12])
        eq(readFile(f2.getmessagefilename(4)),
           "From: foo\nSubject: 9\n\nDummy Message 9\n")

        f.refilemessages([9, 12], f2)
        eq(f.listmessages(), [])
        eq(f2.listmessages(), [3, 4, 5, 6, 7])
        eq(readFile(f2.getmessagefilename(7)),
           "From: foo\nSubject: 12\n\nDummy Message 12\n")
        # XXX This should check that _copysequences does the right thing.

        mh.deletefolder('dummy1')
        mh.deletefolder('dummy2')
        self.assert_('dummy1' not in mh.listfolders())
        self.assert_(not os.path.exists(path))

    def test_read(self):
        mh = getMH()
        eq = self.assertEquals

        f = mh.openfolder('inbox')
        msg = f.openmessage(1)
        # Check some basic stuff from rfc822
        eq(msg.getheader('From'), "Mrs. Premise")
        eq(msg.getheader('To'), "Mrs. Conclusion")

        # Okay, we have the right message.  Let's check the stuff from
        # mhlib.
        lines = sortLines(msg.getheadertext())
        eq(lines, ["Date: 18 July 2001",
                   "From: Mrs. Premise",
                   "To: Mrs. Conclusion"])
        lines = sortLines(msg.getheadertext(lambda h: len(h)==4))
        eq(lines, ["Date: 18 July 2001",
                   "From: Mrs. Premise"])
        eq(msg.getbodytext(), "Hullo, Mrs. Conclusion!\n\n")
        eq(msg.getbodytext(0), "Hullo, Mrs. Conclusion!\n\n")

        # XXXX there should be a better way to reclaim the file handle
        msg.fp.close()
        del msg


def test_main():
    run_unittest(MhlibTests)


if __name__ == "__main__":
    test_main()
