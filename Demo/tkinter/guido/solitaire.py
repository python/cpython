#! /usr/bin/env python

"""Solitaire game, much like the one that comes with MS Windows.

Limitations:

- No cute graphical images for the playing cards faces or backs.
- No scoring or timer.
- No undo.
- No option to turn 3 cards at a time.
- No keyboard shortcuts.
- Less fancy animation when you win.
- The determination of which stack you drag to is more relaxed.

Bugs:

- When you double-click a card on a temp stack to move it to the suit
stack, if the next card is face down, you have to wait until the
double-click time-out expires before you can click it to turn it.
I think this has to do with Tk's multiple-click detection, which means
it's hard to work around.
  
Apology:

I'm not much of a card player, so my terminology in these comments may
at times be a little unusual.  If you have suggestions, please let me
know!

"""

# Imports

import math
import random

from Tkinter import *
from Canvas import Rectangle, CanvasText, Group


# Fix a bug in Canvas.Group as distributed in Python 1.4.  The
# distributed bind() method is broken.  This is what should be used:

class Group(Group):
    def bind(self, sequence=None, command=None):
	return self.canvas.tag_bind(self.id, sequence, command)


# Constants determining the size and lay-out of cards and stacks.  We
# work in a "grid" where each card/stack is surrounded by MARGIN
# pixels of space on each side, so adjacent stacks are separated by
# 2*MARGIN pixels.

CARDWIDTH = 100
CARDHEIGHT = 150
MARGIN = 10
XSPACING = CARDWIDTH + 2*MARGIN
YSPACING = CARDHEIGHT + 4*MARGIN
OFFSET = 5

# The background color, green to look like a playing table.  The
# standard green is way too bright, and dark green is way to dark, so
# we use something in between.  (There are a few more colors that
# could be customized, but they are less controversial.)

BACKGROUND = '#070'


# Suits and colors.  The values of the symbolic suit names are the
# strings used to display them (you change these and VALNAMES to
# internationalize the game).  The COLOR dictionary maps suit names to
# colors (red and black) which must be Tk color names.  The keys() of
# the COLOR dictionary conveniently provides us with a list of all
# suits (in arbitrary order).

HEARTS = 'Heart'
DIAMONDS = 'Diamond'
CLUBS = 'Club'
SPADES = 'Spade'

RED = 'red'
BLACK = 'black'

COLOR = {}
for s in (HEARTS, DIAMONDS):
    COLOR[s] = RED
for s in (CLUBS, SPADES):
    COLOR[s] = BLACK

ALLSUITS = COLOR.keys()
NSUITS = len(ALLSUITS)


# Card values are 1-13, with symbolic names for the picture cards.
# ALLVALUES is a list of all card values.

ACE = 1
JACK = 11
QUEEN = 12
KING = 13
ALLVALUES = range(1, 14) # (one more than the highest value)


# VALNAMES is a list that maps a card value to string.  It contains a
# dummy element at index 0 so it can be indexed directly with the card
# value.

VALNAMES = ["", "A"] + map(str, range(2, 11)) + ["J", "Q", "K"]


# Solitaire constants.  The only one I can think of is the number of
# row stacks.

NROWS = 7


# The rest of the program consists of class definitions.  Read their
# doc strings.

class Bottom:

    """A "card-like" object to serve as the bottom for some stacks.

    Specifically, this is used by the deck and the suit stacks.

    """

    def __init__(self, stack):

	"""Constructor, taking the stack as an argument.

	We displays ourselves as a gray rectangle the size of a
	playing card, positioned at the stack's x and y location.

	We register the stack's bottomhandler to handle clicks.

	No other behavior.

	"""

	self.rect = Rectangle(stack.game.canvas,
			      stack.x, stack.y,
			      stack.x+CARDWIDTH, stack.y+CARDHEIGHT,
			      outline='black', fill='gray')
 	self.rect.bind('<ButtonRelease-1>', stack.bottomhandler)


