# Script (applet) to run any Python command

def main():
	import sys
	sys.stdout = sys.stderr
	del sys.argv[:1]
	if not sys.argv:
		import macfs
		srcfss, ok = macfs.StandardGetFile('TEXT')
		if not ok:
			return
		filename = srcfss.as_pathname()
		sys.argv.append(filename)
	import __main__
	try:
		execfile(sys.argv[0], __main__.__dict__)
	except SystemExit, msg:
		if msg:
			message("Exit status: %s" % str(msg))
		print "exit", `msg`
		#sys.exit(msg)
	except:
		etype = sys.exc_type
		if hasattr(etype, "__name__"): etype = etype.__name__
		message("%s: %s" % (etype, sys.exc_value))
		print "exit 1"
		#sys.exit(1)

def message(str = "Hello, world!", id = 256):
	import Dlg
	d = Dlg.GetNewDialog(id, -1)
	if not d:
		print str
		return
	tp, h, rect = d.GetDItem(2)
	Dlg.SetIText(h, str)
	while 1:
		n = Dlg.ModalDialog(None)
		if n == 1: break

main()
