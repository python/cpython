import Printing
import Qd
import Fm
import Res

# some constants
PostScriptBegin = 190	# Set driver state to PostScript	
PostScriptEnd = 191	# Restore QuickDraw state	
PostScriptHandle = 192	# PostScript data referenced in handle

CHUNK_SIZE = 0x8000 # max size of PicComment

def PostScript(text):
	"""embed text as plain PostScript in print job."""
	handle = Res.Resource('')
	Qd.PicComment(PostScriptBegin, 0, handle)
	while text:
		chunk = text[:CHUNK_SIZE]
		text = text[CHUNK_SIZE:]
		handle.data = chunk
		Qd.PicComment(PostScriptHandle, len(chunk), handle)
	handle.data = ''
	Qd.PicComment(PostScriptEnd, 0, handle)

# create a new print record
printrecord = Printing.NewTPrintRecord()

# open the printer
Printing.PrOpen()
try:
	# initialize print record with default values
	Printing.PrintDefault(printrecord)
	
	# page setup, ok is 0 when user cancelled
	ok = Printing.PrStlDialog(printrecord)
	if not ok:
		raise KeyboardInterrupt
	# at this stage, you should save the print record in your document for later
	# reference. 
	
	# print job dialog, ok is 0 when user cancelled
	ok = Printing.PrJobDialog(printrecord)
	if not ok:
		raise KeyboardInterrupt
	
	# once per document
	port = Printing.PrOpenDoc(printrecord)
	# port is the Printer's GrafPort, it is also the current port, so no need to Qd.SetPort(port)
	try:
		# start printing a page
		# XXX should really look up what pages to print by
		# inspecting the print record.
		Printing.PrOpenPage(port, None)
		try:
			# use QuickDraw like in any other GrafPort
			Qd.FrameRect((10, 250, 100, 500))
			Qd.FrameRect((10, 510, 100, 600))
			Qd.MoveTo(10, 100)
			Qd.TextSize(50)
			Qd.TextFont(Fm.GetFNum("Helvetica"))
			Qd.DrawString("It rreally works!")
			Qd.MoveTo(10, 150)
			Qd.TextSize(20)
			Qd.DrawString("(and now for a little PostScript...)")
			
			# example PostScript code
			ps = """
				% the coordinate system is the quickdraw one, which is flipped
				% compared to the default PS one. That means text will appear
				% flipped when used directly from PostScript. 
				% As an example we start by defining a custom scalefont operator 
				% that corrects this. 
				/myscalefont{[exch 0 0 2 index neg 0 0]makefont}def
				0.75 setgray
				0 0 moveto
				0 30 lineto 10000 30 lineto
				10000 0 lineto closepath fill
				0 setgray
				5 25 moveto /Courier findfont 20 myscalefont setfont
				(Printed with PostScript!) show
				2 setlinewidth [10 10 5 10] 0 setdash 5 5 moveto 400 0 rlineto stroke
				"""
			# embed the PostScript code in the print job
			PostScript(ps)
		finally:
			# when done with the page
			Printing.PrClosePage(port)
	finally:
		# when done with the document
		Printing.PrCloseDoc(port)
finally:
	# when done printing
	Printing.PrClose()
