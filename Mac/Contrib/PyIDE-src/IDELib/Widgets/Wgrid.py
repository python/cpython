"""grid utility for widgets"""

class Grid:
	
	def __init__(self, 	ncol = None, 
				minncol = None,
				maxncol = None,
				colwidth = None, 
				mincolwidth = None, 
				maxcolwidth = None, 
				width = None, 
				minwidth = None, 
				maxwidth = None, 
				vgrid = 8, 
				gutter = 10,
				leftmargin = None,
				rightmargin = None,
				topmargin = None,
				bottommargin = None
				):
		if leftmargin == None:
			leftmargin = gutter
		if rightmargin == None:
			rightmargin = gutter
		if topmargin == None:
			topmargin = vgrid
		if bottommargin == None:
			bottommargin = vgrid
	
	def getbounds(self, width, height, bounds):
		xxx


