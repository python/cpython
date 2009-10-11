#! /usr/bin/env python
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
# the first invocation of the program, and from time to time thereafter.
#   When new groups are first created they may appear on your server as
# empty groups. By default, empty groups are ignored by the -a option.
# However, these new groups will not be created again, and so will not
# appear in the server's list of 'new groups' at a later date. Hence it
# won't appear until you do a '-a' after some articles have appeared.
#
# I should really keep a list of ignored empty groups and re-check them
# for articles on every run, but I haven't got around to it yet.
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
import sys, nntplib, marshal, time, os

#######################################################################
# Check these variables before running!                               #

# Top directory.
# Filenames which don't start with / are taken as being relative to this.
topdir = os.path.expanduser('~/newspage')

# The name of your NNTP host
# eg.
#    newshost = 'nntp-serv.cl.cam.ac.uk'
# or use following to get the name from the NNTPSERVER environment
# variable:
#    newshost = os.environ['NNTPSERVER']
newshost = 'news.example.com'

# The filename for a local cache of the newsgroup list
treefile = 'grouptree'

# The filename for descriptions of newsgroups
# I found a suitable one at ftp.uu.net in /uunet-info/newgroups.gz
# You can set this to '' if you don't wish to use one.
descfile = 'newsgroups'

# The directory in which HTML pages should be created
# eg.
#   pagedir  = '/usr/local/lib/html/newspage'
#   pagedir  = 'pages'
pagedir  = topdir

# The html prefix which will refer to this directory
# eg.
#   httppref = '/newspage/',
# or leave blank for relative links between pages: (Recommended)
#   httppref = ''
httppref = ''

# The name of the 'root' news page in this directory.
# A .html suffix will be added.
rootpage = 'root'

# Set skipempty to 0 if you wish to see links to empty groups as well.
# Only affects the -a option.
skipempty = 1

# pagelinkicon can contain html to put an icon after links to
# further pages. This helps to make important links stand out.
# Set to '' if not wanted, or '...' is quite a good one.
pagelinkicon = '... <img src="http://pelican.cl.cam.ac.uk/icons/page.xbm"> '

# ---------------------------------------------------------------------
# Less important personal preferences:

# Sublistsize controls the maximum number of items the will appear as
# an indented sub-list before the whole thing is moved onto a different
# page. The smaller this is, the more pages you will have, but the
# shorter each will be.
sublistsize = 4

# That should be all.                                                 #
#######################################################################

for dir in os.curdir, os.environ['HOME']:
    rcfile = os.path.join(dir, '.newslistrc.py')
    if os.path.exists(rcfile):
        print rcfile
        execfile(rcfile)
        break

from nntplib import NNTP
from stat import *

rcsrev = '$Revision$'
rcsrev = ' '.join(filter(lambda s: '$' not in s, rcsrev.split()))
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
        parts = i.split('.')
        makeleaf(tree, parts)

# Makeleaf makes a leaf and the branch leading to it if necessary
def makeleaf(tree,path):
    j = path[0]
    l = len(path)

    if j not in tree:
        tree[j] = {}
    if l == 1:
        tree[j]['.'] = '.'
    if l > 1:
        makeleaf(tree[j],path[1:])

# Then the bits for outputting trees as pages ----------------

# Createpage creates an HTML file named <root>.html containing links
# to those groups beginning with <root>.

def createpage(root, tree, p):
    filename = os.path.join(pagedir, root+'.html')
    if root == rootpage:
        detail = ''
    else:
        detail = ' under ' + root
    with open(filename, 'w') as f:
        # f.write('Content-Type: text/html\n')
        f.write('<html>\n<head>\n')
        f.write('<title>Newsgroups available%s</title>\n' % detail)
        f.write('</head>\n<body>\n')
        f.write('<h1>Newsgroups available%s</h1>\n' % detail)
        f.write('<a href="%s%s.html">Back to top level</a><p>\n' %
                (httppref, rootpage))
        printtree(f, tree, 0, p)
        f.write('\n<p>')
        f.write("<i>This page automatically created by 'newslist' v. %s." %
                rcsrev)
        f.write(time.ctime(time.time()) + '</i>\n')
        f.write('</body>\n</html>\n')

# Printtree prints the groups as a bulleted list.  Groups with
# more than <sublistsize> subgroups will be put on a separate page.
# Other sets of subgroups are just indented.

