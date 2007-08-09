import mimetools
import multifile
import io

msg = """Mime-Version: 1.0
Content-Type: multipart/mixed;
        boundary="=====================_590453667==_"
X-OriginalArrivalTime: 05 Feb 2002 03:43:23.0310 (UTC) FILETIME=[42D88CE0:01C1ADF7]

--=====================_590453667==_
Content-Type: multipart/alternative;
        boundary="=====================_590453677==_.ALT"

--=====================_590453677==_.ALT
Content-Type: text/plain; charset="us-ascii"; format=flowed

test A
--=====================_590453677==_.ALT
Content-Type: text/html; charset="us-ascii"

<html>
<b>test B</font></b></html>

--=====================_590453677==_.ALT--

--=====================_590453667==_
Content-Type: text/plain; charset="us-ascii"
Content-Disposition: attachment; filename="att.txt"

Attached Content.
Attached Content.
Attached Content.
Attached Content.

--=====================_590453667==_--

"""

def getMIMEMsg(mf):
    global boundaries, linecount
    msg = mimetools.Message(mf)

    #print "TYPE: %s" % msg.gettype()
    if msg.getmaintype() == 'multipart':
        boundary = msg.getparam("boundary")
        boundaries += 1

        mf.push(boundary)
        while mf.next():
            getMIMEMsg(mf)
        mf.pop()
    else:
        lines = mf.readlines()
        linecount += len(lines)

def test_main():
    global boundaries, linecount
    boundaries = 0
    linecount = 0
    f = io.StringIO(msg)
    getMIMEMsg(multifile.MultiFile(f))
    assert boundaries == 2
    assert linecount == 9

if __name__ == '__main__':
    test_main()
