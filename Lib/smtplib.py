#!/usr/bin/python
"""SMTP/ESMTP client class.

Author: The Dragon De Monsyne <dragondm@integral.org>
ESMTP support, test code and doc fixes added by
Eric S. Raymond <esr@thyrsus.com>
Better RFC 821 compliance (MAIL and RCPT, and CRLF in data)
by Carey Evans <c.evans@clear.net.nz>, for picky mail servers.

(This was modified from the Python 1.5 library HTTP lib.)

This should follow RFC 821 (SMTP) and RFC 1869 (ESMTP).

Example:

>>> import smtplib
>>> s=smtplib.SMTP("localhost")
>>> print s.help()
This is Sendmail version 8.8.4
Topics:
    HELO    EHLO    MAIL    RCPT    DATA
    RSET    NOOP    QUIT    HELP    VRFY
    EXPN    VERB    ETRN    DSN
For more info use "HELP <topic>".
To report bugs in the implementation send email to
    sendmail-bugs@sendmail.org.
For local information send email to Postmaster at your site.
End of HELP info
>>> s.putcmd("vrfy","someone@here")
>>> s.getreply()
(250, "Somebody OverHere <somebody@here.my.org>")
>>> s.quit()

"""

import socket
import string,re

SMTP_PORT = 25
CRLF="\r\n"

# used for exceptions 
SMTPServerDisconnected="Server not connected"
SMTPSenderRefused="Sender address refused"
SMTPRecipientsRefused="All Recipients refused"
SMTPDataError="Error transmitting message data"

def quoteaddr(addr):
    """Quote a subset of the email addresses defined by RFC 821.

    Technically, only a <mailbox> is allowed.  In addition,
    email addresses without a domain are permitted.

    Addresses will not be modified if they are already quoted
    (actually if they begin with '<' and end with '>'."""
    if re.match('(?s)\A<.*>\Z', addr):
        return addr

    localpart = None
    domain = ''
    try:
        at = string.rindex(addr, '@')
        localpart = addr[:at]
        domain = addr[at:]
    except ValueError:
        localpart = addr

    pat = re.compile(r'([<>()\[\]\\,;:@\"\001-\037\177])')
    return '<%s%s>' % (pat.sub(r'\\\1', localpart), domain)

def quotedata(data):
    """Quote data for email.

    Double leading '.', and change Unix newline '\n' into
    Internet CRLF end-of-line."""
    return re.sub(r'(?m)^\.', '..',
                  re.sub(r'\r?\n', CRLF, data))

