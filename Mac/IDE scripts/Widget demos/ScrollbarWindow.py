import W

# make a non-sizable window
#window = W.Window((200, 200), "Fixed Size")

#  make a sizable window
window = W.Window((200, 300), "Variable Size!", minsize = (200, 200))

# make some edit text widgets
# a scrollbar
window.hbar = W.Scrollbar((-1, -15, -14, 16), max = 100)
window.vbar = W.Scrollbar((-15, -1, 16, -14), max = 100)
#window.vbar = W.Scrollbar((-15, -1, 1, -14), max = 100)


# open the window
window.open()
