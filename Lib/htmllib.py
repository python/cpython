# New HTML class

# XXX Check against HTML 2.0 spec

# XXX reorder methods according to hierarchy
# - html structure: head, body, title, isindex
# - headers
# - lists, items
# - paragraph styles
# - forms
# - character styles
# - images
# - bookkeeping
# - output generation


import sys
import regsub
import string
from sgmllib import SGMLParser


ROMAN = 0
ITALIC = 1
BOLD = 2
FIXED = 3


class HTMLParser(SGMLParser):

    def __init__(self):
	SGMLParser.__init__(self)
	self.savedata = None
	self.isindex = 0
	self.title = ''
	self.para = None
	self.lists = []
	self.styles = []
	self.nofill = 0
	self.nospace = 1
	self.softspace = 0

    # --- Data

    def handle_image(self, src, alt):
	self.handle_data(alt)

    def handle_data(self, data):
	if self.nofill:
	    self.handle_literal(data)
	    return
	data = regsub.gsub('[ \t\n\r]+', ' ', data)
	if self.nospace and data[:1] == ' ': data = data[1:]
	if not data: return
	self.nospace = 0
	if self.softspace and data[:1] != ' ': data = ' ' + data
	if data[-1:] == ' ':
	    data = data[:-1]
	    self.softspace = 1
	self.output_data(data)

    def handle_literal(self, data):
	self.nospace = 0
	self.softspace = 0
	self.output_data(data)

    def output_data(self, data):
	if self.savedata is not None:
	    self.savedata = self.savedata + data
	else:
	    self.write_data(data)

    def write_data(self, data):
	sys.stdout.write(data)

    def save_bgn(self):
	self.savedata = ''
	self.nospace = 1
	self.softspace = 0

    def save_end(self):
	saved = self.savedata
	self.savedata = None
	self.nospace = 1
	self.softspace = 0
	return saved

    def new_para(self):
	pass

    def new_style(self):
	pass

    # --- Generic style changes

    def para_bgn(self, tag):
	if not self.nospace:
	    self.handle_literal('\n')
	    self.nospace = 1
	    self.softspace = 0
	if tag is not None:
	    self.para = tag
	self.new_para()

    def para_end(self):
	self.para_bgn('')

    def push_list(self, tag):
	self.lists.append(tag)
	self.para_bgn(None)

    def pop_list(self):
	del self.lists[-1]
	self.para_end()

    def literal_bgn(self, tag, attrs):
	self.para_bgn(tag)

    def literal_end(self, tag):
	self.para_end()

    def push_style(self, tag):
	self.styles.append(tag)
	self.new_style()

    def pop_style(self):
	del self.styles[-1]
	self.new_style()

    def anchor_bgn(self, href, name, type):
	self.push_style(href and 'a' or None)

    def anchor_end(self):
	self.pop_style()

    # --- Top level tags

    def start_html(self, attrs): pass
    def end_html(self): pass

    def start_head(self, attrs): pass
    def end_head(self): pass

    def start_body(self, attrs): pass
    def end_body(self): pass

    def do_isindex(self, attrs):
	self.isindex = 1

    def start_title(self, attrs):
	self.save_bgn()

    def end_title(self):
	self.title = self.save_end()

    # --- Old HTML 'literal text' tags

    def start_listing(self, attrs):
	self.setliteral('listing')
	self.literal_bgn('listing', attrs)

    def end_listing(self):
	self.literal_end('listing')

    def start_xmp(self, attrs):
	self.setliteral('xmp')
	self.literal_bgn('xmp', attrs)

    def end_xmp(self):
	self.literal_end('xmp')

    def do_plaintext(self, attrs):
	self.setnomoretags()
	self.literal_bgn('plaintext', attrs)

    # --- Anchors

    def start_a(self, attrs):
	href = ''
	name = ''
	type = ''
	for attrname, value in attrs:
	    if attrname == 'href':
		href = value
	    if attrname == 'name':
		name = value
	    if attrname == 'type':
		type = string.lower(value)
	if not (href or name):
	    return
	self.anchor_bgn(href, name, type)

    def end_a(self):
	self.anchor_end()

    # --- Paragraph tags

    def do_p(self, attrs):
	self.para_bgn(None)

    def do_br(self, attrs):
	self.handle_literal('\n')
	self.nospace = 1
	self.softspace = 0

    def do_hr(self, attrs):
	self.para_bgn(None)
	self.handle_literal('-'*40)
	self.para_end()

    def start_h1(self, attrs):
	self.para_bgn('h1')

    def start_h2(self, attrs):
	self.para_bgn('h2')

    def start_h3(self, attrs):
	self.para_bgn('h3')

    def start_h4(self, attrs):
	self.para_bgn('h4')

    def start_h5(self, attrs):
	self.para_bgn('h5')

    def start_h6(self, attrs):
	self.para_bgn('h6')

    def end_h1(self):
	self.para_end()

    end_h2 = end_h1
    end_h3 = end_h2
    end_h4 = end_h3
    end_h5 = end_h4
    end_h6 = end_h5

    def start_ul(self, attrs):
	self.para_bgn(None)
	self.push_list('ul')

    def start_ol(self, attrs):
	self.para_bgn(None)
	self.push_list('ol')

    def end_ul(self):
	self.pop_list()
	self.para_end()

    def do_li(self, attrs):
	self.para_bgn('li%d' % len(self.lists))

    start_dir = start_menu = start_ul
    end_dir = end_menu = end_ol = end_ul

    def start_dl(self, attrs):
	self.para_bgn(None)
	self.push_list('dl')

    def end_dl(self):
	self.pop_list()
	self.para_end()

    def do_dt(self, attrs):
	self.para_bgn('dt%d' % len(self.lists))

    def do_dd(self, attrs):
	self.para_bgn('dd%d' % len(self.lists))

    def start_address(self, attrs):
	self.para_bgn('address')

    def end_address(self):
	self.para_end()

    def start_pre(self, attrs):
	self.para_bgn('pre')
	self.nofill = self.nofill + 1

    def end_pre(self):
	self.nofill = self.nofill - 1
	self.para_end()

    start_typewriter = start_pre
    end_typewriter = end_pre

    def do_img(self, attrs):
	src = ''
	alt = ' (image) '
	for attrname, value in attrs:
	    if attrname == 'alt':
		alt = value
	    if attrname == 'src':
		src = value
	self.handle_image(src, alt)

    # --- Character tags -- physical styles

    def start_tt(self, attrs): self.push_style(FIXED)
    def end_tt(self): self.pop_style()

    def start_b(self, attrs): self.push_style(BOLD)
    def end_b(self): self.pop_style()

    def start_i(self, attrs): self.push_style(ITALIC)
    def end_i(self): self.pop_style()

    def start_u(self, attrs): self.push_style(ITALIC) # Underline???
    def end_u(self): self.pop_style()

    def start_r(self, attrs): self.push_style(ROMAN) # Not official
    def end_r(self): self.pop_style()

    # --- Charaacter tags -- logical styles

    start_em = start_i
    end_em = end_i

    start_strong = start_b
    end_strong = end_b

    start_code = start_tt
    end_code = end_tt

    start_samp = start_tt
    end_samp = end_tt

    start_kbd = start_tt
    end_kbd = end_tt

    start_file = start_tt # unofficial
    end_file = end_tt

    start_var = start_i
    end_var = end_i

    start_dfn = start_i
    end_dfn = end_i

    start_cite = start_i
    end_cite = end_i

    start_hp1 = start_i
    end_hp1 = start_i

    start_hp2 = start_b
    end_hp2 = end_b

    # --- Form tags

    def start_form(self, attrs):
	self.para_bgn(None)

    def end_form(self):
	self.para_end()

    # --- Unhandled tags

    def unknown_starttag(self, tag, attrs):
	pass

    def unknown_endtag(self, tag):
	pass


def test():
    file = 'test.html'
    f = open(file, 'r')
    data = f.read()
    f.close()
    p = HTMLParser()
    p.feed(data)
    p.close()


if __name__ == '__main__':
    test()
