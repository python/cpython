#! /usr/local/bin/python
#######################################################################
# Newslist  $Revision$
#
# Syntax:
#    newslist [ -a ]
#
# This is a program to create a directory full of HTML pages 
# which between them contain links to all the newsgroups available
# on your server.
#
# The -a option causes a complete list of all groups to be read from 
# the server rather than just the ones which have appeared since last
# execution. This recreates the local list from scratch. Use this on
# the first invocation of the program.
#
# This assumes an NNTP news feed.
#
# Feel free to copy, distribute and modify this code for 
# non-commercial use. If you make any useful modifications, let me 
# know!
#
# (c) Quentin Stafford-Fraser 1994
# fraser@europarc.xerox.com                     qs101@cl.cam.ac.uk
#                                                                     #
#######################################################################
import sys,nntplib, string, marshal, time, os, posix, string

#######################################################################
# Check these variables before running!                               #

# Top directory.
# Filenames which don't start with / are taken as being relative to this.
##topdir='/anfs/qsbigdisc/web/html/newspage'
topdir = '/hosts/buizerd/ufs/www/cwi/cwionly/newstree'

# The name of your NNTP host
# eg. 
#    newshost = 'nntp-serv.cam.ac.uk'
# or use following to get the name from the NNTPSERVER environment 
# variable:
##newshost = posix.environ['NNTPSERVER']
newshost = 'charon.cwi.nl'

# The filename for a local cache of the newsgroup list
treefile = 'grouptree'

# The filename for descriptions of newsgroups
# I found a suitable one at ftp.uu.net in /uunet-info/newgroups.gz
# You can set this to '' if you don't wish to use one.
##descfile = 'newsgroups'
descfile = '/usr/lib/news/newsgroups'

# The directory in which HTML pages should be created
# eg.
#   pagedir  = '/usr/local/lib/html/newspage'
#   pagedir  = 'pages' 
pagedir  = topdir

# The html prefix which will refer to this directory
# eg. 
#   httppref = '/newspage/', 
# or leave blank for relative links
# between pages. (Recommended)
httppref = ''

# The name of the 'root' news page in this directory. 
# A .html suffix will be added.
##rootpage = 'root'
rootpage = 'index'

# Set skipempty to 0 if you wish to see links to empty groups as well.
# Only affects the -a option.
##skipempty = 1
skipempty = 0

# ---------------------------------------------------------------------
# Less important personal preferences:

# Sublistsize controls the maximum number of items the will appear as
# an indented sub-list before the whole thing is moved onto a different
# page. The smaller this is, the more pages you will have, but the 
# shorter each will be.
sublistsize = 4

# That should be all.                                                 #
#######################################################################

from nntplib import NNTP
from stat import *

rcsrev = '$Revision$'[11:15]
desc = {}

# Make (possibly) relative filenames into absolute ones
treefile = os.path.join(topdir,treefile)
descfile = os.path.join(topdir,descfile)
page = os.path.join(topdir,pagedir)

# First the bits for creating trees ---------------------------

# Addtotree creates/augments a tree from a list of group names
def addtotree(tree, groups):
   print 'Updating tree...'
   for i in groups:
	parts = string.splitfields(i,'.')
	makeleaf(tree, parts)

# Makeleaf makes a leaf and the branch leading to it if necessary
def makeleaf(tree,path):
   j = path[0]
   l = len(path)

   if not tree.has_key(j):
      tree[j] = {}
   if l == 1:
      tree[j]['.'] = '.'
   if l > 1:
      makeleaf(tree[j],path[1:])

# Then the bits for outputting trees as pages ----------------	

# Createpage creates an HTML file named <root>.html containing links
# to those groups beginning with <root>.

def createpage(root, tree, p):
   filename = os.path.join(pagedir,root+'.html')
   if root == rootpage:
      detail = ''
   else:
      detail = ' under ' + root
   f = open(filename,'w')
   # f.write('Content-Type: text/html\n')
   f.write('<TITLE>Newsgroups available' + detail + '</TITLE>\n')
   f.write('<H1>Newsgroups available' + detail +'</H1>\n')
   f.write('<A HREF="'+httppref+rootpage+'.html">Back to top level</A><P>\n')
   printtree(f,tree,0,p)
   f.write('<I>This page automatically created by \'newslist\' v. '+rcsrev+'.')
   f.write(time.ctime(time.time()) + '</I><P>')
   f.close()

# Printtree prints the groups as a bulleted list.  Groups with
# more than <sublistsize> subgroups will be put on a separate page.
# Other sets of subgroups are just indented.