def printtree(f, tree, indent, p):
    l = len(tree)

    if l > sublistsize and indent > 0:
        # Create a new page and a link to it
        f.write('<li><b><a href="%s%s.html">' % (httppref, p[1:]))
        f.write(p[1:] + '.*')
        f.write('</a></b>%s\n' % pagelinkicon)
        createpage(p[1:], tree, p)
        return

    kl = tree.keys()

    if l > 1:
        kl.sort()
        if indent > 0:
            # Create a sub-list
            f.write('<li>%s\n<ul>' % p[1:])
        else:
            # Create a main list
            f.write('<ul>')
        indent = indent + 1

    for i in kl:
        if i == '.':
            # Output a newsgroup
            f.write('<li><a href="news:%s">%s</a> ' % (p[1:], p[1:]))
            if p[1:] in desc:
                f.write('     <i>%s</i>\n' % desc[p[1:]])
            else:
                f.write('\n')
        else:
            # Output a hierarchy
            printtree(f, tree[i], indent, p+'.'+i)

    if l > 1:
        f.write('\n</ul>')

# Reading descriptions file ---------------------------------------

# This returns a dict mapping group name to its description

def readdesc(descfile):
    global desc
    desc = {}

    if descfile == '':
        return

    try:
        with open(descfile, 'r') as d:
            print 'Reading descriptions...'
            for l in d:
                bits = l.split()
                try:
                    grp = bits[0]
                    dsc = ' '.join(bits[1:])
                    if len(dsc) > 1:
                        desc[grp] = dsc
                except IndexError:
                    pass
    except IOError:
        print 'Failed to open description file ' + descfile
        return

# Check that ouput directory exists, ------------------------------
# and offer to create it if not

def checkopdir(pagedir):
    if not os.path.isdir(pagedir):
        print 'Directory %s does not exist.' % pagedir
        print 'Shall I create it for you? (y/n)'
        if sys.stdin.readline()[0] == 'y':
            try:
                os.mkdir(pagedir, 0777)
            except:
                print 'Sorry - failed!'
                sys.exit(1)
        else:
            print 'OK. Exiting.'
            sys.exit(1)

# Read and write current local tree ----------------------------------

def readlocallist(treefile):
    print 'Reading current local group list...'
    tree = {}
    try:
        treetime = time.localtime(os.stat(treefile)[ST_MTIME])
    except:
        print '\n*** Failed to open local group cache '+treefile
        print 'If this is the first time you have run newslist, then'
        print 'use the -a option to create it.'
        sys.exit(1)
    treedate = '%02d%02d%02d' % (treetime[0] % 100, treetime[1], treetime[2])
    try:
        with open(treefile, 'rb') as dump:
            tree = marshal.load(dump)
    except IOError:
        print 'Cannot open local group list ' + treefile
    return (tree, treedate)

def writelocallist(treefile, tree):
    try:
        with open(treefile, 'wb') as dump:
            groups = marshal.dump(tree, dump)
        print 'Saved list to %s\n' % treefile
    except:
        print 'Sorry - failed to write to local group cache', treefile
        print 'Does it (or its directory) have the correct permissions?'
        sys.exit(1)

# Return list of all groups on server -----------------------------

def getallgroups(server):
    print 'Getting list of all groups...'
    treedate = '010101'
    info = server.list()[1]
    groups = []
    print 'Processing...'
    if skipempty:
        print '\nIgnoring following empty groups:'
    for i in info:
        grpname = i[0].split()[0]
        if skipempty and int(i[1]) < int(i[2]):
            print grpname + ' ',
        else:
            groups.append(grpname)
    print '\n'
    if skipempty:
        print '(End of empty groups)'
    return groups

# Return list of new groups on server -----------------------------

def getnewgroups(server, treedate):
    print 'Getting list of new groups since start of %s...' % treedate,
    info = server.newgroups(treedate, '000001')[1]
    print 'got %d.' % len(info)
    print 'Processing...',
    groups = []
    for i in info:
        grpname = i.split()[0]
        groups.append(grpname)
    print 'Done'
    return groups

# Now the main program --------------------------------------------

def main():
    tree = {}

    # Check that the output directory exists
    checkopdir(pagedir)

    try:
        print 'Connecting to %s...' % newshost
        if sys.version[0] == '0':
            s = NNTP.init(newshost)
        else:
            s = NNTP(newshost)
        connected = True
    except (nntplib.error_temp, nntplib.error_perm), x:
        print 'Error connecting to host:', x
        print 'I\'ll try to use just the local list.'
        connected = False

    # If -a is specified, read the full list of groups from server
    if connected and len(sys.argv) > 1 and sys.argv[1] == '-a':
        groups = getallgroups(s)

    # Otherwise just read the local file and then add
    # groups created since local file last modified.
    else:

        (tree, treedate) = readlocallist(treefile)
        if connected:
            groups = getnewgroups(s, treedate)

    if connected:
        addtotree(tree, groups)
        writelocallist(treefile,tree)

    # Read group descriptions
    readdesc(descfile)

    print 'Creating pages...'
    createpage(rootpage, tree, '')
    print 'Done'

if __name__ == "__main__":
    main()

# That's all folks
######################################################################
