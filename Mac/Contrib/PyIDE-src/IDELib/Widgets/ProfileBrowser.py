import W
import Evt

import sys
import StringIO
import string

def timer(TickCount = Evt.TickCount):
	return TickCount() / 60.0

class ProfileBrowser:
	
	def __init__(self, stats = None):
		self.sortkeys = ('calls',)
		self.setupwidgets()
		self.setstats(stats)
	
	def setupwidgets(self): 
		self.w = W.Window((580, 400), "Profile Statistics", minsize = (200, 100), tabbable = 0)
		self.w.divline = W.HorizontalLine((0, 20, 0, 0))
		self.w.titlebar = W.TextBox((4, 4, 40, 12), 'Sort by:')
		self.buttons = []
		self.w.button_calls = W.RadioButton((54, 4, 45, 12), 'calls', self.buttons, self.setsort)
		self.w.button_time = W.RadioButton((104, 4, 40, 12), 'time', self.buttons, self.setsort)
		self.w.button_cumulative = W.RadioButton((154, 4, 75, 12), 'cumulative', self.buttons, self.setsort)
		self.w.button_stdname = W.RadioButton((234, 4, 60, 12), 'stdname', self.buttons, self.setsort)
		self.w.button_calls.set(1)
		self.w.button_file = W.RadioButton((304, 4, 40, 12), 'file', self.buttons, self.setsort)
		self.w.button_line = W.RadioButton((354, 4, 50, 12), 'line', self.buttons, self.setsort)
		self.w.button_module = W.RadioButton((404, 4, 55, 12), 'module', self.buttons, self.setsort)
		self.w.button_name = W.RadioButton((464, 4, 50, 12), 'name', self.buttons, self.setsort)
##		self.w.button_nfl = W.RadioButton((4, 4, 12, 12), 'nfl', self.buttons, self.setsort)
##		self.w.button_pcalls = W.RadioButton((4, 4, 12, 12), 'pcalls', self.buttons, self.setsort)
		self.w.text = W.TextEditor((0, 21, -15, -15), inset = (6, 5), 
				readonly = 1, wrap = 0, fontsettings = ('Monaco', 0, 9, (0, 0, 0)))
		self.w._bary = W.Scrollbar((-15, 20, 16, -14), self.w.text.vscroll, max = 32767)
		self.w._barx = W.Scrollbar((-1, -15, -14, 16), self.w.text.hscroll, max = 32767)
		self.w.open()
	
	def setstats(self, stats):
		self.stats = stats
		self.stats.strip_dirs()
		self.displaystats()
	
	def setsort(self):
		# Grmpf. The callback doesn't give us the button:-(
		for b in self.buttons:
			if b.get():
				if b._title == self.sortkeys[0]:
					return
				self.sortkeys = (b._title,) + self.sortkeys[:3]
				break
		self.displaystats()
	
	def displaystats(self):
		W.SetCursor('watch')
		apply(self.stats.sort_stats, self.sortkeys)
		saveout = sys.stdout
		try:
			s = sys.stdout = StringIO.StringIO()
			self.stats.print_stats()
		finally:
			sys.stdout = saveout
		text = string.join(string.split(s.getvalue(), '\n'), '\r')
		self.w.text.set(text)


def main():
	import pstats
	args = sys.argv[1:]
	for i in args:
		stats = pstats.Stats(i)
		browser = ProfileBrowser(stats)
	else:
		import macfs
		fss, ok = macfs.PromptGetFile('Profiler data')
		if not ok: sys.exit(0)
		stats = pstats.Stats(fss.as_pathname())
		browser = ProfileBrowser(stats)

if __name__ == '__main__':
	main()
