"""

Robots.txt file parser class.  Accepts a list of lines or robots.txt URL as
input, builds a set of rules from that list, then answers questions about
fetchability of other URLs.

"""

class RobotFileParser:

    def __init__(self):
	self.rules = {}
	self.debug = 0
	self.url = ''
	self.last_checked = 0

    def mtime(self):
	return self.last_checked

    def modified(self):
	import time
	self.last_checked = time.time()

    def set_url(self, url):
	self.url = url
## 	import urlmisc
## 	self.url = urlmisc.canonical_url(url)

    def read(self):
	import urllib
	self.parse(urllib.urlopen(self.url).readlines())

    def parse(self, lines):
	import regsub, string, regex
	active = []
	for line in lines:
	    if self.debug: print '>', line,
	    # blank line terminates current record
	    if not line[:-1]:
		active = []
		continue
	    # remove optional comment and strip line
	    line = string.strip(line[:string.find(line, '#')])
	    if not line:
		continue
	    line = regsub.split(line, ' *: *')
	    if len(line) == 2:
		line[0] = string.lower(line[0])
		if line[0] == 'user-agent':
		    # this record applies to this user agent
		    if self.debug: print '>> user-agent:', line[1]
		    active.append(line[1])
		    if not self.rules.has_key(line[1]):
			self.rules[line[1]] = []
		elif line[0] == 'disallow':
		    if line[1]:
			if self.debug: print '>> disallow:', line[1]
			for agent in active:
			    self.rules[agent].append(regex.compile(line[1]))
		    else:
			pass
			for agent in active:
			    if self.debug: print '>> allow', agent
			    self.rules[agent] = []
		else:
		    if self.debug: print '>> unknown:', line

	self.modified()

    # returns true if agent is allowed to fetch url
    def can_fetch(self, agent, url):
	import urlparse
	ag = agent
	if not self.rules.has_key(ag): ag = '*'
	if not self.rules.has_key(ag):
	    if self.debug: print '>> allowing', url, 'fetch by', agent
	    return 1
	path = urlparse.urlparse(url)[2]
	for rule in self.rules[ag]:
	    if rule.match(path) != -1:
		if self.debug: print '>> disallowing', url, 'fetch by', agent
		return 0
	if self.debug: print '>> allowing', url, 'fetch by', agent
	return 1

def test():
    rp = RobotFileParser()
    rp.debug = 1
    rp.set_url('http://www.automatrix.com/robots.txt')
    rp.read()
    print rp.rules
    print rp.can_fetch('*', 'http://www.calendar.com/concerts/')
    print rp.can_fetch('Musi-Cal-Robot',
		       'http://dolphin:80/cgi-bin/music-search?performer=Rolling+Stones')

    print rp.can_fetch('Lycos', 'http://www/~skip/volkswagen/')
    print rp.can_fetch('Lycos', 'http://www/~skip/volkswagen/vanagon-list-001')
