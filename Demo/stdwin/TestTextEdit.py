#! /usr/local/bin/python

# Test TextEdit widgets

def main():
	from TextEdit import TextEdit
	from WindowParent import WindowParent, MainLoop
	w = WindowParent().create('Test TextEdit', (0, 0))
	t = TextEdit().create(w, (40, 4))
	w.realize()
	MainLoop()

main()
