import W

def twothird(width, height):
    return (8, 8, width - 8, 2*height/3 - 4)

def onethird(width, height):
    return (8, 2*height/3 + 4, width - 8, height - 22)

def halfbounds1(width, height):
    return (0, 0, width/2 - 4, height)

def halfbounds2(width, height):
    return (width/2 + 4, 0, width, height)

window = W.Window((400, 400), "Sizable window with two lists", minsize = (200, 200))

window.listgroup = W.Group(twothird)
window.listgroup.list1 = W.List(halfbounds1, range(13213, 13310))
window.listgroup.list2 = W.List(halfbounds2, range(800, 830))
window.et = W.EditText(onethird, "Wat nu weer?")

window.open()
