# A new Feed-style Parser

from email import Errors, Message
import re

NLCRE = re.compile('\r\n|\r|\n')

EMPTYSTRING = ''
NL = '\n'

NeedMoreData = object()

class FeedableLumpOfText:
    "A file-like object that can have new data loaded into it"

    def __init__(self):
        self._partial = ''
        self._done = False
        # _pending is a list of lines, in reverse order
        self._pending = []

    def readline(self):
        """ Return a line of data.

            If data has been pushed back with unreadline(), the most recently
            returned unreadline()d data will be returned.
        """
        if not self._pending:
            if self._done:
                return ''
            return NeedMoreData
        return self._pending.pop()

    def unreadline(self, line):
        """ Push a line back into the object. 
        """
        self._pending.append(line)

    def peekline(self):
        """ Non-destructively look at the next line """
        if not self._pending:
            if self._done:
                return ''
            return NeedMoreData
        return self._pending[-1]


    # for r in self._input.readuntil(regexp):
    #     if r is NeedMoreData:
    #         yield NeedMoreData
    #     preamble, matchobj = r
    def readuntil(self, matchre, afterblank=False, includematch=False):
        """ Read a line at a time until we get the specified RE. 

            Returns the text up to (and including, if includematch is true) the 
            matched text, and the RE match object. If afterblank is true, 
            there must be a blank line before the matched text. Moves current 
            filepointer to the line following the matched line. If we reach 
            end-of-file, return what we've got so far, and return None as the
            RE match object.
        """
        prematch = []
        blankseen = 0
        while 1: 
            if not self._pending:
                if self._done:
                    # end of file
                    yield EMPTYSTRING.join(prematch), None
                else:
                    yield NeedMoreData
                continue
            line = self._pending.pop()
            if afterblank:
                if NLCRE.match(line):
                    blankseen = 1
                    continue
                else:
                    blankseen = 0
            m = matchre.match(line)
            if (m and not afterblank) or (m and afterblank and blankseen):
                if includematch:
                    prematch.append(line)
                yield EMPTYSTRING.join(prematch), m
            prematch.append(line)


    NLatend = re.compile('(\r\n|\r|\n)$').match
    NLCRE_crack = re.compile('(\r\n|\r|\n)')

    def push(self, data):
        """ Push some new data into this object """
        # Handle any previous leftovers
        data, self._partial = self._partial+data, ''
        # Crack into lines, but leave the newlines on the end of each
        lines = self.NLCRE_crack.split(data)
        # The *ahem* interesting behaviour of re.split when supplied
        # groups means that the last element is the data after the 
        # final RE. In the case of a NL/CR terminated string, this is
        # the empty string.
        self._partial = lines.pop()
        o = []
        for i in range(len(lines) / 2):
            o.append(EMPTYSTRING.join([lines[i*2], lines[i*2+1]]))
        self.pushlines(o)
    
    def pushlines(self, lines):
        """ Push a list of new lines into the object """
        # Reverse and insert at the front of _pending
        self._pending[:0] = lines[::-1]

    def end(self):
        """ There is no more data """
        self._done = True

    def is_done(self):
        return self._done

    def __iter__(self):
        return self

    def next(self):
        l = self.readline()
        if l == '': 
            raise StopIteration
        return l

