"""SMTP Client class.

Author: The Dragon De Monsyne <dragondm@integral.org>

(This was modified from the Python 1.5 library HTTP lib.)

This should follow RFC 821. (it dosen't do esmtp (yet))

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
SMTPSenderRefused="Sender address refused"
SMTPRecipientsRefused="All Recipients refused"
SMTPDataError="Error transmoitting message data"

class SMTP:
    """This class manages a connection to an SMTP server."""
    
    def __init__(self, host = '', port = 0):
        """Initialize a new instance.

        If specified, `host' is the name of the remote host to which
        to connect.  If specified, `port' specifies the port to which
        to connect.  By default, smtplib.SMTP_PORT is used.

        """
        self.debuglevel = 0
        self.file = None
	self.helo_resp = None
        if host: self.connect(host, port)
    
    def set_debuglevel(self, debuglevel):
        """Set the debug output level.

        A non-false value results in debug messages for connection and
        for all messages sent to and received from the server.

        """
        self.debuglevel = debuglevel

    def connect(self, host='localhost', port = 0):
        """Connect to a host on a given port.
          
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
        self.sock.send(str)
    
    def putcmd(self, cmd, args=""):
        """Send a command to the server.
        """
        str = '%s %s%s' % (cmd, args, CRLF)
        self.send(str)
    
    def getreply(self):
        """Get a reply from the server.
        
        Returns a tuple consisting of:
        - server response code (e.g. '250', or such, if all goes well)
          Note: returns -1 if it can't read responce code.
        - server response string corresponding to response code
		(note : multiline responces converted to a single, multiline 
                 string)

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
	""" Send a command, and return it's responce code """
	
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

    def help(self):
        """ SMTP 'help' command. Returns help text from server """
	self.putcmd("help")
	(code,msg)=self.getreply()
	return msg

    def rset(self):
        """ SMTP 'rset' command. Resets session. """
	code=self.docmd("rset")
	return code

    def noop(self):
        """ SMTP 'noop' command. Dosen't do anything :> """
	code=self.docmd("noop")
	return code

    def mail(self,sender):
        """ SMTP 'mail' command. Begins mail xfer session. """
        self.putcmd("mail","from: %s" % sender)
	return self.getreply()

    def rcpt(self,recip):
        """ SMTP 'rcpt' command. Indicates 1 recipient for this mail. """
	self.putcmd("rcpt","to: %s" % recip)
	return self.getreply()

    def data(self,msg):
        """ SMTP 'DATA' command. Sends message data to server. 
            Automatically quotes lines beginning with a period per rfc821 """
	#quote periods in msg according to RFC821
        # ps, I don't know why I have to do it this way... doing: 
	# quotepat=re.compile(r"^[.]",re.M)
	# msg=re.sub(quotepat,"..",msg)
        # should work, but it dosen't (it doubles the number of any   
        # contiguous series of .'s at the beginning of a line, 
        # instead of just adding one. )
	quotepat=re.compile(r"^[.]+",re.M)
        def m(pat):
          return "."+pat.group(0)
	msg=re.sub(quotepat,m,msg)
	self.putcmd("data")
	(code,repl)=self.getreply()
	if self.debuglevel >0 : print "data:", (code,repl)
	if code <> 354:
	    return -1
	else:
	    self.send(msg)
	    self.send("\n.\n")
	    (code,msg)=self.getreply()
	    if self.debuglevel >0 : print "data:", (code,msg)
            return code

#some usefull methods
    def sendmail(self,from_addr,to_addrs,msg):
        """ This command performs an entire mail transaction. 
	    The arguments are: 
               - from_addr : The address sending this mail.
               - to_addrs :  a list of addresses to send this mail to
               - msg : the message to send. 

	This method will return normally if the mail is accepted for at
        least one recipiant .Otherwise it will throw an exception (either
        SMTPSenderRefused,SMTPRecipientsRefused, or SMTPDataError)

	That is, if this method does not throw an excception, then someone 
        should get your mail.

  	It returns a dictionary , with one entry for each recipient that 
        was refused. 

        example:
      
         >>> import smtplib
         >>> s=smtplib.SMTP("localhost")
         >>> tolist= [ "one@one.org",
         ...           "two@two.org",
         ...           "three@three.org",
         ...           "four@four.org"]
         >>> msg = '''
         ... From: Me@my.org
         ... Subject: testin'...
         ...
         ... This is a test '''
         >>> s.sendmail("me@my.org",tolist,msg)
         { "three@three.org" : ( 550 ,"User unknown" ) }
         >>> s.quit()
        
         In the above example, the message was accepted for delivery to three 
         of the four addresses, and one was rejected, with the error code 550.
         If all addresses are accepted, then the method will return an
         empty dictionary.  
         """

	if not self.helo_resp:
	    self.helo()
        (code,resp)=self.mail(from_addr)
        if code <>250:
	    self.rset()
	    raise SMTPSenderRefused
	senderrs={}
        for each in to_addrs:
	    (code,resp)=self.rcpt(each)
	    if (code <> 250) and (code <> 251):
                senderr[each]=(code,resp)
        if len(senderrs)==len(to_addrs):
	    #th' server refused all our recipients
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
	self.docmd("quit")
	self.close()
