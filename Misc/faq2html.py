#! /usr/bin/env python

# A somewhat-generalized FAQ-to-HTML converter (by Ka-Ping Yee, 10 Sept 96)

# Reads a text file given on standard input or named as first argument, and
# generates HTML 2.0 on standard output.  Recognizes these constructions:
#
#     HTML element               pattern at the beginning of a line
#
#     section heading            (<number><period>)+<space>
#     numbered list element      <1-2 spaces>(<number><period>)+<space>
#     unnumbered list element    <0-2 spaces><hyphen or asterisk><space>
#     preformatted section       <more than two spaces>
#
# Heading level is determined by the number of (<number><period>) segments.
# Blank lines force a separation of elements; if none of the above four
# types is indicated, a new paragraph begins.  A line beginning with many
# spaces is interpreted as a continuation (instead of preformatted) after
# a list element.  Headings are anchored; paragraphs starting with "Q." are
# emphasized, and those marked with "A." get their first sentence emphasized.
#
# Hyperlinks are created from references to:
#     URLs, explicitly marked using <URL:scheme://host...> 
#     other questions, of the form "question <number>(<period><number>)*"
#     sections, of the form "section <number>".

import sys, string, regex, regsub, regex_syntax
regex.set_syntax(regex_syntax.RE_SYNTAX_AWK)

# --------------------------------------------------------- regular expressions
orditemprog = regex.compile('  ?([1-9][0-9]*\.)+ +')
itemprog = regex.compile(' ? ?[-*] +')
headingprog = regex.compile('([1-9][0-9]*\.)+ +')
prefmtprog = regex.compile('   ')
blankprog = regex.compile('^[ \t\r\n]$')
questionprog = regex.compile(' *Q\. +')
answerprog = regex.compile(' *A\. +')
sentprog = regex.compile('(([^.:;?!(]|[.:;?!][^ \t\r\n])+[.:;?!]?)')

mailhdrprog = regex.compile('^(Subject|Newsgroups|Followup-To|From|Reply-To'
    '|Approved|Archive-Name|Version|Last-Modified): +', regex.casefold)
urlprog = regex.compile('&lt;URL:([^&]+)&gt;')
addrprog = regex.compile('&lt;([^>@:]+@[^&@:]+)&gt;')
qrefprog = regex.compile('question +([1-9](\.[0-9]+)*)')
srefprog = regex.compile('section +([1-9][0-9]*)')
entityprog = regex.compile('[&<>]')

# ------------------------------------------------------------ global variables
body = []
ollev = ullev = 0
element = content = secnum = version = ''

# ----------------------------------------------------- for making nested lists
def dnol():
    global body, ollev
    ollev = ollev + 1
    if body[-1] == '</li>': del body[-1]
    body.append('<ol>')

def upol(): 
    global body, ollev
    ollev = ollev - 1
    body.append(ollev and '</ol></li>' or '</ol>')

# --------------------------------- output one element and convert its contents
def spew(clearol=0, clearul=0):
    global content, body, ollev, ullev

    if content:
        if entityprog.search(content) > -1:
            content = regsub.gsub('&', '&amp;', content)
            content = regsub.gsub('<', '&lt;', content)
            content = regsub.gsub('>', '&gt;', content)

        n = questionprog.match(content)
        if n > 0:
            content = '<em>' + content[n:] + '</em>'
            if ollev:                       # question reference in index
                fragid = regsub.gsub('^ +|\.? +$', '', secnum)
                content = '<a href="#%s">%s</a>' % (fragid, content)

        if element[0] == 'h':               # heading in the main text
            fragid = regsub.gsub('^ +|\.? +$', '', secnum)
            content = secnum + '<a name="%s">%s</a>' % (fragid, content)

        n = answerprog.match(content)
        if n > 0:                           # answer paragraph
            content = regsub.sub(sentprog, '<strong>\\1</strong>', content[n:])

        body.append('<' + element + '>' + content)
        body.append('</' + element + '>')
        content = ''

    while clearol and ollev: upol()
    if clearul and ullev: body.append('</ul>'); ullev = 0

# ---------------------------------------------------------------- main program
faq = len(sys.argv)>1 and sys.argv[1] and open(sys.argv[1]) or sys.stdin
lines = faq.readlines()

for line in lines:
    if line[2:9] == '=======':              # <hr> will appear *before*
        body.append('<hr>')                 # the underlined heading
        continue

    n = orditemprog.match(line)
    if n > 0:                               # make ordered list item
        spew(0, 'clear ul')
        secnum = line[:n]
        level = string.count(secnum, '.')
        while level > ollev: dnol()
        while level < ollev: upol()
        element, content = 'li', line[n:]
        continue

    n = itemprog.match(line)
    if n > 0:                               # make unordered list item
        spew('clear ol', 0)
        if ullev == 0: body.append('<ul>'); ullev = 1
        element, content = 'li', line[n:]
        continue

    n = headingprog.match(line)
    if n > 0:                               # make heading element
        spew('clear ol', 'clear ul')
        secnum = line[:n]
        sys.stderr.write(line)
        element, content = 'h%d' % string.count(secnum, '.'), line[n:]
        continue

    n = 0
    if not secnum:                          # haven't hit body yet
        n = mailhdrprog.match(line) 
        v = version and -1 or regex.match('Version: ', line)
        if v > 0 and not version: version = line[v:]
    if n <= 0 and element != 'li':          # not pre if after a list item
        n = prefmtprog.match(line)
    if n > 0:                               # make preformatted element
        if element == 'pre':
            content = content + line
        else: 
            spew('clear ol', 'clear ul')
            element, content = 'pre', line
        continue

    if blankprog.match(line) > 0:           # force a new element
        spew()
        element = ''
    elif element:                           # continue current element
        content = content + line
    else:                                   # no element; make paragraph
        spew('clear ol', 'clear ul')
        element, content = 'p', line

spew()										# output last element

body = string.joinfields(body, '')
body = regsub.gsub(urlprog, '<a href="\\1">\\1</a>', body)
body = regsub.gsub(addrprog, '<a href="mailto:\\1">\\1</a>', body)
body = regsub.gsub(qrefprog, '<a href="#\\1">question \\1</a>', body)
body = regsub.gsub(srefprog, '<a href="#\\1">section \\1</a>', body)

print '<!doctype html public "-//IETF//DTD HTML 2.0//EN"><html>'
print '<head><title>Python Frequently-Asked Questions v' + version
print "</title></head><body>(This file was generated using Ping's"
print '<a href="faq2html.py">faq2html.py</a>.)'
print body + '</body></html>'
