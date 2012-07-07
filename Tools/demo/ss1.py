#!/usr/bin/env python3

"""
SS1 -- a spreadsheet-like application.
"""

import os
import re
import sys
import html
from xml.parsers import expat

LEFT, CENTER, RIGHT = "LEFT", "CENTER", "RIGHT"

def ljust(x, n):
    return x.ljust(n)
def center(x, n):
    return x.center(n)
def rjust(x, n):
    return x.rjust(n)
align2action = {LEFT: ljust, CENTER: center, RIGHT: rjust}

align2xml = {LEFT: "left", CENTER: "center", RIGHT: "right"}
xml2align = {"left": LEFT, "center": CENTER, "right": RIGHT}

align2anchor = {LEFT: "w", CENTER: "center", RIGHT: "e"}

def sum(seq):
    total = 0
    for x in seq:
        if x is not None:
            total += x
    return total

class Sheet:

    def __init__(self):
        self.cells = {} # {(x, y): cell, ...}
        self.ns = dict(
            cell = self.cellvalue,
            cells = self.multicellvalue,
            sum = sum,
        )

    def cellvalue(self, x, y):
        cell = self.getcell(x, y)
        if hasattr(cell, 'recalc'):
            return cell.recalc(self.ns)
        else:
            return cell

    def multicellvalue(self, x1, y1, x2, y2):
        if x1 > x2:
            x1, x2 = x2, x1
        if y1 > y2:
            y1, y2 = y2, y1
        seq = []
        for y in range(y1, y2+1):
            for x in range(x1, x2+1):
                seq.append(self.cellvalue(x, y))
        return seq

    def getcell(self, x, y):
        return self.cells.get((x, y))

    def setcell(self, x, y, cell):
        assert x > 0 and y > 0
        assert isinstance(cell, BaseCell)
        self.cells[x, y] = cell

    def clearcell(self, x, y):
        try:
            del self.cells[x, y]
        except KeyError:
            pass

    def clearcells(self, x1, y1, x2, y2):
        for xy in self.selectcells(x1, y1, x2, y2):
            del self.cells[xy]

    def clearrows(self, y1, y2):
        self.clearcells(0, y1, sys.maxint, y2)

    def clearcolumns(self, x1, x2):
        self.clearcells(x1, 0, x2, sys.maxint)

    def selectcells(self, x1, y1, x2, y2):
        if x1 > x2:
            x1, x2 = x2, x1
        if y1 > y2:
            y1, y2 = y2, y1
        return [(x, y) for x, y in self.cells
                if x1 <= x <= x2 and y1 <= y <= y2]

    def movecells(self, x1, y1, x2, y2, dx, dy):
        if dx == 0 and dy == 0:
            return
        if x1 > x2:
            x1, x2 = x2, x1
        if y1 > y2:
            y1, y2 = y2, y1
        assert x1+dx > 0 and y1+dy > 0
        new = {}
        for x, y in self.cells:
            cell = self.cells[x, y]
            if hasattr(cell, 'renumber'):
                cell = cell.renumber(x1, y1, x2, y2, dx, dy)
            if x1 <= x <= x2 and y1 <= y <= y2:
                x += dx
                y += dy
            new[x, y] = cell
        self.cells = new

    def insertrows(self, y, n):
        assert n > 0
        self.movecells(0, y, sys.maxint, sys.maxint, 0, n)

    def deleterows(self, y1, y2):
        if y1 > y2:
            y1, y2 = y2, y1
        self.clearrows(y1, y2)
        self.movecells(0, y2+1, sys.maxint, sys.maxint, 0, y1-y2-1)

    def insertcolumns(self, x, n):
        assert n > 0
        self.movecells(x, 0, sys.maxint, sys.maxint, n, 0)

    def deletecolumns(self, x1, x2):
        if x1 > x2:
            x1, x2 = x2, x1
        self.clearcells(x1, x2)
        self.movecells(x2+1, 0, sys.maxint, sys.maxint, x1-x2-1, 0)

    def getsize(self):
        maxx = maxy = 0
        for x, y in self.cells:
            maxx = max(maxx, x)
            maxy = max(maxy, y)
        return maxx, maxy

    def reset(self):
        for cell in self.cells.values():
            if hasattr(cell, 'reset'):
                cell.reset()

    def recalc(self):
        self.reset()
        for cell in self.cells.values():
            if hasattr(cell, 'recalc'):
                cell.recalc(self.ns)

    def display(self):
        maxx, maxy = self.getsize()
        width, height = maxx+1, maxy+1
        colwidth = [1] * width
        full = {}
        # Add column heading labels in row 0
        for x in range(1, width):
            full[x, 0] = text, alignment = colnum2name(x), RIGHT
            colwidth[x] = max(colwidth[x], len(text))
        # Add row labels in column 0
        for y in range(1, height):
            full[0, y] = text, alignment = str(y), RIGHT
            colwidth[0] = max(colwidth[0], len(text))
        # Add sheet cells in columns with x>0 and y>0
        for (x, y), cell in self.cells.items():
            if x <= 0 or y <= 0:
                continue
            if hasattr(cell, 'recalc'):
                cell.recalc(self.ns)
            if hasattr(cell, 'format'):
                text, alignment = cell.format()
                assert isinstance(text, str)
                assert alignment in (LEFT, CENTER, RIGHT)
            else:
                text = str(cell)
                if isinstance(cell, str):
                    alignment = LEFT
                else:
                    alignment = RIGHT
            full[x, y] = (text, alignment)
            colwidth[x] = max(colwidth[x], len(text))
        # Calculate the horizontal separator line (dashes and dots)
        sep = ""
        for x in range(width):
            if sep:
                sep += "+"
            sep += "-"*colwidth[x]
        # Now print The full grid
        for y in range(height):
            line = ""
            for x in range(width):
                text, alignment = full.get((x, y)) or ("", LEFT)
                text = align2action[alignment](text, colwidth[x])
                if line:
                    line += '|'
                line += text
            print(line)
            if y == 0:
                print(sep)

    def xml(self):
        out = ['<spreadsheet>']
        for (x, y), cell in self.cells.items():
            if hasattr(cell, 'xml'):
                cellxml = cell.xml()
            else:
                cellxml = '<value>%s</value>' % html.escape(cell)
            out.append('<cell row="%s" col="%s">\n  %s\n</cell>' %
                       (y, x, cellxml))
        out.append('</spreadsheet>')
        return '\n'.join(out)

    def save(self, filename):
        text = self.xml()
        f = open(filename, "w")
        f.write(text)
        if text and not text.endswith('\n'):
            f.write('\n')
        f.close()

    def load(self, filename):
        f = open(filename, 'rb')
        SheetParser(self).parsefile(f)
        f.close()