def printtree(f, tree, indent, p):
   global desc
   l = len(tree)

   if l > sublistsize and indent>0:
      # Create a new page and a link to it
      f.write('<LI><B><A HREF="'+httppref+p[1:]+'.html">')
      f.write(p[1:]+'.* ...')
      f.write('</A></B>\n')
      createpage(p[1:], tree, p)
      return

   kl = tree.keys()

   if l > 1:
      kl.sort()
      if indent > 0:
	 # Create a sub-list
	 f.write('<LI><B>'+p[1:]+'</B>\n<UL>')
      else:
	 # Create a main list
	 f.write('<UL>')
      indent = indent + 1
   
   for i in kl:
      if i == '.':
	 # Output a newsgroup
	 f.write('<LI><A HREF="news:' + p[1:] + '">'+ p[1:] + '</A> ')
	 if desc.has_key(p[1:]):
	    f.write('     <I>'+desc[p[1:]]+'</I>\n')
	 else:
	    f.write('\n')
      else:
	 # Output a hierarchy
	 printtree(f,tree[i], indent, p+'.'+i)

   if l > 1:
      f.write('\n</UL>')

# Reading descriptions file ---------------------------------------

# This returns an array mapping group name to its description

def readdesc():
   global desc

   desc = {}

   if descfile == '':
	return

   try:
      d = open(descfile, 'r')
      print 'Reading descriptions...'
   except (IOError):
      print 'Failed to open description file ' + descfile
      return
   l = d.readline()
   while l != '':
      bits = string.split(l)
      try:
	 grp = bits[0]
	 dsc = string.join(bits[1:])
	 if len(dsc)>1:
	    desc[grp] = dsc
      except (IndexError):
	 pass
      l = d.readline()

# Now the main program --------------------------------------------

def main():
   global desc

   connected = 0
   tree={}

   # Check that the output directory exists
   if not os.path.isdir(pagedir):
      print 'Directory '+pagedir+' does not exist.'
      print 'Shall I create it for you? (y/n)'
      if sys.stdin.readline()[0] == 'y':
	 try:
	    os.mkdir(pagedir,0777)
	 except:
	    print 'Sorry - failed!'
	    sys.exit(1)
      else:
	 print 'OK. Exiting.'
	 sys.exit(1)

   try:
      print 'Connecting to '+newshost+'...'
      if sys.version[0] == '0':
	 s = NNTP.init(newshost)
      else:
	 s = NNTP(newshost)
      connected = 1
   except (nntplib.error_temp, nntplib.error_perm), x:
      print 'Error connecting to host:'
      print x
      print 'I\'ll try to use just the local list.'
   
   # If -a is specified, read the full list of groups from server   
   if connected and len(sys.argv) > 1 and sys.argv[1] == '-a':
      print 'Getting list of all groups...'
      treedate='010101'
      info = s.list()[1]
      groups = []
      print 'Processing...'
      if skipempty:
	 print '\nIgnoring following empty groups:'
      for i in info:
	 if skipempty and string.atoi(i[1]) < string.atoi(i[2]):
	    print i[0]+' ',
	 else:
	    groups.append(i[0])
      print '\n(End of empty groups)'

   # Otherwise just read groups created since local file last modified.
   else:
      print 'Reading current local group list...'
      try:
	 treetime = time.localtime(os.stat(treefile)[ST_MTIME])
      except:
	 print '\n*** Failed to open local group cache '+treefile
	 print 'If this is the first time you have run newslist, then'
	 print 'use the -a option to create it.'
	 sys.exit(1)
      treedate = '%02d%02d%02d' % (treetime[0] % 100 ,treetime[1], treetime[2])
      try:
	 dump = open(treefile,'r')
	 tree = marshal.load(dump)
	 dump.close()
      except (IOError):
	 print 'Cannot open local group list ' + treefile
	
      if connected:
	 print 'Getting list of new groups since start of '+treedate+'...',
	 groups = s.newgroups(treedate,'000001')[1]
	 print 'got '+`len(groups)`+'.'

   if connected:
      addtotree(tree, groups)
      try:
	 dump = open(treefile,'w')
	 groups = marshal.dump(tree,dump)
	 dump.close()
	 print 'Saved list to '+treefile+'\n'
      except:
	 print 'Sorry - failed to write to local group cache '+treefile
	 print 'Does it (or its directory) have the correct permissions?'
	 sys.exit(1)

   readdesc()
   print 'Creating pages...'
   createpage(rootpage, tree, '')
   print 'Done'


main()

# That's all folks
######################################################################
