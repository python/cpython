#! /usr/local/bin/python

testlabels = 'Name', 'Address', 'City', 'Country', 'Comments'

def main():
	import stdwin
	from WindowParent import WindowParent, MainLoop
	from FormSplit import FormSplit
	from Buttons import Label
	from TextEdit import TextEdit
	#
	stdwin.setdefscrollbars(0, 0)
	#
	w = WindowParent().create('FormTest', (0, 0))
	f = FormSplit().create(w)
	#
	h, v = 0, 0
	for label in testlabels:
		f.placenext(h, v)
		lbl = Label().definetext(f, label)
		f.placenext(h + 100, v)
		txt = TextEdit().createboxed(f, (40, 2), (2, 2))
		#txt = TextEdit().create(f, (40, 2))
		v = v + 2*stdwin.lineheight() + 10
	#
	w.realize()
	#
	MainLoop()

main()
