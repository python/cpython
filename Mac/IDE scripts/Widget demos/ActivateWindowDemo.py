import W

# this demoscript illustrates how to tie a callback to activating or clicking away of the window.
# evb 22 4 99

class ActivateDemo:

    def __init__(self):
        self.w = W.Window((200, 200), 'Activate demo')
        self.w.bind("<activate>", self.my_activate_callback)
        self.w.open()

    def my_activate_callback(self, onoff):
        '''the callback gets 1 parameter which indicates whether the window
        has been activated (1) or clicked to the back (0)'''
        if onoff == 1:
            print "I'm in the front now!"
        else:
            print "I've been clicked away, Oh No!"

ad = ActivateDemo()
