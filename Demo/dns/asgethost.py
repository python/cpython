import sys
import dnslib
import dnstype
import dnsopcode
import dnsclass
import socket
import select

def main():
    server = 'cnri.reston.va.us'        # How?
    port = 53
    opcode = dnsopcode.QUERY
    rd = 0
    qtype = dnstype.MX
    qname = sys.argv[1:] and sys.argv[1] or 'www.python.org'
    m = dnslib.Mpacker()
    m.addHeader(0,
                0, opcode, 0, 0, rd, 0, 0, 0,
                1, 0, 0, 0)
    m.addQuestion(qname, qtype, dnsclass.IN)
    request = m.getbuf()
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect((server, port))
    s.send(request)
    while 1:
        r, w, x = [s], [], []
        r, w, x = select.select(r, w, x, 0.333)
        print r, w, x
        if r:
            reply = s.recv(1024)
            u = dnslib.Munpacker(reply)
            dnslib.dumpM(u)
            break

main()
