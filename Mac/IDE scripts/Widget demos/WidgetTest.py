import W

# define some callbacks
def callback():
	window.close()

def checkcallback(value):
	print "hit the checkbox", value

def radiocallback(value):
	print "hit radiobutton #3", value

def scrollcallback(value):
	widget = window.hbar
	if value == "+":
		widget.set(widget.get() - 1)
	elif value == "-":
		widget.set(widget.get() + 1)
	elif value == "++":
		widget.set(widget.get() - 10)
	elif value == "--":
		widget.set(widget.get() + 10)
	else:   # in thumb
		widget.set(value)
	print "scroll...", widget.get()

def textcallback():
	window.et3.set(window.et1.get())

def cancel():
	import EasyDialogs
	EasyDialogs.Message("Cancel!")

# make a non-sizable window
#window = W.Window((200, 300), "Fixed Size")

#  make a sizable window
window = W.Window((200, 300), "Variable Size!", minsize = (200, 300))

# make some edit text widgets
window.et1 = W.EditText((10, 10, 110, 110), "Hallo!", textcallback)
window.et2 = W.EditText((130, 40, 60, 30), "one!")
window.et3 = W.EditText((130, 80, -10, 40), "two?")

# a button
window.button = W.Button((-70, 10, 60, 16), "Close", callback)

# a checkbox
window.ch = W.CheckBox((10, 130, 160, 16), "Check (command \xa4)", checkcallback)

# set of radio buttons (should become easier/nicer)
thebuttons = []
window.r1 = W.RadioButton((10, 150, 180, 16), "Radio 1 (cmd 1)", thebuttons)
window.r2 = W.RadioButton((10, 170, 180, 16), "Radio 2 (cmd 2)", thebuttons)
window.r3 = W.RadioButton((10, 190, 180, 16), "Radio 3 (cmd 3)", thebuttons, radiocallback)
window.r1.set(1)

# a normal button
window.cancelbutton = W.Button((10, 220, 60, 16), "Cancel", cancel)

# a scrollbar
window.hbar = W.Scrollbar((-1, -15, -14, 16), scrollcallback, max = 100)

# some static text
window.static = W.TextBox((10, 260, 110, 16), "Schtatic")

# bind some keystrokes to functions
window.bind('cmd\xa4', window.ch.push)
window.bind('cmd1', window.r1.push)
window.bind('cmd2', window.r2.push)
window.bind('cmd3', window.r3.push)
window.bind('cmdw', window.button.push)
window.bind('cmd.', window.cancelbutton.push)

window.setdefaultbutton(window.button)
# open the window
window.open()

if 0:
	import time
	for i in range(20):
		window.et2.set(`i`)
		#window.et2.SetPort()
		#window.et2.draw()
		time.sleep(0.1)