class SMTP:
    """This class manages a connection to an SMTP or ESMTP server."""
    debuglevel = 0
    file = None
    helo_resp = None
    ehlo_resp = None
    esmtp_features = []

    def __init__(self, host = '', port = 0):
        """Initialize a new instance.

        If specified, `host' is the name of the remote host to which
        to connect.  If specified, `port' specifies the port to which
        to connect.  By default, smtplib.SMTP_PORT is used.

        """
        if host: self.connect(host, port)
    
    def set_debuglevel(self, debuglevel):
        """Set the debug output level.

        A non-false value results in debug messages for connection and
        for all messages sent to and received from the server.

        """
        self.debuglevel = debuglevel

    def verify(self, address):
        """ SMTP 'verify' command. Checks for address validity. """
        self.putcmd("vrfy", address)
        return self.getreply()

    def connect(self, host='localhost', port = 0):
        """Connect to a host on a given port.

        If the hostname ends with a colon (`:') followed by a number,
        that suffix will be stripped off and the number interpreted as
        the port number to use.

        Note:  This method is automatically invoked by __init__,
        if a host is specified during instantiation.

        """
        if not port:
            i = string.find(host, ':')
            if i >= 0:
                host, port = host[:i], host[i+1:]
                try: port = string.atoi(port)
                except string.atoi_error:
                    raise socket.error, "nonnumeric port"
        if not port: port = SMTP_PORT
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if self.debuglevel > 0: print 'connect:', (host, port)
        self.sock.connect(host, port)
        (code,msg)=self.getreply()
        if self.debuglevel >0 : print "connect:", msg
        return msg
    
    def send(self, str):
        """Send `str' to the server."""
        if self.debuglevel > 0: print 'send:', `str`
        if self.sock:
            self.sock.send(str)
        else:
            raise SMTPServerDisconnected
 
    def putcmd(self, cmd, args=""):
        """Send a command to the server.
        """
        str = '%s %s%s' % (cmd, args, CRLF)
        self.send(str)
    
    def getreply(self, linehook=None):
        """Get a reply from the server.
        
        Returns a tuple consisting of:
        - server response code (e.g. '250', or such, if all goes well)
          Note: returns -1 if it can't read response code.
        - server response string corresponding to response code
                (note : multiline responses converted to a single, 
                 multiline string)
        """
        resp=[]
        self.file = self.sock.makefile('rb')
        while 1:
            line = self.file.readline()
            if self.debuglevel > 0: print 'reply:', `line`
            resp.append(string.strip(line[4:]))
            code=line[:3]
            #check if multiline resp
            if line[3:4]!="-":
                break
            elif linehook:
                linehook(line)
        try:
            errcode = string.atoi(code)
        except(ValueError):
            errcode = -1

        errmsg = string.join(resp,"\n")
        if self.debuglevel > 0: 
            print 'reply: retcode (%s); Msg: %s' % (errcode,errmsg)
        return errcode, errmsg
    
    def docmd(self, cmd, args=""):
        """ Send a command, and return its response code """
        
        self.putcmd(cmd,args)
        (code,msg)=self.getreply()
        return code
# std smtp commands

    def helo(self, name=''):
        """ SMTP 'helo' command. Hostname to send for this command  
        defaults to the FQDN of the local host """
        name=string.strip(name)
        if len(name)==0:
                name=socket.gethostbyaddr(socket.gethostname())[0]
        self.putcmd("helo",name)
        (code,msg)=self.getreply()
        self.helo_resp=msg
        return code

    def ehlo(self, name=''):
        """ SMTP 'ehlo' command. Hostname to send for this command  
        defaults to the FQDN of the local host """
        name=string.strip(name)
        if len(name)==0:
                name=socket.gethostbyaddr(socket.gethostname())[0]
        self.putcmd("ehlo",name)
        (code,msg)=self.getreply(self.ehlo_hook)
        self.ehlo_resp=msg
        return code

    def ehlo_hook(self, line):
        # Interpret EHLO response lines
        if line[4] in string.uppercase+string.digits:
            self.esmtp_features.append(string.lower(string.strip(line)[4:]))

    def has_option(self, opt):
        """Does the server support a given SMTP option?"""
        return opt in self.esmtp_features

    def help(self, args=''):
        """ SMTP 'help' command. Returns help text from server """
        self.putcmd("help", args)
        (code,msg)=self.getreply()
        return msg

    def rset(self):
        """ SMTP 'rset' command. Resets session. """
        code=self.docmd("rset")
        return code

    def noop(self):
        """ SMTP 'noop' command. Doesn't do anything :> """
        code=self.docmd("noop")
        return code

    def mail(self,sender,options=[]):
        """ SMTP 'mail' command. Begins mail xfer session. """
        if options:
            options = " " + string.joinfields(options, ' ')
        else:
            options = ''
        self.putcmd("mail", "from:" + quoteaddr(sender) + options)
        return self.getreply()

    def rcpt(self,recip):
        """ SMTP 'rcpt' command. Indicates 1 recipient for this mail. """
        self.putcmd("rcpt","to:%s" % quoteaddr(recip))
        return self.getreply()

    def data(self,msg):
        """ SMTP 'DATA' command. Sends message data to server. 
            Automatically quotes lines beginning with a period per rfc821. """
        self.putcmd("data")
        (code,repl)=self.getreply()
        if self.debuglevel >0 : print "data:", (code,repl)
        if code <> 354:
            return -1
        else:
            self.send(quotedata(msg))
            self.send("%s.%s" % (CRLF, CRLF))
            (code,msg)=self.getreply()
            if self.debuglevel >0 : print "data:", (code,msg)
            return code