class Card:

    """A playing card.

    Public methods:

    moveto(x, y) -- move the card to an absolute position
    moveby(dx, dy) -- move the card by a relative offset
    tkraise() -- raise the card to the top of its stack
    showface(), showback() -- turn the card face up or down & raise it
    turnover() -- turn the card (face up or down) & raise it
    onclick(handler), ondouble(handler), onmove(handler),
    onrelease(handler) -- set various mount event handlers
    reset() -- move the card out of sight, face down, and reset all
               event handlers

    Public instance variables:

    color, suit, value -- the card's color, suit and value
    face_shown -- true when the card is shown face up, else false

    Semi-public instance variables (XXX should be made private):
    
    group -- the Canvas.Group representing the card
    x, y -- the position of the card's top left corner

    Private instance variables:

    __back, __rect, __text -- the canvas items making up the card

    (To show the card face up, the text item is placed in front of
    rect and the back is placed behind it.  To show it face down, this
    is reversed.)

    """

    def __init__(self, game, suit, value):
	self.suit = suit
	self.color = COLOR[suit]
	self.value = value
	canvas = game.canvas
	self.x = self.y = 0
	self.__back = Rectangle(canvas, MARGIN, MARGIN,
			      CARDWIDTH-MARGIN, CARDHEIGHT-MARGIN,
			      outline='black', fill='blue')
	self.__rect = Rectangle(canvas, 0, 0, CARDWIDTH, CARDHEIGHT,
			      outline='black', fill='white')
	text = "%s  %s" % (VALNAMES[value], suit)
	self.__text = CanvasText(canvas, CARDWIDTH/2, 0,
			       anchor=N, fill=self.color, text=text)
	self.group = Group(canvas)
	self.group.addtag_withtag(self.__back)
	self.group.addtag_withtag(self.__rect)
	self.group.addtag_withtag(self.__text)
	self.reset()

    def __repr__(self):
	return "Card(game, %s, %s)" % (`self.suit`, `self.value`)

    def moveto(self, x, y):
	dx = x - self.x
	dy = y - self.y
	self.group.move(dx, dy)
	self.x = x
	self.y = y

    def moveby(self, dx, dy):
	self.moveto(self.x + dx, self.y + dy)

    def tkraise(self):
	self.group.tkraise()

    def showface(self):
	self.tkraise()
	self.__rect.tkraise()
	self.__text.tkraise()
	self.face_shown = 1

    def showback(self):
	self.tkraise()
	self.__rect.tkraise()
	self.__back.tkraise()
	self.face_shown = 0

    def turnover(self):
	if self.face_shown:
	    self.showback()
	else:
	    self.showface()

    def onclick(self, handler):
	self.group.bind('<1>', handler)

    def ondouble(self, handler):
	self.group.bind('<Double-1>', handler)

    def onmove(self, handler):
	self.group.bind('<B1-Motion>', handler)

    def onrelease(self, handler):
	self.group.bind('<ButtonRelease-1>', handler)

    def reset(self):
	self.moveto(-1000, -1000)	# Out of sight
	self.onclick('')
	self.ondouble('')
	self.onmove('')
	self.onrelease('')
	self.showback()

class Deck:

    def __init__(self, game):
	self.game = game
	self.allcards = []
	for suit in ALLSUITS:
	    for value in ALLVALUES:
		self.allcards.append(Card(self.game, suit, value))
	self.reset()

    def shuffle(self):
	n = len(self.cards)
	newcards = []
	for i in randperm(n):
	    newcards.append(self.cards[i])
	self.cards = newcards

    def deal(self):
	# Raise IndexError when no more cards
	card = self.cards[-1]
	del self.cards[-1]
	return card

    def accept(self, card):
	if card not in self.cards:
	    self.cards.append(card)

    def reset(self):
	self.cards = self.allcards[:]
	for card in self.cards:
	    card.reset()

def randperm(n):
    r = range(n)
    x = []
    while r:
	i = random.choice(r)
	x.append(i)
	r.remove(i)
    return x

