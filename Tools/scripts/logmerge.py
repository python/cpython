#! /usr/bin/env python

"""Consolidate a bunch of CVS or RCS logs read from stdin.

Input should be the output of a CVS or RCS logging command, e.g.

    cvs log -rrelease14:

which dumps all log messages from release1.4 upwards (assuming that
release 1.4 was tagged with tag 'release14').  Note the trailing
colon!

This collects all the revision records and outputs them sorted by date
rather than by file, collapsing duplicate revision record, i.e.,
records with the same message for different files.

The -t option causes it to truncate (discard) the last revision log
entry; this is useful when using something like the above cvs log
command, which shows the revisions including the given tag, while you
probably want everything *since* that tag.

The -r option reverses the output (oldest first; the default is oldest
last).

The -b tag option restricts the output to *only* checkin messages
belonging to the given branch tag.  The form -b HEAD restricts the
output to checkin messages belonging to the CVS head (trunk).  (It
produces some output if tag is a non-branch tag, but this output is
not very useful.)

-h prints this message and exits.

XXX This code was created by reverse engineering CVS 1.9 and RCS 5.7
from their output.
"""

import os, sys, errno, getopt, re

sep1 = '='*77 + '\n'                    # file separator
sep2 = '-'*28 + '\n'                    # revision separator

def main():
    """Main program"""
    truncate_last = 0
    reverse = 0
    branch = None
    opts, args = getopt.getopt(sys.argv[1:], "trb:h")
    for o, a in opts:
        if o == '-t':
            truncate_last = 1
        elif o == '-r':
            reverse = 1
        elif o == '-b':
            branch = a
        elif o == '-h':
            print __doc__
            sys.exit(0)
    database = []
    while 1:
        chunk = read_chunk(sys.stdin)
        if not chunk:
            break
        records = digest_chunk(chunk, branch)
        if truncate_last:
            del records[-1]
        database[len(database):] = records
    database.sort()
    if not reverse:
        database.reverse()
    format_output(database)

def read_chunk(fp):
    """Read a chunk -- data for one file, ending with sep1.

    Split the chunk in parts separated by sep2.

    """
    chunk = []
    lines = []
    while 1:
        line = fp.readline()
        if not line:
            break
        if line == sep1:
            if lines:
                chunk.append(lines)
            break
        if line == sep2:
            if lines:
                chunk.append(lines)
                lines = []
        else:
            lines.append(line)
    return chunk

def digest_chunk(chunk, branch=None):
    """Digest a chunk -- extract working file name and revisions"""
    lines = chunk[0]
    key = 'Working file:'
    keylen = len(key)
    for line in lines:
        if line[:keylen] == key:
            working_file = line[keylen:].strip()
            break
    else:
        working_file = None
    if branch is None:
        pass
    elif branch == "HEAD":
        branch = re.compile(r"^\d+\.\d+$")
    else:
        revisions = {}
        key = 'symbolic names:\n'
        found = 0
        for line in lines:
            if line == key:
                found = 1
            elif found:
                if line[0] in '\t ':
                    tag, rev = line.split()
                    if tag[-1] == ':':
                        tag = tag[:-1]
                    revisions[tag] = rev
                else:
                    found = 0
        rev = revisions.get(branch)
        branch = re.compile(r"^<>$") # <> to force a mismatch by default
        if rev:
            if rev.find('.0.') >= 0:
                rev = rev.replace('.0.', '.')
                branch = re.compile(r"^" + re.escape(rev) + r"\.\d+$")
    records = []
    for lines in chunk[1:]:
        revline = lines[0]
        dateline = lines[1]
        text = lines[2:]
        words = dateline.split()
        author = None
        if len(words) >= 3 and words[0] == 'date:':
            dateword = words[1]
            timeword = words[2]
            if timeword[-1:] == ';':
                timeword = timeword[:-1]
            date = dateword + ' ' + timeword
            if len(words) >= 5 and words[3] == 'author:':
                author = words[4]
                if author[-1:] == ';':
                    author = author[:-1]
        else:
            date = None
            text.insert(0, revline)
        words = revline.split()
        if len(words) >= 2 and words[0] == 'revision':
            rev = words[1]
        else:
            # No 'revision' line -- weird...
            rev = None
            text.insert(0, revline)
        if branch:
            if rev is None or not branch.match(rev):
                continue
        records.append((date, working_file, rev, author, text))
    return records

def format_output(database):
    prevtext = None
    prev = []
    database.append((None, None, None, None, None)) # Sentinel
    for (date, working_file, rev, author, text) in database:
        if text != prevtext:
            if prev:
                print sep2,
                for (p_date, p_working_file, p_rev, p_author) in prev:
                    print p_date, p_author, p_working_file, p_rev
                sys.stdout.writelines(prevtext)
            prev = []
        prev.append((date, working_file, rev, author))
        prevtext = text

if __name__ == '__main__':
    try:
        main()
    except IOError, e:
        if e.errno != errno.EPIPE:
            raise
