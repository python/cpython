"""Simple W demo -- shows how to make a window, and bind a function to a "key" event."""

import W

# key callback function
def tester(char, event):
    text = "%r\r%d\r%s\r%s" % (char, ord(char), hex(ord(chart)), oct(ord(char)))
    window.keys.set(text)

# close callback
def close():
    window.close()

# new window
window = W.Dialog((180, 100), "Type a character")

# make a frame (a simple rectangle)
window.frame = W.Frame((5, 5, -5, -33))

# some labels, static text
window.captions = W.TextBox((10, 9, 43, -36), "char:\rdecimal:\rhex:\roctal:")

# another static text box
window.keys = W.TextBox((60, 9, 40, -36))

# a button
window.button = W.Button((-69, -24, 60, 16), "Done", close)

# bind the callbacks
window.bind("<key>", tester)
window.bind("cmdw", window.button.push)

# open the window
window.open()