class SheetParser:

    def __init__(self, sheet):
        self.sheet = sheet

    def parsefile(self, f):
        parser = expat.ParserCreate()
        parser.StartElementHandler = self.startelement
        parser.EndElementHandler = self.endelement
        parser.CharacterDataHandler = self.data
        parser.ParseFile(f)

    def startelement(self, tag, attrs):
        method = getattr(self, 'start_'+tag, None)
        if method:
            for key, value in attrs.items():
                attrs[key] = str(value) # XXX Convert Unicode to 8-bit
            method(attrs)
        self.texts = []

    def data(self, text):
        text = str(text) # XXX Convert Unicode to 8-bit
        self.texts.append(text)

    def endelement(self, tag):
        method = getattr(self, 'end_'+tag, None)
        if method:
            method("".join(self.texts))

    def start_cell(self, attrs):
        self.y = int(attrs.get("row"))
        self.x = int(attrs.get("col"))

    def start_value(self, attrs):
        self.fmt = attrs.get('format')
        self.alignment = xml2align.get(attrs.get('align'))

    start_formula = start_value

    def end_int(self, text):
        try:
            self.value = int(text)
        except:
            self.value = None

    def end_long(self, text):
        try:
            self.value = int(text)
        except:
            self.value = None

    def end_double(self, text):
        try:
            self.value = float(text)
        except:
            self.value = None

    def end_complex(self, text):
        try:
            self.value = complex(text)
        except:
            self.value = None

    def end_string(self, text):
        try:
            self.value = text
        except:
            self.value = None

    def end_value(self, text):
        if isinstance(self.value, BaseCell):
            self.cell = self.value
        elif isinstance(self.value, str):
            self.cell = StringCell(self.value,
                                   self.fmt or "%s",
                                   self.alignment or LEFT)
        else:
            self.cell = NumericCell(self.value,
                                    self.fmt or "%s",
                                    self.alignment or RIGHT)

    def end_formula(self, text):
        self.cell = FormulaCell(text,
                                self.fmt or "%s",
                                self.alignment or RIGHT)

    def end_cell(self, text):
        self.sheet.setcell(self.x, self.y, self.cell)

