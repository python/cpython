#! /usr/local/bin/python

# Convert the Python FAQ to HTML

import string
import regex
import regsub
import sys
import os

FAQ = 'FAQ'

chapterprog = regex.compile('^\([1-9][0-9]*\)\. ')
questionprog = regex.compile('^\([1-9][0-9]*\)\.\([1-9][0-9]*\)\. ')
newquestionprog = regex.compile('^Q\. ')
blankprog = regex.compile('^[ \t]*$')
indentedorblankprog = regex.compile('^\([ \t]+\|[ \t]*$\)')
underlineprog = regex.compile('^==*$')
eightblanksprog = regex.compile('^\(        \| *\t\)')
mailheaderprog = regex.compile('^\(Subject\|Newsgroups\|Followup-To\|From\|Reply-To\|Approved\|Archive-name\|Version\|Last-modified\): +')
urlprog = regex.compile('<URL:\([^>]*\)>')
ampprog = regex.compile('&')
aprog = regex.compile('^A\. +')
qprog = regex.compile('>Q\. +')
qrefprog = regex.compile('question +\([0-9]\.[0-9]+\)')
versionprog = regex.compile('^Version: ')
emailprog = regex.compile('<\([^>@:]+@[^>@:]+\)>')

def main():
    print 'Reading lines...'
    lines = open(FAQ, 'r').readlines()
    print 'Renumbering in memory...'
    oldlines = lines[:]
    after_blank = 1
    chapter = 0
    question = 0
    chapters = ['<OL>']
    questions = ['<OL>']
    for i in range(len(lines)):
	line = lines[i]
	if after_blank:
	    n = chapterprog.match(line)
	    if n >= 0:
		chapter = chapter + 1
		if chapter != 1:
		    questions.append('</UL>\n')
		question = 0
		lines[i] = '<H2>' + line[n:-1] + '</H2>\n'
		chapters.append('<LI> ' + line[n:])
		questions.append('<LI> ' + line[n:])
		questions.append('<UL>\n')
		afterblank = 0
		continue
	    n = underlineprog.match(line)
	    if n >= 0:
		lines[i] = ''
		continue
	    n = questionprog.match(line)
	    if n < 0: n = newquestionprog.match(line) - 3
	    if n >= 0:
		question = question + 1
		number = '%d.%d'%(chapter, question)
		lines[i] = '<A NAME="' + number + '"><H3>' + line[n:]
		questions.append('<LI><A HREF="#' + \
				 number + '">' + line[n:])
		# Add up to 4 continuations of the question
		n = len(number)
		for j in range(i+1, i+5):
		    if blankprog.match(lines[j]) >= 0:
			lines[j-1] = lines[j-1] + '</H3></A>'
			questions[-1] = \
			      questions[-1][:-1] + '</A>\n'
			break
		    questions.append(' '*(n+2) + lines[j])
		afterblank = 0
		continue
	afterblank = (blankprog.match(line) >= 0)
    print 'Inserting list of chapters...'
    chapters.append('</OL>\n')
    for i in range(len(lines)):
	line = lines[i]
	if regex.match(
		  '^This FAQ is divided in the following chapters',
		  line) >= 0:
	    i = i+1
	    while 1:
		line = lines[i]
		if indentedorblankprog.match(line) < 0:
		    break
		del lines[i]
	    lines[i:i] = chapters
	    break
    else:
	print '*** Can\'t find header for list of chapters'
	print '*** Chapters found:'
	for line in chapters: print line,
    print 'Inserting list of questions...'
    questions.append('</UL></OL>\n')
    for i in range(len(lines)):
	line = lines[i]
	if regex.match('^Here.s an overview of the questions',
		  line) >= 0:
	    i = i+1
	    while 1:
		line = lines[i]
		if indentedorblankprog.match(line) < 0:
		    break
		del lines[i]
	    lines[i:i] = questions
	    break
    else:
	print '*** Can\'t find header for list of questions'
	print '*** Questions found:'
	for line in questions: print line,
    # final cleanup
    print "Final cleanup..."
    doingpre = 0
    for i in range(len(lines)):
	# set lines indented by >= 8 spaces using PRE
	# blank lines either terminate PRE or separate paragraphs
	n = eightblanksprog.match(lines[i])
	if n < 0: n = mailheaderprog.match(lines[i])
	if n >= 0:
	    if versionprog.match(lines[i]) > 0:
		version = string.split(lines[i])[1]
	    if doingpre == 0:
		lines[i] = '<PRE>\n' + lines[i]
		doingpre = 1
		continue
	n = blankprog.match(lines[i])
	if n >= 0:
	    # print '*** ', lines[i-1], doingpre
	    if doingpre == 1:
		lines[i] = '</PRE><P>\n'
		doingpre = 0
	    else:
		lines[i] = '<P>\n'
	    continue

	# & -> &amp;
	n = ampprog.search(lines[i])
	if n >= 0:
	    lines[i] = regsub.gsub(ampprog, '&amp;', lines[i])
	    # no continue - there might be other changes to the line...

	# zap all the 'Q.' and 'A.' leaders - what happened to the
	# last couple?
	n = qprog.search(lines[i])
	if n >= 0:
	    lines[i] = regsub.sub(qprog, '>', lines[i])
	    # no continue - there might be other changes to the line...

	n = aprog.search(lines[i])
	if n >= 0:
	    lines[i] = regsub.sub(aprog, '', lines[i])
	    # no continue - there might be other changes to the line...

	# patch up hard refs to questions
	n = qrefprog.search(lines[i])
	if n >= 0:
	    lines[i] = regsub.sub(qrefprog,
				  '<A HREF="#\\1">question \\1</A>', lines[i])
	    # no continue - there might be other changes to the line...

	# make <URL:...> into actual links
	n = urlprog.search(lines[i])
	if n >= 0:
	    lines[i] = regsub.gsub(urlprog, '<A HREF="\\1">\\1</A>', lines[i])
	    # no continue - there might be other changes to the line...

	# make <user@host.domain> into <mailto:...> links
	n = emailprog.search(lines[i])
	if n >= 0:
	    lines[i] = regsub.gsub(emailprog,
				   '<A HREF="mailto:\\1">\\1</A>', lines[i])
	    # no continue - there might be other changes to the line...

    lines[0:0] = ['<HTML><HEAD><TITLE>Python Frequently Asked Questions v',
		  version,
		  '</TITLE>\n',
		  '</HEAD><body>\n',
		  '(This file was generated using',
		  '<A HREF="faq2html.py">faq2html.py</A>.)<P>\n']
    lines.append('<P></BODY></HTML>\n')

    print 'Writing html file...'
    f = open(FAQ + '.html', 'w')
    for line in lines:
	f.write(line)
    f.close()
    print 'Done.'

main()
