#!/usr/bin/python
# Module and documentation by Eric S. Raymond, 21 Dec 1998 

import sys, os, string

class shlex:
    "A lexical analyzer class for simple shell-like syntaxes." 
    def __init__(self, instream=None):
        if instream:
            self.instream = instream
        else:
            self.instream = sys.stdin
        self.commenters = '#'
        self.wordchars = 'abcdfeghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_'
        self.whitespace = ' \t\r\n'
        self.quotes = '\'"'
        self.state = ' '
        self.pushback = [];
        self.lineno = 1
        self.debug = 0
        self.token = ''

    def push_token(self, tok):
        "Push a token onto the stack popped by the get_token method"
        if (self.debug >= 1):
            print "Pushing " + tok
        self.pushback = [tok] + self.pushback;

    def get_token(self):
        "Get a token from the input stream (or from stack if it's monempty)"
        if self.pushback:
            tok = self.pushback[0]
            self.pushback = self.pushback[1:]
            if (self.debug >= 1):
                print "Popping " + tok
            return tok
        tok = ''
        while 1:
            nextchar = self.instream.read(1);
            if nextchar == '\n':
                self.lineno = self.lineno + 1
            if self.debug >= 3:
                print "In state " + repr(self.state) + " I see character: " + repr(nextchar) 
            if self.state == None:
                return ''
            elif self.state == ' ':
                if not nextchar:
                    self.state = None;	# end of file
                    break
                elif nextchar in self.whitespace:
                    if self.debug >= 2:
                        print "I see whitespace in whitespace state"
                    if self.token:
                        break	# emit current token
                    else:
                        continue
                elif nextchar in self.commenters:
                    self.instream.readline()
                    self.lineno = self.lineno + 1
                elif nextchar in self.wordchars:
                    self.token = nextchar
                    self.state = 'a'
                elif nextchar in self.quotes:
                    self.token = nextchar
                    self.state = nextchar
                else:
                    self.token = nextchar
                    if self.token:
                        break	# emit current token
                    else:
                        continue
            elif self.state in self.quotes:
                self.token = self.token + nextchar
                if nextchar == self.state:
                    self.state = ' '
                    break
            elif self.state == 'a':
                if not nextchar:
                    self.state = None;	# end of file
                    break
                elif nextchar in self.whitespace:
                    if self.debug >= 2:
                        print "I see whitespace in word state"
                    self.state = ' '
                    if self.token:
                        break	# emit current token
                    else:
                        continue
                elif nextchar in self.commenters:
                    self.instream.readline()
                    self.lineno = self.lineno + 1
                elif nextchar in self.wordchars or nextchar in self.quotes:
                    self.token = self.token + nextchar
                else:
                    self.pushback = [nextchar] + self.pushback
                    if self.debug >= 2:
                        print "I see punctuation in word state"
                    state = ' '
                    if self.token:
                        break	# emit current token
                    else:
                        continue
                
        result = self.token
        self.token = ''
        if self.debug >= 1:
            print "Token: " + result
        return result

if __name__ == '__main__': 

    lexer = shlex()
    while 1:
        tt = lexer.get_token()
        if tt != None:
            print "Token: " + repr(tt)
        else:
            break

