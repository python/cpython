import W

def listhit(isdbl):
    if isdbl:
        print "double-click in list!"
    else:
        print "click in list."

window = W.Window((200, 400), "Window with List", minsize = (150, 200))

window.list = W.List((-1, 20, 1, -14), [], listhit)

# or (equivalent):
# window.list = W.List((-1, 20, 1, -14), callback = listhit)

window.list.set(range(13213, 13350))
window.open()