class FeedParser:
    "A feed-style parser of email. copy docstring here"

    def __init__(self, _class=Message.Message):
        "fnord fnord fnord"
        self._class = _class
        self._input = FeedableLumpOfText()
        self._root = None
        self._objectstack = []
        self._parse = self._parsegen().next

    def end(self):
        self._input.end()
        self._call_parse()
        return self._root

    def feed(self, data):
        self._input.push(data)
        self._call_parse()

    def _call_parse(self):
        try:
            self._parse()
        except StopIteration:
            pass

    headerRE = re.compile(r'^(From |[-\w]{2,}:|[\t ])')

    def _parse_headers(self,headerlist):
        # Passed a list of strings that are the headers for the 
        # current object
        lastheader = ''
        lastvalue = []


        for lineno, line in enumerate(headerlist):
            # Check for continuation
            if line[0] in ' \t':
                if not lastheader:
                    raise Errors.HeaderParseError('First line must not be a continuation')
                lastvalue.append(line)
                continue

            if lastheader:
                # XXX reconsider the joining of folded lines
                self._cur[lastheader] = EMPTYSTRING.join(lastvalue).rstrip()
                lastheader, lastvalue = '', []

            # Check for Unix-From
            if line.startswith('From '):
                if lineno == 0:
                    self._cur.set_unixfrom(line)
                    continue
                elif lineno == len(headerlist) - 1:
                    # Something looking like a unix-from at the end - it's
                    # probably the first line of the body
                    self._input.unreadline(line)
                    return
                else:
                    # Weirdly placed unix-from line. Ignore it.
                    continue

            i = line.find(':')
            if i < 0:
                # The older parser had various special-cases here. We've
                # already handled them
                raise Errors.HeaderParseError(
                       "Not a header, not a continuation: ``%s''" % line)
            lastheader = line[:i]
            lastvalue = [line[i+1:].lstrip()]

        if lastheader:
            # XXX reconsider the joining of folded lines
            self._cur[lastheader] = EMPTYSTRING.join(lastvalue).rstrip()


    def _parsegen(self):
        # Parse any currently available text
        self._new_sub_object()
        self._root = self._cur
        completing = False
        last = None
        
        for line in self._input:
            if line is NeedMoreData:
                yield None # Need More Data
                continue
            self._input.unreadline(line)
            if not completing:
                headers = []
                # Now collect all headers.
                for line in self._input:
                    if line is NeedMoreData:
                        yield None # Need More Data
                        continue
                    if not self.headerRE.match(line):
                        self._parse_headers(headers)
                        # A message/rfc822 has no body and no internal 
                        # boundary.
                        if self._cur.get_content_maintype() == "message":
                            self._new_sub_object()
                            completing = False
                            headers = []
                            continue
                        if line.strip():
                            # No blank line between headers and body. 
                            # Push this line back, it's the first line of 
                            # the body.
                            self._input.unreadline(line)
                        break
                    else:
                        headers.append(line)
                else:
                    # We're done with the data and are still inside the headers
                    self._parse_headers(headers)

            # Now we're dealing with the body
            boundary = self._cur.get_boundary()
            isdigest = (self._cur.get_content_type() == 'multipart/digest')
            if boundary and not self._cur._finishing:
                separator = '--' + boundary
                self._cur._boundaryRE = re.compile(
                        r'(?P<sep>' + re.escape(separator) +
                        r')(?P<end>--)?(?P<ws>[ \t]*)(?P<linesep>\r\n|\r|\n)$')
                for r in self._input.readuntil(self._cur._boundaryRE):
                    if r is NeedMoreData:
                         yield NeedMoreData
                    else:
                        preamble, matchobj = r
                        break
                if not matchobj:
                    # Broken - we hit the end of file. Just set the body 
                    # to the text.
                    if completing:
                        self._attach_trailer(last, preamble)
                    else:
                        self._attach_preamble(self._cur, preamble)
                    # XXX move back to the parent container.
                    self._pop_container()
                    completing = True
                    continue
                if preamble:
                    if completing:
                        preamble = preamble[:-len(matchobj.group('linesep'))]
                        self._attach_trailer(last, preamble)
                    else:
                        self._attach_preamble(self._cur, preamble)
                elif not completing:
                    # The module docs specify an empty preamble is None, not ''
                    self._cur.preamble = None
                    # If we _are_ completing, the last object gets no payload

                if matchobj.group('end'):
                    # That was the end boundary tag. Bounce back to the
                    # parent container
                    last = self._pop_container()
                    self._input.unreadline(matchobj.group('linesep'))
                    completing = True
                    continue

                # A number of MTAs produced by a nameless large company
                # we shall call "SicroMoft" produce repeated boundary 
                # lines.
                while True:
                    line = self._input.peekline()
                    if line is NeedMoreData:
                        yield None
                        continue
                    if self._cur._boundaryRE.match(line):
                        self._input.readline()
                    else:
                        break

                self._new_sub_object()
                
                completing = False
                if isdigest:
                    self._cur.set_default_type('message/rfc822')
                    continue
            else:
                # non-multipart or after end-boundary
                if last is not self._root:
                    last = self._pop_container()
                if self._cur.get_content_maintype() == "message":
                    # We double-pop to leave the RFC822 object
                    self._pop_container()
                    completing = True
                elif self._cur._boundaryRE and last <> self._root:
                    completing = True
                else:
                    # Non-multipart top level, or in the trailer of the 
                    # top level multipart
                    while not self._input.is_done():
                        yield None
                    data = list(self._input)
                    body = EMPTYSTRING.join(data)
                    self._attach_trailer(last, body)


    def _attach_trailer(self, obj, trailer):
        #import pdb ; pdb.set_trace()
        if obj.get_content_maintype() in ( "multipart", "message" ):
            obj.epilogue = trailer
        else:
            obj.set_payload(trailer)

    def _attach_preamble(self, obj, trailer):
        if obj.get_content_maintype() in ( "multipart", "message" ):
            obj.preamble = trailer
        else:
            obj.set_payload(trailer)


    def _new_sub_object(self):
        new = self._class()
        #print "pushing", self._objectstack, repr(new)
        if self._objectstack:
            self._objectstack[-1].attach(new)
        self._objectstack.append(new)
        new._boundaryRE = None
        new._finishing = False
        self._cur = new

    def _pop_container(self):
        # Move the pointer to the container of the current object.
        # Returns the (old) current object
        #import pdb ; pdb.set_trace()
        #print "popping", self._objectstack
        last = self._objectstack.pop()
        if self._objectstack:
            self._cur = self._objectstack[-1]
        else:
            self._cur._finishing = True
        return last