#some useful methods
    def sendmail(self,from_addr,to_addrs,msg,options=[]):
        """ This command performs an entire mail transaction. 
            The arguments are: 
               - from_addr : The address sending this mail.
               - to_addrs :  a list of addresses to send this mail to
               - msg : the message to send. 
               - encoding : list of ESMTP options (such as 8bitmime)

	If there has been no previous EHLO or HELO command this session,
	this method tries ESMTP EHLO first. If the server does ESMTP, message
        size and each of the specified options will be passed to it (if the
        option is in the feature set the server advertises).  If EHLO fails,
        HELO will be tried and ESMTP options suppressed.

        This method will return normally if the mail is accepted for at least 
        one recipient. Otherwise it will throw an exception (either
        SMTPSenderRefused, SMTPRecipientsRefused, or SMTPDataError)
        That is, if this method does not throw an exception, then someone 
        should get your mail.  If this method does not throw an exception,
        it returns a dictionary, with one entry for each recipient that was 
        refused. 

        Example:
      
         >>> import smtplib
         >>> s=smtplib.SMTP("localhost")
         >>> tolist=["one@one.org","two@two.org","three@three.org","four@four.org"]
         >>> msg = '''
         ... From: Me@my.org
         ... Subject: testin'...
         ...
         ... This is a test '''
         >>> s.sendmail("me@my.org",tolist,msg)
         { "three@three.org" : ( 550 ,"User unknown" ) }
         >>> s.quit()
        
         In the above example, the message was accepted for delivery to 
         three of the four addresses, and one was rejected, with the error
         code 550. If all addresses are accepted, then the method
         will return an empty dictionary.  
         """
        if not self.helo_resp and not self.ehlo_resp:
            if self.ehlo() >= 400:
                self.helo()
        if self.esmtp_features:
            self.esmtp_features.append('7bit')
        esmtp_opts = []
        if 'size' in self.esmtp_features:
            esmtp_opts.append("size=" + `len(msg)`)
        for option in options:
            if option in self.esmtp_features:
                esmtp_opts.append(option)
        (code,resp) = self.mail(from_addr, esmtp_opts)
        if code <> 250:
            self.rset()
            raise SMTPSenderRefused
        senderrs={}
        for each in to_addrs:
            (code,resp)=self.rcpt(each)
            if (code <> 250) and (code <> 251):
                senderrs[each]=(code,resp)
        if len(senderrs)==len(to_addrs):
            # the server refused all our recipients
            self.rset()
            raise SMTPRecipientsRefused
        code=self.data(msg)
        if code <>250 :
            self.rset()
            raise SMTPDataError
        #if we got here then somebody got our mail
        return senderrs         


    def close(self):
        """Close the connection to the SMTP server."""
        if self.file:
            self.file.close()
        self.file = None
        if self.sock:
            self.sock.close()
        self.sock = None


    def quit(self):
        """Terminate the SMTP session."""
        self.docmd("quit")
        self.close()

# Test the sendmail method, which tests most of the others.
# Note: This always sends to localhost.
if __name__ == '__main__':
    import sys, rfc822

    def prompt(prompt):
        sys.stdout.write(prompt + ": ")
        return string.strip(sys.stdin.readline())

    fromaddr = prompt("From")
    toaddrs  = string.splitfields(prompt("To"), ',')
    print "Enter message, end with ^D:"
    msg = ''
    while 1:
        line = sys.stdin.readline()
        if not line:
            break
        msg = msg + line
    print "Message length is " + `len(msg)`

    server = SMTP('localhost')
    server.set_debuglevel(1)
    server.sendmail(fromaddr, toaddrs, msg)
    server.quit()


