#!/ufs/guido/bin/sgi/tkpython
# Simulate "electrons" migrating across the screen.  
# An optional bitmap file in can be in the background.
#
# Usage: electrons [n [bitmapfile]]
#
# n is the number of electrons to animate; default is 4, maximum 15.
#
# The bitmap file can be any X11 bitmap file (look in
# /usr/include/X11/bitmaps for samples); it is displayed as the
# background of the animation.  Default is no bitmap.

# This uses Steen Lumholt's Tk interface
from Tkinter import *



# The graphical interface
class Electrons:

	# Create our objects
	def __init__(self, n, bitmap = None):
		self.n = n
		self.tk = tk = Tk()
		self.canvas = c = Canvas(tk)
		c.pack()
		width, height = tk.getint(c['width']), tk.getint(c['height'])

		# Add background bitmap
		if bitmap:
			self.bitmap = c.create_bitmap(width/2, height/2,
						      {'bitmap': bitmap,
						       'foreground': 'blue'})

		self.pieces = {}
		x1, y1, x2, y2 = 10,70,14,74
		for i in range(n,0,-1):
			p = c.create_oval(x1, y1, x2, y2,
					       {'fill': 'red'})
			self.pieces[i] = p
			y1, y2 = y1 +2, y2 + 2
		self.tk.update()

	def random_move(self,n):
		for i in range(1,n+1):
			p = self.pieces[i]
			c = self.canvas
			import rand
			x = rand.choice(range(-2,4))
			y = rand.choice(range(-3,4))
			c.move(p, x, y)
		self.tk.update()
	# Run -- never returns
	def run(self):
		while 1:
			self.random_move(self.n)
		self.tk.mainloop() # Hang around...

# Main program
def main():
	import sys, string

	# First argument is number of pegs, default 4
	if sys.argv[1:]:
		n = string.atoi(sys.argv[1])
	else:
		n = 30

	# Second argument is bitmap file, default none
	if sys.argv[2:]:
		bitmap = sys.argv[2]
		# Reverse meaning of leading '@' compared to Tk
		if bitmap[0] == '@': bitmap = bitmap[1:]
		else: bitmap = '@' + bitmap
	else:
		bitmap = None

	# Create the graphical objects...
	h = Electrons(n, bitmap)

	# ...and run!
	h.run()


# Call main when run as script
if __name__ == '__main__':
	main()