class Stack:

    x = MARGIN
    y = MARGIN

    def __init__(self, game):
	self.game = game
	self.cards = []

    def __repr__(self):
	return "<Stack at (%d, %d)>" % (self.x, self.y)

    def reset(self):
	self.cards = []

    def acceptable(self, cards):
	return 1

    def accept(self, card):
	self.cards.append(card)
	card.onclick(self.clickhandler)
	card.onmove(self.movehandler)
	card.onrelease(self.releasehandler)
	card.ondouble(self.doublehandler)
	card.tkraise()
	self.placecard(card)

    def placecard(self, card):
	card.moveto(self.x, self.y)

    def showtop(self):
	if self.cards:
	    self.cards[-1].showface()

    def clickhandler(self, event):
	pass

    def movehandler(self, event):
	pass

    def releasehandler(self, event):
	pass

    def doublehandler(self, event):
	pass

class PoolStack(Stack):

    def __init__(self, game):
	Stack.__init__(self, game)
	self.bottom = Bottom(self)

    def releasehandler(self, event):
	if not self.cards:
	    return
	card = self.cards[-1]
	self.game.turned.accept(card)
	del self.cards[-1]
	card.showface()

    def bottomhandler(self, event):
	cards = self.game.turned.cards
	cards.reverse()
	for card in cards:
	    card.showback()
	    self.accept(card)
	self.game.turned.reset()

class MovingStack(Stack):

    thecards = None
    theindex = None

    def clickhandler(self, event):
	self.thecards = self.theindex = None # Just in case
	tags = self.game.canvas.gettags('current')
	if not tags:
	    return
	tag = tags[0]
	for i in range(len(self.cards)):
	    card = self.cards[i]
	    if tag == str(card.group):
		break
	else:
	    return
	self.theindex = i
	self.thecards = Group(self.game.canvas)
	for card in self.cards[i:]:
	    self.thecards.addtag_withtag(card.group)
	self.thecards.tkraise()
	self.lastx = self.firstx = event.x
	self.lasty = self.firsty = event.y

    def movehandler(self, event):
	if not self.thecards:
	    return
	card = self.cards[self.theindex]
	if not card.face_shown:
	    return
	dx = event.x - self.lastx
	dy = event.y - self.lasty
	self.thecards.move(dx, dy)
	self.lastx = event.x
	self.lasty = event.y

    def releasehandler(self, event):
	cards = self._endmove()
	if not cards:
	    return
	card = cards[0]
	if not card.face_shown:
	    if len(cards) == 1:
		card.showface()
	    self.thecards = self.theindex = None
	    return
	stack = self.game.closeststack(cards[0])
	if stack and stack is not self and stack.acceptable(cards):
	    for card in cards:
		stack.accept(card)
		self.cards.remove(card)
	else:
	    for card in cards:
		self.placecard(card)

    def doublehandler(self, event):
	cards = self._endmove()
	if not cards:
	    return
	for stack in self.game.suits:
	    if stack.acceptable(cards):
		break
	else:
	    return
	for card in cards:
	    stack.accept(card)
	del self.cards[self.theindex:]
	self.thecards = self.theindex = None

    def _endmove(self):
	if not self.thecards:
	    return []
	self.thecards.move(self.firstx - self.lastx,
			   self.firsty - self.lasty)
	self.thecards.dtag()
	cards = self.cards[self.theindex:]
	if not cards:
	    return []
	card = cards[0]
	card.moveby(self.lastx - self.firstx, self.lasty - self.firsty)
	self.lastx = self.firstx
	self.lasty = self.firsty
	return cards

class TurnedStack(MovingStack):

    x = XSPACING + MARGIN
    y = MARGIN

