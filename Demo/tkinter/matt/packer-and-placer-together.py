from Tkinter import *

# This is a program that tests the placer geom manager in conjunction with 
# the packer. The background (green) is packed, while the widget inside is placed


def do_motion(event):
    app.button.place(x=event.x, y=event.y)

def dothis():
    print 'calling me!'

def createWidgets(top):
    # make a frame. Note that the widget is 200 x 200
    # and the window containing is 400x400. We do this
    # simply to show that this is possible. The rest of the
    # area is inaccesssible.
    f = Frame(top, width=200, height=200, background='green')

    # note that we use a different manager here. 
    # This way, the top level frame widget resizes when the 
    # application window does. 
    f.pack(fill=BOTH, expand=1)

    # now make a button
    f.button = Button(f, foreground='red', text='amazing', command=dothis)

    # and place it so that the nw corner is 
    # 1/2 way along the top X edge of its' parent
    f.button.place(relx=0.5, rely=0.0, anchor=NW)

    # allow the user to move the button SUIT-style.
    f.bind('<Control-Shift-Motion>', do_motion)

    return f

root = Tk()
app = createWidgets(root)
root.geometry("400x400")
root.maxsize(1000, 1000)
root.mainloop()

