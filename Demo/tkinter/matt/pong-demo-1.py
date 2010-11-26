from tkinter import *


class Pong(Frame):
    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=LEFT, fill=BOTH)

        ## The playing field
        self.draw = Canvas(self, width="5i", height="5i")

        ## The speed control for the ball
        self.speed = Scale(self, orient=HORIZONTAL, label="ball speed",
                           from_=-100, to=100)

        self.speed.pack(side=BOTTOM, fill=X)

        # The ball
        self.ball = self.draw.create_oval("0i", "0i", "0.10i", "0.10i",
                                          fill="red")
        self.x = 0.05
        self.y = 0.05
        self.velocity_x = 0.3
        self.velocity_y = 0.5

        self.draw.pack(side=LEFT)

    def moveBall(self, *args):
        if (self.x > 5.0) or (self.x < 0.0):
            self.velocity_x = -1.0 * self.velocity_x
        if (self.y > 5.0) or (self.y < 0.0):
            self.velocity_y = -1.0 * self.velocity_y

        deltax = (self.velocity_x * self.speed.get() / 100.0)
        deltay = (self.velocity_y * self.speed.get() / 100.0)
        self.x = self.x + deltax
        self.y = self.y + deltay

        self.draw.move(self.ball,  "%ri" % deltax, "%ri" % deltay)
        self.after(10, self.moveBall)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()
        self.after(10, self.moveBall)


game = Pong()

game.mainloop()
