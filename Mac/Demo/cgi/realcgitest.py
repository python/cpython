"""cgitest - A minimal CGI applet. Echos parameters back to the client.
"""

from MiniAEFrame import AEServer, MiniApplication

class CGITest(AEServer, MiniApplication):
	
	def __init__(self):
		MiniApplication.__init__(self)
		AEServer.__init__(self)
		self.installaehandler('aevt', 'oapp', self.open_app)
		self.installaehandler('aevt', 'quit', self.quit)
		self.installaehandler('WWW\275', 'sdoc', self.cgihandler)
		oldparams = MacOS.SchedParams(0, 0)
		self.mainloop()
		apply(MacOS.SchedParams, oldparams)

	def quit(self, **args):
		self.quitting = 1
		
	def open_app(self, **args):
		pass
				
	def cgihandler(self, pathargs, **args):
		rv = """HTTP/1.0 200 OK
Server: NetPresenz; python-cgi-script
MIME-Version: 1.0
Content-type: text/html

<title>Python CGI-script results</title>
<h1>Python CGI-script results</h1>
<hr>
"""
		rv = rv+'<br><b>Direct object:</b> %s\n'%pathargs
		
		for key in args.keys():
			if key[0] != '_':
				rv = rv + '<br><b>%s:</b> %s\n'%(key, args[key])
		rv = rv +'<hr>\nSee you next time!\n'
		
		# Note: if you want to quit after each request enable the line
		# self.quitting = 1
		
		return rv

if __name__ == '__main__':
	CGITest()