class SuitStack(MovingStack):

    y = MARGIN

    def __init__(self, game, i):
	self.index = i
	self.x = MARGIN + XSPACING * (i+3)
	Stack.__init__(self, game)
	self.bottom = Bottom(self)

    bottomhandler = ""

    def __repr__(self):
	return "SuitStack(game, %d)" % self.index

    def acceptable(self, cards):
	if len(cards) != 1:
	    return 0
	card = cards[0]
	if not card.face_shown:
	    return 0
	if not self.cards:
	    return card.value == ACE
	topcard = self.cards[-1]
	if not topcard.face_shown:
	    return 0
	return card.suit == topcard.suit and card.value == topcard.value + 1

    def doublehandler(self, event):
	pass

    def accept(self, card):
	MovingStack.accept(self, card)
	if card.value == KING:
	    # See if we won
	    for s in self.game.suits:
		card = s.cards[-1]
		if card.value != KING:
		    return
	    self.game.win()
	    self.game.deal()

class RowStack(MovingStack):

    def __init__(self, game, i):
	self.index = i
	self.x = MARGIN + XSPACING * i
	self.y = MARGIN + YSPACING
	Stack.__init__(self, game)

    def __repr__(self):
	return "RowStack(game, %d)" % self.index

    def placecard(self, card):
	offset = 0
	for c in self.cards:
	    if c is card:
		break
	    if c.face_shown:
		offset = offset + 2*MARGIN
	    else:
		offset = offset + OFFSET
	card.moveto(self.x, self.y + offset)

    def acceptable(self, cards):
	card = cards[0]
	if not card.face_shown:
	    return 0
	if not self.cards:
	    return card.value == KING
	topcard = self.cards[-1]
	if not topcard.face_shown:
	    return 0
	if card.value != topcard.value - 1:
	    return 0
	if card.color == topcard.color:
	    return 0
	return 1

class Solitaire:

    def __init__(self, master):
	self.master = master

	self.buttonframe = Frame(self.master, background=BACKGROUND)
	self.buttonframe.pack(fill=X)

	self.dealbutton = Button(self.buttonframe,
				 text="Deal",
				 highlightthickness=0,
				 background=BACKGROUND,
				 activebackground="green",
				 command=self.deal)
	self.dealbutton.pack(side=LEFT)

	self.canvas = Canvas(self.master,
			     background=BACKGROUND,
			     highlightthickness=0,
			     width=NROWS*XSPACING,
			     height=3*YSPACING)
	self.canvas.pack(fill=BOTH, expand=TRUE)

	self.deck = Deck(self)
	
	self.pool = PoolStack(self)
	self.turned = TurnedStack(self)
	
	self.suits = []
	for i in range(NSUITS):
	    self.suits.append(SuitStack(self, i))

	self.rows = []
	for i in range(NROWS):
	    self.rows.append(RowStack(self, i))
	
	self.deal()

    def win(self):
	"""Stupid animation when you win."""
	cards = self.deck.allcards
	for i in range(1000):
	    card = random.choice(cards)
	    dx = random.randint(-50, 50)
	    dy = random.randint(-50, 50)
	    card.moveby(dx, dy)
	    self.master.update_idletasks()

    def closeststack(self, card):
	closest = None
	cdist = 999999999
	# Since we only compare distances,
	# we don't bother to take the square root.
	for stack in self.rows + self.suits:
	    dist = (stack.x - card.x)**2 + (stack.y - card.y)**2
	    if dist < cdist:
		closest = stack
		cdist = dist
	return closest

    def reset(self):
	self.pool.reset()
	self.turned.reset()
	for stack in self.rows + self.suits:
	    stack.reset()
	self.deck.reset()

    def deal(self):
	self.reset()
	self.deck.shuffle()
	for i in range(NROWS):
	    for r in self.rows[i:]:
		card = self.deck.deal()
		r.accept(card)
	for r in self.rows:
	    r.showtop()
	try:
	    while 1:
		self.pool.accept(self.deck.deal())
	except IndexError:
	    pass


# Main function, run when invoked as a stand-alone Python program.

def main():
    root = Tk()
    game = Solitaire(root)
    root.protocol('WM_DELETE_WINDOW', root.quit)
    root.mainloop()

if __name__ == '__main__':
    main()
