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

Apology:

I'm not much of a card player, so my terminology in these comments may
at times be a little unusual.  If you have suggestions, please let me
know!

"""

# Imports

import random

from tkinter import *
from canvasevents import Group


# Constants determining the size and lay-out of cards and stacks.  We
# work in a "grid" where each card/stack is surrounded by MARGIN
# pixels of space on each side, so adjacent stacks are separated by
# 2*MARGIN pixels.  OFFSET is the offset used for displaying the
# face down cards in the row stacks.

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

ALLSUITS = list(COLOR.keys())
NSUITS = len(ALLSUITS)


# Card values are 1-13.  We also define symbolic names for the picture
# cards.  ALLVALUES is a list of all card values.

ACE = 1
JACK = 11
QUEEN = 12
KING = 13
ALLVALUES = range(1, 14) # (one more than the highest value)
NVALUES = len(ALLVALUES)


# VALNAMES is a list that maps a card value to string.  It contains a
# dummy element at index 0 so it can be indexed directly with the card
# value.

VALNAMES = ["", "A"] + list(map(str, range(2, 11))) + ["J", "Q", "K"]


# Solitaire constants.  The only one I can think of is the number of
# row stacks.

NROWS = 7


# The rest of the program consists of class definitions.  These are
# further described in their documentation strings.


class Card:

    """A playing card.

    A card doesn't record to which stack it belongs; only the stack
    records this (it turns out that we always know this from the
    context, and this saves a ``double update'' with potential for
    inconsistencies).

    Public methods:

    moveto(x, y) -- move the card to an absolute position
    moveby(dx, dy) -- move the card by a relative offset
    tkraise() -- raise the card to the top of its stack
    showface(), showback() -- turn the card face up or down & raise it

    Public read-only instance variables:

    suit, value, color -- the card's suit, value and color
    face_shown -- true when the card is shown face up, else false

    Semi-public read-only instance variables (XXX should be made
    private):

    group -- the Canvas.Group representing the card
    x, y -- the position of the card's top left corner

    Private instance variables:

    __back, __rect, __text -- the canvas items making up the card

    (To show the card face up, the text item is placed in front of
    rect and the back is placed behind it.  To show it face down, this
    is reversed.  The card is created face down.)

    """

    def __init__(self, suit, value, canvas):
        """Card constructor.

        Arguments are the card's suit and value, and the canvas widget.

        The card is created at position (0, 0), with its face down
        (adding it to a stack will position it according to that
        stack's rules).

        """
        self.suit = suit
        self.value = value
        self.color = COLOR[suit]
        self.face_shown = 0

        self.x = self.y = 0
        self.canvas = canvas
        self.group = Group(canvas)

        text = "%s  %s" % (VALNAMES[value], suit)
        self.__text = canvas.create_text(CARDWIDTH // 2, 0, anchor=N,
                                         fill=self.color, text=text)
        self.group.addtag_withtag(self.__text)

        self.__rect = canvas.create_rectangle(0, 0, CARDWIDTH, CARDHEIGHT,
                                              outline='black', fill='white')
        self.group.addtag_withtag(self.__rect)

        self.__back = canvas.create_rectangle(MARGIN, MARGIN,
                                              CARDWIDTH - MARGIN,
                                              CARDHEIGHT - MARGIN,
                                              outline='black', fill='blue')
        self.group.addtag_withtag(self.__back)

    def __repr__(self):
        """Return a string for debug print statements."""
        return "Card(%r, %r)" % (self.suit, self.value)

    def moveto(self, x, y):
        """Move the card to absolute position (x, y)."""
        self.moveby(x - self.x, y - self.y)

    def moveby(self, dx, dy):
        """Move the card by (dx, dy)."""
        self.x = self.x + dx
        self.y = self.y + dy
        self.group.move(dx, dy)

    def tkraise(self):
        """Raise the card above all other objects in its canvas."""
        self.group.tkraise()

    def showface(self):
        """Turn the card's face up."""
        self.tkraise()
        self.canvas.tag_raise(self.__rect)
        self.canvas.tag_raise(self.__text)
        self.face_shown = 1

    def showback(self):
        """Turn the card's face down."""
        self.tkraise()
        self.canvas.tag_raise(self.__rect)
        self.canvas.tag_raise(self.__back)
        self.face_shown = 0


class Stack:

    """A generic stack of cards.

    This is used as a base class for all other stacks (e.g. the deck,
    the suit stacks, and the row stacks).

    Public methods:

    add(card) -- add a card to the stack
    delete(card) -- delete a card from the stack
    showtop() -- show the top card (if any) face up
    deal() -- delete and return the top card, or None if empty

    Method that subclasses may override:

    position(card) -- move the card to its proper (x, y) position

        The default position() method places all cards at the stack's
        own (x, y) position.

    userclickhandler(), userdoubleclickhandler() -- called to do
    subclass specific things on single and double clicks

        The default user (single) click handler shows the top card
        face up.  The default user double click handler calls the user
        single click handler.

    usermovehandler(cards) -- called to complete a subpile move

        The default user move handler moves all moved cards back to
        their original position (by calling the position() method).

    Private methods:

    clickhandler(event), doubleclickhandler(event),
    motionhandler(event), releasehandler(event) -- event handlers

        The default event handlers turn the top card of the stack with
        its face up on a (single or double) click, and also support
        moving a subpile around.

    startmoving(event) -- begin a move operation
    finishmoving() -- finish a move operation

    """

    def __init__(self, x, y, game=None):
        """Stack constructor.

        Arguments are the stack's nominal x and y position (the top
        left corner of the first card placed in the stack), and the
        game object (which is used to get the canvas; subclasses use
        the game object to find other stacks).

        """
        self.x = x
        self.y = y
        self.game = game
        self.cards = []
        self.group = Group(self.game.canvas)
        self.group.bind('<1>', self.clickhandler)
        self.group.bind('<Double-1>', self.doubleclickhandler)
        self.group.bind('<B1-Motion>', self.motionhandler)
        self.group.bind('<ButtonRelease-1>', self.releasehandler)
        self.makebottom()

    def makebottom(self):
        pass

    def __repr__(self):
        """Return a string for debug print statements."""
        return "%s(%d, %d)" % (self.__class__.__name__, self.x, self.y)

    # Public methods

    def add(self, card):
        self.cards.append(card)
        card.tkraise()
        self.position(card)
        self.group.addtag_withtag(card.group)

    def delete(self, card):
        self.cards.remove(card)
        card.group.dtag(self.group)

    def showtop(self):
        if self.cards:
            self.cards[-1].showface()

    def deal(self):
        if not self.cards:
            return None
        card = self.cards[-1]
        self.delete(card)
        return card

    # Subclass overridable methods

    def position(self, card):
        card.moveto(self.x, self.y)

    def userclickhandler(self):
        self.showtop()

    def userdoubleclickhandler(self):
        self.userclickhandler()

    def usermovehandler(self, cards):
        for card in cards:
            self.position(card)

    # Event handlers

    def clickhandler(self, event):
        self.finishmoving()             # In case we lost an event
        self.userclickhandler()
        self.startmoving(event)

    def motionhandler(self, event):
        self.keepmoving(event)

    def releasehandler(self, event):
        self.keepmoving(event)
        self.finishmoving()

    def doubleclickhandler(self, event):
        self.finishmoving()             # In case we lost an event
        self.userdoubleclickhandler()
        self.startmoving(event)

    # Move internals

    moving = None

    def startmoving(self, event):
        self.moving = None
        tags = self.game.canvas.gettags('current')
        for i in range(len(self.cards)):
            card = self.cards[i]
            if card.group.tag in tags:
                break
        else:
            return
        if not card.face_shown:
            return
        self.moving = self.cards[i:]
        self.lastx = event.x
        self.lasty = event.y
        for card in self.moving:
            card.tkraise()

    def keepmoving(self, event):
        if not self.moving:
            return
        dx = event.x - self.lastx
        dy = event.y - self.lasty
        self.lastx = event.x
        self.lasty = event.y
        if dx or dy:
            for card in self.moving:
                card.moveby(dx, dy)

    def finishmoving(self):
        cards = self.moving
        self.moving = None
        if cards:
            self.usermovehandler(cards)


class Deck(Stack):

    """The deck is a stack with support for shuffling.

    New methods:

    fill() -- create the playing cards
    shuffle() -- shuffle the playing cards

    A single click moves the top card to the game's open deck and
    moves it face up; if we're out of cards, it moves the open deck
    back to the deck.

    """

    def makebottom(self):
        bottom = self.game.canvas.create_rectangle(self.x, self.y,
            self.x + CARDWIDTH, self.y + CARDHEIGHT, outline='black',
            fill=BACKGROUND)
        self.group.addtag_withtag(bottom)

    def fill(self):
        for suit in ALLSUITS:
            for value in ALLVALUES:
                self.add(Card(suit, value, self.game.canvas))

    def shuffle(self):
        n = len(self.cards)
        newcards = []
        for i in randperm(n):
            newcards.append(self.cards[i])
        self.cards = newcards

    def userclickhandler(self):
        opendeck = self.game.opendeck
        card = self.deal()
        if not card:
            while 1:
                card = opendeck.deal()
                if not card:
                    break
                self.add(card)
                card.showback()
        else:
            self.game.opendeck.add(card)
            card.showface()


def randperm(n):
    """Function returning a random permutation of range(n)."""
    r = list(range(n))
    x = []
    while r:
        i = random.choice(r)
        x.append(i)
        r.remove(i)
    return x


class OpenStack(Stack):

    def acceptable(self, cards):
        return 0

    def usermovehandler(self, cards):
        card = cards[0]
        stack = self.game.closeststack(card)
        if not stack or stack is self or not stack.acceptable(cards):
            Stack.usermovehandler(self, cards)
        else:
            for card in cards:
                self.delete(card)
                stack.add(card)
            self.game.wincheck()

    def userdoubleclickhandler(self):
        if not self.cards:
            return
        card = self.cards[-1]
        if not card.face_shown:
            self.userclickhandler()
            return
        for s in self.game.suits:
            if s.acceptable([card]):
                self.delete(card)
                s.add(card)
                self.game.wincheck()
                break


class SuitStack(OpenStack):

    def makebottom(self):
        bottom = self.game.canvas.create_rectangle(self.x, self.y,
            self.x + CARDWIDTH, self.y + CARDHEIGHT, outline='black', fill='')

    def userclickhandler(self):
        pass

    def userdoubleclickhandler(self):
        pass

    def acceptable(self, cards):
        if len(cards) != 1:
            return 0
        card = cards[0]
        if not self.cards:
            return card.value == ACE
        topcard = self.cards[-1]
        return card.suit == topcard.suit and card.value == topcard.value + 1


class RowStack(OpenStack):

    def acceptable(self, cards):
        card = cards[0]
        if not self.cards:
            return card.value == KING
        topcard = self.cards[-1]
        if not topcard.face_shown:
            return 0
        return card.color != topcard.color and card.value == topcard.value - 1

    def position(self, card):
        y = self.y
        for c in self.cards:
            if c == card:
                break
            if c.face_shown:
                y = y + 2*MARGIN
            else:
                y = y + OFFSET
        card.moveto(self.x, y)


class Solitaire:

    def __init__(self, master):
        self.master = master

        self.canvas = Canvas(self.master,
                             background=BACKGROUND,
                             highlightthickness=0,
                             width=NROWS*XSPACING,
                             height=3*YSPACING + 20 + MARGIN)
        self.canvas.pack(fill=BOTH, expand=TRUE)

        self.dealbutton = Button(self.canvas,
                                 text="Deal",
                                 highlightthickness=0,
                                 background=BACKGROUND,
                                 activebackground="green",
                                 command=self.deal)
        self.canvas.create_window(MARGIN, 3 * YSPACING + 20,
            window=self.dealbutton, anchor=SW)

        x = MARGIN
        y = MARGIN

        self.deck = Deck(x, y, self)

        x = x + XSPACING
        self.opendeck = OpenStack(x, y, self)

        x = x + XSPACING
        self.suits = []
        for i in range(NSUITS):
            x = x + XSPACING
            self.suits.append(SuitStack(x, y, self))

        x = MARGIN
        y = y + YSPACING

        self.rows = []
        for i in range(NROWS):
            self.rows.append(RowStack(x, y, self))
            x = x + XSPACING

        self.openstacks = [self.opendeck] + self.suits + self.rows

        self.deck.fill()
        self.deal()

    def wincheck(self):
        for s in self.suits:
            if len(s.cards) != NVALUES:
                return
        self.win()
        self.deal()

    def win(self):
        """Stupid animation when you win."""
        cards = []
        for s in self.openstacks:
            cards = cards + s.cards
        while cards:
            card = random.choice(cards)
            cards.remove(card)
            self.animatedmoveto(card, self.deck)

    def animatedmoveto(self, card, dest):
        for i in range(10, 0, -1):
            dx, dy = (dest.x-card.x)//i, (dest.y-card.y)//i
            card.moveby(dx, dy)
            self.master.update_idletasks()

    def closeststack(self, card):
        closest = None
        cdist = 999999999
        # Since we only compare distances,
        # we don't bother to take the square root.
        for stack in self.openstacks:
            dist = (stack.x - card.x)**2 + (stack.y - card.y)**2
            if dist < cdist:
                closest = stack
                cdist = dist
        return closest

    def deal(self):
        self.reset()
        self.deck.shuffle()
        for i in range(NROWS):
            for r in self.rows[i:]:
                card = self.deck.deal()
                r.add(card)
        for r in self.rows:
            r.showtop()

    def reset(self):
        for stack in self.openstacks:
            while 1:
                card = stack.deal()
                if not card:
                    break
                self.deck.add(card)
                card.showback()


# Main function, run when invoked as a stand-alone Python program.

def main():
    root = Tk()
    game = Solitaire(root)
    root.protocol('WM_DELETE_WINDOW', root.quit)
    root.mainloop()

if __name__ == '__main__':
    main()
