import regex
import regsub
import string
import sys


AS_IS = None

whitespace = '[' + string.whitespace + ']+'


class AbstractFormatter:

    def __init__(self, writer):
	self.writer = writer		# Output device
	self.font_stack = []		# Font state
	self.margin_stack = []		# Margin state
	self.spacing = None		# Vertical spacing state
	self.style_stack = []		# Other state, e.g. color
	self.nospace = 1 # Should leading space be suppressed
	self.softspace = 0 # Should a space be inserted

    def end_paragraph(self, blankline):
	if not self.nospace:
	    self.writer.send_paragraph(blankline)
	self.nospace = 1
	self.softspace = 0

    def add_line_break(self):
	self.writer.send_line_break()
	self.nospace = 1
	self.softspace = 0

    def add_hor_rule(self):
	self.writer.send_hor_rule()
	self.nospace = 1
	self.softspace = 0

    def add_label_data(self, format, counter):
	data = self.format_counter(format, counter)
	self.writer.send_label_data(data)

    def format_counter(self, format, counter):
	if counter <= 0:
	    return format
        label = ''
        for c in format:
            try:
                if c == '1':
		    c = '%d' % counter
                elif c in 'aA':
		    c = self.format_letter(c, counter)
                elif c in 'iI':
		    c = self.format_roman(c, counter)
            except:
                pass
            label = label + c
        return label

    def format_letter(self, case, counter):
	label = ''
	while counter > 0:
	    counter, x = divmod(counter-1, 26)
	    s = chr(ord(case) + x)
	    label = s + label
	return label

    def format_roman(self, case, counter):
        ones = ['i', 'x', 'c', 'm']
        fives = ['v', 'l', 'd']
        label = ''
        index = 0
	# This will die of IndexError when counter is too big
        while counter > 0:
            counter, x = divmod(counter, 10)
            if x == 9:
                s = ones[index] + ones[index+1]
            elif x == 4:
                s = ones[index] + fives[index]
            else:
                if x >= 5:
                    s = fives[index]
                    x = x-5
                else:
                    s = ''
                s = s + ones[index]*x
            label = s + label
            index = index + 1
        if case == 'I': label = string.upper(label)
        return label

    def add_flowing_data(self, data):
	if not data: return
	data = regsub.gsub(whitespace, ' ', data)
	if self.nospace and data[0] == ' ':
	    data = data[1:]
	    if not data: return
	elif self.softspace and data[0] != ' ':
	    data = ' ' + data
	self.nospace = self.softspace = 0
	if data[-1] == ' ':
	    data = data[:-1]
	    self.softspace = 1
	self.writer.send_flowing_data(data)

    def add_literal_data(self, data):
	if self.softspace and data[:1] != '\n':
	    data = ' ' + data
	self.nospace = self.softspace = 0
	self.writer.send_literal_data(data)

    def push_font(self, (size, i, b, tt)):
	if self.font_stack:
	    csize, ci, cb, ctt = self.font_stack[-1]
	    if size is AS_IS: size = csize
	    if i is AS_IS: i = ci
	    if b is AS_IS: b = cb
	    if tt is AS_IS: tt = ctt
	font = (size, i, b, tt)
	self.font_stack.append(font)
	self.writer.new_font(font)

    def pop_font(self):
	if self.font_stack:
	    del self.font_stack[-1]
	if self.font_stack:
	    font = self.font_stack[-1]
	else:
	    font = None
	self.writer.new_font(font)

    def push_margin(self, margin):
	self.margin_stack.append(margin)
	self.writer.new_margin(margin, len(self.margin_stack))

    def pop_margin(self):
	if self.margin_stack:
	    del self.margin_stack[-1]
	if self.margin_stack:
	    margin = self.margin_stack[-1]
	else:
	    margin = None
	self.writer.new_margin(margin, len(self.margin_stack))

    def set_spacing(self, spacing):
	self.spacing = spacing
	self.writer.new_spacing(spacing)

    def push_style(self, style):
	self.style_stack.append(style)
	self.writer.new_styles(tuple(self.style_stack))

    def pop_style(self):
	if self.style_stack:
	    del self.style_stack[-1]
	self.writer.new_styles(tuple(self.style_stack))


class AbstractWriter:

    def __init__(self):
	pass

    def new_font(self, font):
	print "new_font(%s)" % `font`

    def new_margin(self, margin, level):
	print "new_margin(%s, %d)" % (`margin`, level)

    def new_spacing(self, spacing):
	print "new_spacing(%s)" % `spacing`

    def new_styles(self, styles):
	print "new_styles(%s)" % `styles`

    def send_paragraph(self, blankline):
	print "send_paragraph(%s)" % `blankline`

    def send_line_break(self):
	print "send_line_break()"

    def send_hor_rule(self):
	print "send_hor_rule()"

    def send_label_data(self, data):
	print "send_label_data(%s)" % `data`

    def send_flowing_data(self, data):
	print "send_flowing_data(%s)" % `data`

    def send_literal_data(self, data):
	print "send_literal_data(%s)" % `data`


class DumbWriter(AbstractWriter):

    def __init__(self, file=None, maxcol=72):
	self.file = file or sys.stdout
	self.maxcol = maxcol
	AbstractWriter.__init__(self)
	self.reset()

    def reset(self):
	self.col = 0
	self.atbreak = 0

    def send_paragraph(self, blankline):
	self.file.write('\n' + '\n'*blankline)
	self.col = 0
	self.atbreak = 0

    def send_line_break(self):
	self.file.write('\n')
	self.col = 0
	self.atbreak = 0

    def send_hor_rule(self):
	self.file.write('\n')
	self.file.write('-'*self.maxcol)
	self.file.write('\n')
	self.col = 0
	self.atbreak = 0

    def send_literal_data(self, data):
	self.file.write(data)
	i = string.rfind(data, '\n')
	if i >= 0:
	    self.col = 0
	    data = data[i+1:]
	data = string.expandtabs(data)
	self.col = self.col + len(data)
	self.atbreak = 0

    def send_flowing_data(self, data):
	if not data: return
	atbreak = self.atbreak or data[0] in string.whitespace
	col = self.col
	maxcol = self.maxcol
	write = self.file.write
	for word in string.split(data):
	    if atbreak:
		if col + len(word) >= maxcol:
		    write('\n')
		    col = 0
		else:
		    write(' ')
		    col = col + 1
	    write(word)
	    col = col + len(word)
	    atbreak = 1
	self.col = col
	self.atbreak = data[-1] in string.whitespace


def test(file = None):
    w = DumbWriter()
    f = AbstractFormatter(w)
    if file:
	fp = open(file)
    elif sys.argv[1:]:
	fp = open(sys.argv[1])
    else:
	fp = sys.stdin
    while 1:
	line = fp.readline()
	if not line:
	    break
	if line == '\n':
	    f.end_paragraph(1)
	else:
	    f.add_flowing_data(line)
    f.end_paragraph(0)


if __name__ == '__main__':
    test()