class BaseCell:
    __init__ = None # Must provide
    """Abstract base class for sheet cells.

    Subclasses may but needn't provide the following APIs:

    cell.reset() -- prepare for recalculation
    cell.recalc(ns) -> value -- recalculate formula
    cell.format() -> (value, alignment) -- return formatted value
    cell.xml() -> string -- return XML
    """

class NumericCell(BaseCell):

    def __init__(self, value, fmt="%s", alignment=RIGHT):
        assert isinstance(value, (int, int, float, complex))
        assert alignment in (LEFT, CENTER, RIGHT)
        self.value = value
        self.fmt = fmt
        self.alignment = alignment

    def recalc(self, ns):
        return self.value

    def format(self):
        try:
            text = self.fmt % self.value
        except:
            text = str(self.value)
        return text, self.alignment

    def xml(self):
        method = getattr(self, '_xml_' + type(self.value).__name__)
        return '<value align="%s" format="%s">%s</value>' % (
                align2xml[self.alignment],
                self.fmt,
                method())

    def _xml_int(self):
        if -2**31 <= self.value < 2**31:
            return '<int>%s</int>' % self.value
        else:
            return self._xml_long()

    def _xml_long(self):
        return '<long>%s</long>' % self.value

    def _xml_float(self):
        return '<double>%s</double>' % repr(self.value)

    def _xml_complex(self):
        return '<complex>%s</double>' % repr(self.value)

class StringCell(BaseCell):

    def __init__(self, text, fmt="%s", alignment=LEFT):
        assert isinstance(text, (str, str))
        assert alignment in (LEFT, CENTER, RIGHT)
        self.text = text
        self.fmt = fmt
        self.alignment = alignment

    def recalc(self, ns):
        return self.text

    def format(self):
        return self.text, self.alignment

    def xml(self):
        s = '<value align="%s" format="%s"><string>%s</string></value>'
        return s % (
            align2xml[self.alignment],
            self.fmt,
            html.escape(self.text))

class FormulaCell(BaseCell):

    def __init__(self, formula, fmt="%s", alignment=RIGHT):
        assert alignment in (LEFT, CENTER, RIGHT)
        self.formula = formula
        self.translated = translate(self.formula)
        self.fmt = fmt
        self.alignment = alignment
        self.reset()

    def reset(self):
        self.value = None

    def recalc(self, ns):
        if self.value is None:
            try:
                # A hack to evaluate expressions using true division
                self.value = eval(self.translated, ns)
            except:
                exc = sys.exc_info()[0]
                if hasattr(exc, "__name__"):
                    self.value = exc.__name__
                else:
                    self.value = str(exc)
        return self.value

    def format(self):
        try:
            text = self.fmt % self.value
        except:
            text = str(self.value)
        return text, self.alignment

    def xml(self):
        return '<formula align="%s" format="%s">%s</formula>' % (
            align2xml[self.alignment],
            self.fmt,
            self.formula)

    def renumber(self, x1, y1, x2, y2, dx, dy):
        out = []
        for part in re.split('(\w+)', self.formula):
            m = re.match('^([A-Z]+)([1-9][0-9]*)$', part)
            if m is not None:
                sx, sy = m.groups()
                x = colname2num(sx)
                y = int(sy)
                if x1 <= x <= x2 and y1 <= y <= y2:
                    part = cellname(x+dx, y+dy)
            out.append(part)
        return FormulaCell("".join(out), self.fmt, self.alignment)

def translate(formula):
    """Translate a formula containing fancy cell names to valid Python code.

    Examples:
        B4 -> cell(2, 4)
        B4:Z100 -> cells(2, 4, 26, 100)
    """
    out = []
    for part in re.split(r"(\w+(?::\w+)?)", formula):
        m = re.match(r"^([A-Z]+)([1-9][0-9]*)(?::([A-Z]+)([1-9][0-9]*))?$", part)
        if m is None:
            out.append(part)
        else:
            x1, y1, x2, y2 = m.groups()
            x1 = colname2num(x1)
            if x2 is None:
                s = "cell(%s, %s)" % (x1, y1)
            else:
                x2 = colname2num(x2)
                s = "cells(%s, %s, %s, %s)" % (x1, y1, x2, y2)
            out.append(s)
    return "".join(out)

