#! /usr/bin/env python

# Renumber the Python FAQ

import string
import regex
import sys
import os

FAQ = 'FAQ'

chapterprog = regex.compile('^\([1-9][0-9]*\)\. ')
questionprog = regex.compile('^\([1-9][0-9]*\)\.\([1-9][0-9]*\)\. ')
newquestionprog = regex.compile('^Q\. ')
blankprog = regex.compile('^[ \t]*$')
indentedorblankprog = regex.compile('^\([ \t]+\|[ \t]*$\)')

def main():
	print 'Reading lines...'
	lines = open(FAQ, 'r').readlines()
	print 'Renumbering in memory...'
	oldlines = lines[:]
	after_blank = 1
	chapter = 0
	question = 0
	chapters = ['\n']
	questions = []
	for i in range(len(lines)):
		line = lines[i]
		if after_blank:
			n = chapterprog.match(line)
			if n >= 0:
				chapter = chapter + 1
				question = 0
				line = `chapter` + '. ' + line[n:]
				lines[i] = line
				chapters.append(' ' + line)
				questions.append('\n')
				questions.append(' ' + line)
				afterblank = 0
				continue
			n = questionprog.match(line)
			if n < 0: n = newquestionprog.match(line) - 3
			if n >= 0:
				question = question + 1
				number = '%d.%d. '%(chapter, question)
				line = number + line[n:]
				lines[i] = line
				questions.append('  ' + line)
				# Add up to 4 continuations of the question
				n = len(number)
				for j in range(i+1, i+5):
					if blankprog.match(lines[j]) >= 0:
						break
					questions.append(' '*(n+2) + lines[j])
				afterblank = 0
				continue
		afterblank = (blankprog.match(line) >= 0)
	print 'Inserting list of chapters...'
	chapters.append('\n')
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
	questions.append('\n')
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
	if lines == oldlines:
		print 'No changes.'
		return
	print 'Writing new file...'
	f = open(FAQ + '.new', 'w')
	for line in lines:
		f.write(line)
	f.close()
	print 'Making backup...'
	os.rename(FAQ, FAQ + '~')
	print 'Moving new file...'
	os.rename(FAQ + '.new', FAQ)
	print 'Done.'

main()
