#!/usr/bin/python
"""SMTP/ESMTP client class.

Author: The Dragon De Monsyne <dragondm@integral.org>
ESMTP support, test code and doc fixes added by
    Eric S. Raymond <esr@thyrsus.com>
Better RFC 821 compliance (MAIL and RCPT, and CRLF in data)
    by Carey Evans <c.evans@clear.net.nz>, for picky mail servers.
   
(This was modified from the Python 1.5 library HTTP lib.)

This should follow RFC 821 (SMTP) and RFC 1869 (ESMTP).

Notes:

Please remember, when doing ESMTP, that the names of the SMTP service
extensions are NOT the same thing as the option keyords for the RCPT
and MAIL commands!

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
import string, re
import rfc822
import types

SMTP_PORT = 25
CRLF="\r\n"

# used for exceptions 
SMTPServerDisconnected="Server not connected"
SMTPSenderRefused="Sender address refused"
SMTPRecipientsRefused="All Recipients refused"
SMTPDataError="Error transmitting message data"

def quoteaddr(addr):
    """Quote a subset of the email addresses defined by RFC 821.

    Should be able to handle anything rfc822.parseaddr can handle."""

    m=None
    try:
        m=rfc822.parseaddr(addr)[1]
    except AttributeError:
        pass
    if not m:
        #something weird here.. punt -ddm
        return addr
    else:
        return "<%s>" % m

def quotedata(data):
    """Quote data for email.

    Double leading '.', and change Unix newline '\n', or Mac '\r' into
    Internet CRLF end-of-line."""
    return re.sub(r'(?m)^\.', '..',
        re.sub(r'(?:\r\n|\n|\r(?!\n))', CRLF, data))

class SMTP:
    """This class manages a connection to an SMTP or ESMTP server.
    SMTP Objects:
        SMTP objects have the following attributes:    
            helo_resp 
                This is the message given by the server in responce to the 
                most recent HELO command.
                
            ehlo_resp
                This is the message given by the server in responce to the 
                most recent EHLO command. This is usually multiline.

            does_esmtp 
                This is a True value _after you do an EHLO command_, if the
                server supports ESMTP.

            esmtp_features 
                This is a dictionary, which, if the server supports ESMTP,
                will  _after you do an EHLO command_, contain the names of the
                SMTP service  extentions this server supports, and their 
                parameters (if any).
                Note, all extention names are mapped to lower case in the 
                dictionary. 

        For method docs, see each method's docstrings. In general, there is 
            a method of the same name to preform each SMTP comand, and there 
            is a method called 'sendmail' that will do an entiere mail 
            transaction."""

    debuglevel = 0
    file = None
    helo_resp = None
    ehlo_resp = None
    does_esmtp = 0

    def __init__(self, host = '', port = 0):
        """Initialize a new instance.

        If specified, `host' is the name of the remote host to which
        to connect.  If specified, `port' specifies the port to which
        to connect.  By default, smtplib.SMTP_PORT is used.

        """
        self.esmtp_features = {}
        if host: self.connect(host, port)
    
    def set_debuglevel(self, debuglevel):
        """Set the debug output level.

        A non-false value results in debug messages for connection and
        for all messages sent to and received from the server.

        """
        self.debuglevel = debuglevel

    def connect(self, host='localhost', port = 0):
        """Connect to a host on a given port.

        If the hostname ends with a colon (`:') followed by a number,
        and there is no port specified,  that suffix will be stripped 
        off and the number interpreted as the port number to use.

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
            try:
                self.sock.send(str)
            except socket.error:
                raise SMTPServerDisconnected
        else:
            raise SMTPServerDisconnected
 
    def putcmd(self, cmd, args=""):
        """Send a command to the server.
        """
        str = '%s %s%s' % (cmd, args, CRLF)
        self.send(str)
    
    def getreply(self):
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
        defaults to the FQDN of the local host.  """
        name=string.strip(name)
        if len(name)==0:
                name=socket.gethostbyaddr(socket.gethostname())[0]
        self.putcmd("ehlo",name)
        (code,msg)=self.getreply()
        # According to RFC1869 some (badly written) 
        # MTA's will disconnect on an ehlo. Toss an exception if 
        # that happens -ddm
        if code == -1 and len(msg) == 0:
            raise SMTPServerDisconnected
        self.ehlo_resp=msg
        if code<>250:
            return code
        self.does_esmtp=1
        #parse the ehlo responce -ddm
        resp=string.split(self.ehlo_resp,'\n')
        del resp[0]
        for each in resp:
            m=re.match(r'(?P<feature>[A-Za-z0-9][A-Za-z0-9\-]*)',each)
            if m:
                feature=string.lower(m.group("feature"))
                params=string.strip(m.string[m.end("feature"):])
                self.esmtp_features[feature]=params
        return code

    def has_extn(self, opt):
        """Does the server support a given SMTP service extension?"""
        return self.esmtp_features.has_key(string.lower(opt))

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
        optionlist = ''
        if options and self.does_esmtp:
            optionlist = string.join(options, ' ')
        self.putcmd("mail", "FROM:%s %s" % (quoteaddr(sender) ,optionlist))
        return self.getreply()

    def rcpt(self,recip,options=[]):
        """ SMTP 'rcpt' command. Indicates 1 recipient for this mail. """
        optionlist = ''
        if options and self.does_esmtp:
            optionlist = string.join(options, ' ')
        self.putcmd("rcpt","TO:%s %s" % (quoteaddr(recip),optionlist))
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

    def vrfy(self, address):
        return self.verify(address)

    def verify(self, address):
        """ SMTP 'verify' command. Checks for address validity. """
        self.putcmd("vrfy", quoteaddr(address))
        return self.getreply()

    def expn(self, address):
        """ SMTP 'verify' command. Checks for address validity. """
        self.putcmd("expn", quoteaddr(address))
        return self.getreply()


#some useful methods
    def sendmail(self, from_addr, to_addrs, msg, mail_options=[],
                 rcpt_options=[]): 
        """ This command performs an entire mail transaction. 
            The arguments are: 
               - from_addr : The address sending this mail.
               - to_addrs :  a list of addresses to send this mail to
                         (a string will be treated as a list with 1 address)
               - msg : the message to send. 
               - mail_options : list of ESMTP options (such as 8bitmime)
                                for the mail command
               - rcpt_options : List of ESMTP options (such as DSN commands)
                               for all the rcpt commands
        If there has been no previous EHLO or HELO command this session,
        this method tries ESMTP EHLO first. If the server does ESMTP, message
        size and each of the specified options will be passed to it.  
        If EHLO fails, HELO will be tried and ESMTP options suppressed.

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
        esmtp_opts = []
        if self.does_esmtp:
            # Hmmm? what's this? -ddm
            # self.esmtp_features['7bit']=""
            if self.has_extn('size'):
                esmtp_opts.append("size=" + `len(msg)`)
            for option in mail_options:
                esmtp_opts.append(option)

        (code,resp) = self.mail(from_addr, esmtp_opts)
        if code <> 250:
            self.rset()
            raise SMTPSenderRefused
        senderrs={}
        if type(to_addrs) == types.StringType:
            to_addrs = [to_addrs]
        for each in to_addrs:
            (code,resp)=self.rcpt(each, rcpt_options)
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
