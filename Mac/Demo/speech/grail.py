# Pass this program the Holy Grail script on stdin.
import sys
import string
import stdwin
from stdwinevents import *

try:
	import macspeech
except ImportError:
	macspeech = None

WINWIDTH = 1000
scrw, scrh = stdwin.getscrsize()
if WINWIDTH > 0.8*scrw:
	WINWIDTH = int(0.8*scrw)
BLACK = stdwin.fetchcolor('black')
RED = stdwin.fetchcolor('red')
BLUE = stdwin.fetchcolor('blue')

done='done'

class MacSpeaker:
	def __init__(self):
		self.voices = []
		self.nvoices = macspeech.CountVoices()
		self.curvoice = 1
		self.rate = 1.0
		
	def _newvoice(self):
		vd = macspeech.GetIndVoice(self.curvoice)
		sc = vd.NewChannel()
		self.curvoice = self.curvoice + 1
		if self.curvoice > self.nvoices:
			self.curvoice = 1
		return sc
		
	def newvoices(self, n):
		self.voices = []
		for i in range(n):
			self.voices.append(self._newvoice())
		if self.rate <> 1.0:
			self.setrate(1.0)
			
	def setrate(self, factor):
		self.rate = self.rate*factor
		for v in self.voices:
			r = v.GetRate()
			v.SetRate(r*factor)
					
	def speak(self, i, text):
		self.voices[i-1].SpeakText(text)
		
	def busy(self):
		return macspeech.Busy()

[NOTHING, NEWSCENE, ACT, TEXT, MORETEXT] = range(5)
def parseline(line):
	stripline = string.strip(line)
	if not stripline:
		return NOTHING, ''
	if stripline[:5] == 'Scene':
		return NEWSCENE, stripline
	if line[0] == '[':
		return ACT, stripline
	if line[0] == ' ' and ':' in line:
		splitline = string.splitfields(stripline, ':')
		stripline = string.joinfields(splitline[1:], ':')
		return TEXT, (splitline[0], string.strip(stripline))
	return MORETEXT, stripline

def readscript(file):
	lines = file.readlines()
	acts = []
	actor_dict = {}
	longest = 0
	prev_act = 0
	for i in range(len(lines)):
		tp, data = parseline(lines[i])
		if tp == NEWSCENE:
			acts.append(actor_dict.keys(), lines[prev_act:i])
			prev_act = i
			actor_dict = {}
		elif tp == TEXT:
			actor_dict[data[0]] = 1
		lines[i] = tp, data
	return acts[1:]

class Main:
	def __init__(self):
		if macspeech:
			self.speaker = MacSpeaker()
		else:
			self.speaker = None
		sys.stdin = open('SCRIPT', 'r')
		self.acts = readscript(sys.stdin)
		maxactor = 0
		for actorlist, actdata in self.acts:
			if len(actorlist) > maxactor:
				maxactor = len(actorlist)
		if not self.loadnextact():
			print 'No acts to play!'
			raise done
		self.lh = stdwin.lineheight()
		self.winheight = (maxactor+2)*self.lh
		stdwin.setdefwinsize(WINWIDTH, self.winheight)
		self.win = stdwin.open('The Play')
		self.win.setdocsize(WINWIDTH, self.winheight)
		self.win.change(((0,0),(WINWIDTH, self.winheight)))
		self.menu = self.win.menucreate('Play')
		self.menu.additem('Faster', '+')
		self.menu.additem('Slower', '-')
		self.menu.additem('Quit', 'Q')
		self.speed = 4
		
	def done(self):
		del self.win
		del self.menu

	def loadnextact(self):
		if not self.acts: return 0
		actors, lines = self.acts[0]
		del self.acts[0]
		prevactor = 0
		for i in range(len(lines)):
			tp, data = lines[i]
			if tp == NOTHING:
				continue
			elif tp in (NEWSCENE, ACT):
				lines[i] = 0, data
			elif tp == TEXT:
				prevactor = actors.index(data[0])
				lines[i] = prevactor+1, data[1]
			else:
				lines[i] = prevactor+1, data
		self.lines = lines
		self.actors = [''] + actors
		self.actorlines = [''] * len(self.actors)
		if self.speaker:
			self.speaker.newvoices(len(self.actors)-1)
		self.prevline = 0
		self.actwidth = 0
		for a in self.actors:
			w = stdwin.textwidth(a)
			if w > self.actwidth:
				self.actwidth = w
		return 1

	def loadnextline(self):
		if not self.lines: return 0
		self.actorlines[self.prevline] = ''
		top = self.lh*self.prevline
		self.win.change(((0, top), (WINWIDTH, top+self.lh)))
		line, data = self.lines[0]
		del self.lines[0]
		self.actorlines[line] = data
		self.prevline = line
		top = self.lh*self.prevline
		self.win.change(((0, top), (WINWIDTH, top+self.lh)))
		if line == 0:
			self.win.settimer(5*self.speed)
		else:
			if self.speaker:
				self.speaker.speak(line, data)
				tv = 1
			else:
				nwords = len(string.split(data))
				tv = self.speed*(nwords+1)
			self.win.settimer(tv)
		return 1

	def timerevent(self):
		if self.speaker and self.speaker.busy():
			self.win.settimer(1)
			return
		while 1:
			if self.loadnextline(): return
			if not self.loadnextact():
				stdwin.message('The END')
				self.win.close()
				raise done
			self.win.change(((0,0), (WINWIDTH, self.winheight)))

	def redraw(self, top, bottom, draw):
		for i in range(len(self.actors)):
			tpos = i*self.lh
			bpos = (i+1)*self.lh-1
			if tpos < bottom and bpos > top:
				draw.setfgcolor(BLUE)
				draw.text((0, tpos), self.actors[i])
				if i == 0:
					draw.setfgcolor(RED)
				else:
					draw.setfgcolor(BLACK)
				draw.text((self.actwidth+5, tpos), self.actorlines[i])

	def run(self):
		self.win.settimer(10)
		while 1:
			ev, win, arg = stdwin.getevent()
			if ev == WE_DRAW:
				((left, top), (right, bot)) = arg
				self.redraw(top, bot, self.win.begindrawing())
			elif ev == WE_TIMER:
				self.timerevent()
			elif ev == WE_CLOSE:
				self.win.close()
				raise done
			elif ev == WE_MENU and arg[0] == self.menu:
				if arg[1] == 0:
					if self.speed > 1:
						self.speed = self.speed/2
						if self.speaker:
							self.speaker.setrate(1.4)
				elif arg[1] == 1:
					self.speed = self.speed * 2
					if self.speaker:
						self.speaker.setrate(0.7)
				elif arg[1] == 2:
					self.win.close()
					raise done

if 1:
	main = Main()
	try:
		main.run()
	except done:
		pass
	del main