def cellname(x, y):
    "Translate a cell coordinate to a fancy cell name (e.g. (1, 1)->'A1')."
    assert x > 0 # Column 0 has an empty name, so can't use that
    return colnum2name(x) + str(y)

def colname2num(s):
    "Translate a column name to number (e.g. 'A'->1, 'Z'->26, 'AA'->27)."
    s = s.upper()
    n = 0
    for c in s:
        assert 'A' <= c <= 'Z'
        n = n*26 + ord(c) - ord('A') + 1
    return n

def colnum2name(n):
    "Translate a column number to name (e.g. 1->'A', etc.)."
    assert n > 0
    s = ""
    while n:
        n, m = divmod(n-1, 26)
        s = chr(m+ord('A')) + s
    return s

import tkinter as Tk

class SheetGUI:

    """Beginnings of a GUI for a spreadsheet.

    TO DO:
    - clear multiple cells
    - Insert, clear, remove rows or columns
    - Show new contents while typing
    - Scroll bars
    - Grow grid when window is grown
    - Proper menus
    - Undo, redo
    - Cut, copy and paste
    - Formatting and alignment
    """

    def __init__(self, filename="sheet1.xml", rows=10, columns=5):
        """Constructor.

        Load the sheet from the filename argument.
        Set up the Tk widget tree.
        """
        # Create and load the sheet
        self.filename = filename
        self.sheet = Sheet()
        if os.path.isfile(filename):
            self.sheet.load(filename)
        # Calculate the needed grid size
        maxx, maxy = self.sheet.getsize()
        rows = max(rows, maxy)
        columns = max(columns, maxx)
        # Create the widgets
        self.root = Tk.Tk()
        self.root.wm_title("Spreadsheet: %s" % self.filename)
        self.beacon = Tk.Label(self.root, text="A1",
                               font=('helvetica', 16, 'bold'))
        self.entry = Tk.Entry(self.root)
        self.savebutton = Tk.Button(self.root, text="Save",
                                    command=self.save)
        self.cellgrid = Tk.Frame(self.root)
        # Configure the widget lay-out
        self.cellgrid.pack(side="bottom", expand=1, fill="both")
        self.beacon.pack(side="left")
        self.savebutton.pack(side="right")
        self.entry.pack(side="left", expand=1, fill="x")
        # Bind some events
        self.entry.bind("<Return>", self.return_event)
        self.entry.bind("<Shift-Return>", self.shift_return_event)
        self.entry.bind("<Tab>", self.tab_event)
        self.entry.bind("<Shift-Tab>", self.shift_tab_event)
        self.entry.bind("<Delete>", self.delete_event)
        self.entry.bind("<Escape>", self.escape_event)
        # Now create the cell grid
        self.makegrid(rows, columns)
        # Select the top-left cell
        self.currentxy = None
        self.cornerxy = None
        self.setcurrent(1, 1)
        # Copy the sheet cells to the GUI cells
        self.sync()

    def delete_event(self, event):
        if self.cornerxy != self.currentxy and self.cornerxy is not None:
            self.sheet.clearcells(*(self.currentxy + self.cornerxy))
        else:
            self.sheet.clearcell(*self.currentxy)
        self.sync()
        self.entry.delete(0, 'end')
        return "break"

    def escape_event(self, event):
        x, y = self.currentxy
        self.load_entry(x, y)

    def load_entry(self, x, y):
        cell = self.sheet.getcell(x, y)
        if cell is None:
            text = ""
        elif isinstance(cell, FormulaCell):
            text = '=' + cell.formula
        else:
            text, alignment = cell.format()
        self.entry.delete(0, 'end')
        self.entry.insert(0, text)
        self.entry.selection_range(0, 'end')

    def makegrid(self, rows, columns):
        """Helper to create the grid of GUI cells.

        The edge (x==0 or y==0) is filled with labels; the rest is real cells.
        """
        self.rows = rows
        self.columns = columns
        self.gridcells = {}
        # Create the top left corner cell (which selects all)
        cell = Tk.Label(self.cellgrid, relief='raised')
        cell.grid_configure(column=0, row=0, sticky='NSWE')
        cell.bind("<ButtonPress-1>", self.selectall)
        # Create the top row of labels, and configure the grid columns
        for x in range(1, columns+1):
            self.cellgrid.grid_columnconfigure(x, minsize=64)
            cell = Tk.Label(self.cellgrid, text=colnum2name(x), relief='raised')
            cell.grid_configure(column=x, row=0, sticky='WE')
            self.gridcells[x, 0] = cell
            cell.__x = x
            cell.__y = 0
            cell.bind("<ButtonPress-1>", self.selectcolumn)
            cell.bind("<B1-Motion>", self.extendcolumn)
            cell.bind("<ButtonRelease-1>", self.extendcolumn)
            cell.bind("<Shift-Button-1>", self.extendcolumn)
        # Create the leftmost column of labels
        for y in range(1, rows+1):
            cell = Tk.Label(self.cellgrid, text=str(y), relief='raised')
            cell.grid_configure(column=0, row=y, sticky='WE')
            self.gridcells[0, y] = cell
            cell.__x = 0
            cell.__y = y
            cell.bind("<ButtonPress-1>", self.selectrow)
            cell.bind("<B1-Motion>", self.extendrow)
            cell.bind("<ButtonRelease-1>", self.extendrow)
            cell.bind("<Shift-Button-1>", self.extendrow)
        # Create the real cells
        for x in range(1, columns+1):
            for y in range(1, rows+1):
                cell = Tk.Label(self.cellgrid, relief='sunken',
                                bg='white', fg='black')
                cell.grid_configure(column=x, row=y, sticky='NSWE')
                self.gridcells[x, y] = cell
                cell.__x = x
                cell.__y = y
                # Bind mouse events
                cell.bind("<ButtonPress-1>", self.press)
                cell.bind("<B1-Motion>", self.motion)
                cell.bind("<ButtonRelease-1>", self.release)
                cell.bind("<Shift-Button-1>", self.release)

    def selectall(self, event):
        self.setcurrent(1, 1)
        self.setcorner(sys.maxint, sys.maxint)

    def selectcolumn(self, event):
        x, y = self.whichxy(event)
        self.setcurrent(x, 1)
        self.setcorner(x, sys.maxint)

    def extendcolumn(self, event):
        x, y = self.whichxy(event)
        if x > 0:
            self.setcurrent(self.currentxy[0], 1)
            self.setcorner(x, sys.maxint)

    def selectrow(self, event):
        x, y = self.whichxy(event)
        self.setcurrent(1, y)
        self.setcorner(sys.maxint, y)

    def extendrow(self, event):
        x, y = self.whichxy(event)
        if y > 0:
            self.setcurrent(1, self.currentxy[1])
            self.setcorner(sys.maxint, y)

    def press(self, event):
        x, y = self.whichxy(event)
        if x > 0 and y > 0:
            self.setcurrent(x, y)

    def motion(self, event):
        x, y = self.whichxy(event)
        if x > 0 and y > 0:
            self.setcorner(x, y)

    release = motion

    def whichxy(self, event):
        w = self.cellgrid.winfo_containing(event.x_root, event.y_root)
        if w is not None and isinstance(w, Tk.Label):
            try:
                return w.__x, w.__y
            except AttributeError:
                pass
        return 0, 0

    def save(self):
        self.sheet.save(self.filename)

    def setcurrent(self, x, y):
        "Make (x, y) the current cell."
        if self.currentxy is not None:
            self.change_cell()
        self.clearfocus()
        self.beacon['text'] = cellname(x, y)
        self.load_entry(x, y)
        self.entry.focus_set()
        self.currentxy = x, y
        self.cornerxy = None
        gridcell = self.gridcells.get(self.currentxy)
        if gridcell is not None:
            gridcell['bg'] = 'yellow'

    def setcorner(self, x, y):
        if self.currentxy is None or self.currentxy == (x, y):
            self.setcurrent(x, y)
            return
        self.clearfocus()
        self.cornerxy = x, y
        x1, y1 = self.currentxy
        x2, y2 = self.cornerxy or self.currentxy
        if x1 > x2:
            x1, x2 = x2, x1
        if y1 > y2:
            y1, y2 = y2, y1
        for (x, y), cell in self.gridcells.items():
            if x1 <= x <= x2 and y1 <= y <= y2:
                cell['bg'] = 'lightBlue'
        gridcell = self.gridcells.get(self.currentxy)
        if gridcell is not None:
            gridcell['bg'] = 'yellow'
        self.setbeacon(x1, y1, x2, y2)

    def setbeacon(self, x1, y1, x2, y2):
        if x1 == y1 == 1 and x2 == y2 == sys.maxint:
            name = ":"
        elif (x1, x2) == (1, sys.maxint):
            if y1 == y2:
                name = "%d" % y1
            else:
                name = "%d:%d" % (y1, y2)
        elif (y1, y2) == (1, sys.maxint):
            if x1 == x2:
                name = "%s" % colnum2name(x1)
            else:
                name = "%s:%s" % (colnum2name(x1), colnum2name(x2))
        else:
            name1 = cellname(*self.currentxy)
            name2 = cellname(*self.cornerxy)
            name = "%s:%s" % (name1, name2)
        self.beacon['text'] = name


    def clearfocus(self):
        if self.currentxy is not None:
            x1, y1 = self.currentxy
            x2, y2 = self.cornerxy or self.currentxy
            if x1 > x2:
                x1, x2 = x2, x1
            if y1 > y2:
                y1, y2 = y2, y1
            for (x, y), cell in self.gridcells.items():
                if x1 <= x <= x2 and y1 <= y <= y2:
                    cell['bg'] = 'white'

    def return_event(self, event):
        "Callback for the Return key."
        self.change_cell()
        x, y = self.currentxy
        self.setcurrent(x, y+1)
        return "break"

    def shift_return_event(self, event):
        "Callback for the Return key with Shift modifier."
        self.change_cell()
        x, y = self.currentxy
        self.setcurrent(x, max(1, y-1))
        return "break"

    def tab_event(self, event):
        "Callback for the Tab key."
        self.change_cell()
        x, y = self.currentxy
        self.setcurrent(x+1, y)
        return "break"

    def shift_tab_event(self, event):
        "Callback for the Tab key with Shift modifier."
        self.change_cell()
        x, y = self.currentxy
        self.setcurrent(max(1, x-1), y)
        return "break"

    def change_cell(self):
        "Set the current cell from the entry widget."
        x, y = self.currentxy
        text = self.entry.get()
        cell = None
        if text.startswith('='):
            cell = FormulaCell(text[1:])
        else:
            for cls in int, int, float, complex:
                try:
                    value = cls(text)
                except:
                    continue
                else:
                    cell = NumericCell(value)
                    break
        if cell is None and text:
            cell = StringCell(text)
        if cell is None:
            self.sheet.clearcell(x, y)
        else:
            self.sheet.setcell(x, y, cell)
        self.sync()

    def sync(self):
        "Fill the GUI cells from the sheet cells."
        self.sheet.recalc()
        for (x, y), gridcell in self.gridcells.items():
            if x == 0 or y == 0:
                continue
            cell = self.sheet.getcell(x, y)
            if cell is None:
                gridcell['text'] = ""
            else:
                if hasattr(cell, 'format'):
                    text, alignment = cell.format()
                else:
                    text, alignment = str(cell), LEFT
                gridcell['text'] = text
                gridcell['anchor'] = align2anchor[alignment]


def test_basic():
    "Basic non-gui self-test."
    a = Sheet()
    for x in range(1, 11):
        for y in range(1, 11):
            if x == 1:
                cell = NumericCell(y)
            elif y == 1:
                cell = NumericCell(x)
            else:
                c1 = cellname(x, 1)
                c2 = cellname(1, y)
                formula = "%s*%s" % (c1, c2)
                cell = FormulaCell(formula)
            a.setcell(x, y, cell)
##    if os.path.isfile("sheet1.xml"):
##        print "Loading from sheet1.xml"
##        a.load("sheet1.xml")
    a.display()
    a.save("sheet1.xml")

def test_gui():
    "GUI test."
    if sys.argv[1:]:
        filename = sys.argv[1]
    else:
        filename = "sheet1.xml"
    g = SheetGUI(filename)
    g.root.mainloop()

if __name__ == '__main__':
    #test_basic()
    test_gui()
