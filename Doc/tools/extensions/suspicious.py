"""
Try to detect suspicious constructs, resembling markup
that has leaked into the final output.

Suspicious lines are reported in a comma-separated-file,
``suspicious.csv``, located in the output directory.

The file is utf-8 encoded, and each line contains four fields:

 * document name (normalized)
 * line number in the source document
 * problematic text
 * complete line showing the problematic text in context

It is common to find many false positives. To avoid reporting them
again and again, they may be added to the ``ignored.csv`` file
(located in the configuration directory). The file has the same
format as ``suspicious.csv`` with a few differences:

  - each line defines a rule; if the rule matches, the issue
    is ignored.
  - line number may be empty (that is, nothing between the
    commas: ",,"). In this case, line numbers are ignored (the
    rule matches anywhere in the file).
  - the last field does not have to be a complete line; some
    surrounding text (never more than a line) is enough for
    context.

Rules are processed sequentially. A rule matches when:

 * document names are the same
 * problematic texts are the same
 * line numbers are close to each other (5 lines up or down)
 * the rule text is completely contained into the source line

The simplest way to create the ignored.csv file is by copying
undesired entries from suspicious.csv (possibly trimming the last
field.)

Copyright 2009 Gabriel A. Genellina

"""

import os
import re
import csv
import sys

from docutils import nodes
from sphinx.builders import Builder
import sphinx.util

detect_all = re.compile(r'''
    ::(?=[^=])|            # two :: (but NOT ::=)
    :[a-zA-Z][a-zA-Z0-9]+| # :foo
    `|                     # ` (seldom used by itself)
    (?<!\.)\.\.[ \t]*\w+:  # .. foo: (but NOT ... else:)
    ''', re.UNICODE | re.VERBOSE).finditer

py3 = sys.version_info >= (3, 0)


class Rule:
    def __init__(self, docname, lineno, issue, line):
        """A rule for ignoring issues"""
        self.docname = docname # document to which this rule applies
        self.lineno = lineno   # line number in the original source;
                               # this rule matches only near that.
                               # None -> don't care
        self.issue = issue     # the markup fragment that triggered this rule
        self.line = line       # text of the container element (single line only)
        self.used = False

    def __repr__(self):
        return '{0.docname},,{0.issue},{0.line}'.format(self)



class dialect(csv.excel):
    """Our dialect: uses only linefeed as newline."""
    lineterminator = '\n'


class CheckSuspiciousMarkupBuilder(Builder):
    """
    Checks for possibly invalid markup that may leak into the output.
    """
    name = 'suspicious'
    logger = sphinx.util.logging.getLogger("CheckSuspiciousMarkupBuilder")

    def init(self):
        # create output file
        self.log_file_name = os.path.join(self.outdir, 'suspicious.csv')
        open(self.log_file_name, 'w').close()
        # load database of previously ignored issues
        self.load_rules(os.path.join(os.path.dirname(__file__), '..',
                                     'susp-ignored.csv'))

    def get_outdated_docs(self):
        return self.env.found_docs

    def get_target_uri(self, docname, typ=None):
        return ''

    def prepare_writing(self, docnames):
        pass

    def write_doc(self, docname, doctree):
        # set when any issue is encountered in this document
        self.any_issue = False
        self.docname = docname
        visitor = SuspiciousVisitor(doctree, self)
        doctree.walk(visitor)

    def finish(self):
        unused_rules = [rule for rule in self.rules if not rule.used]
        if unused_rules:
            self.logger.warning(
                'Found %s/%s unused rules: %s' % (
                    len(unused_rules), len(self.rules),
                    ''.join(repr(rule) for rule in unused_rules),
                )
            )
        return

    def check_issue(self, line, lineno, issue):
        if not self.is_ignored(line, lineno, issue):
            self.report_issue(line, lineno, issue)

    def is_ignored(self, line, lineno, issue):
        """Determine whether this issue should be ignored."""
        docname = self.docname
        for rule in self.rules:
            if rule.docname != docname: continue
            if rule.issue != issue: continue
            # Both lines must match *exactly*. This is rather strict,
            # and probably should be improved.
            # Doing fuzzy matches with levenshtein distance could work,
            # but that means bringing other libraries...
            # Ok, relax that requirement: just check if the rule fragment
            # is contained in the document line
            if rule.line not in line: continue
            # Check both line numbers. If they're "near"
            # this rule matches. (lineno=None means "don't care")
            if (rule.lineno is not None) and \
                abs(rule.lineno - lineno) > 5: continue
            # if it came this far, the rule matched
            rule.used = True
            return True
        return False

    def report_issue(self, text, lineno, issue):
        self.any_issue = True
        self.write_log_entry(lineno, issue, text)
        if py3:
            self.logger.warning('[%s:%d] "%s" found in "%-.120s"' %
                                (self.docname, lineno, issue, text))
        else:
            self.logger.warning(
                '[%s:%d] "%s" found in "%-.120s"' % (
                    self.docname.encode(sys.getdefaultencoding(),'replace'),
                    lineno,
                    issue.encode(sys.getdefaultencoding(),'replace'),
                    text.strip().encode(sys.getdefaultencoding(),'replace')))
        self.app.statuscode = 1

    def write_log_entry(self, lineno, issue, text):
        if py3:
            f = open(self.log_file_name, 'a')
            writer = csv.writer(f, dialect)
            writer.writerow([self.docname, lineno, issue, text.strip()])
            f.close()
        else:
            f = open(self.log_file_name, 'ab')
            writer = csv.writer(f, dialect)
            writer.writerow([self.docname.encode('utf-8'),
                             lineno,
                             issue.encode('utf-8'),
                             text.strip().encode('utf-8')])
            f.close()

    def load_rules(self, filename):
        """Load database of previously ignored issues.

        A csv file, with exactly the same format as suspicious.csv
        Fields: document name (normalized), line number, issue, surrounding text
        """
        self.logger.info("loading ignore rules... ", nonl=1)
        self.rules = rules = []
        try:
            if py3:
                f = open(filename, 'r')
            else:
                f = open(filename, 'rb')
        except IOError:
            return
        for i, row in enumerate(csv.reader(f)):
            if len(row) != 4:
                raise ValueError(
                    "wrong format in %s, line %d: %s" % (filename, i+1, row))
            docname, lineno, issue, text = row
            if lineno:
                lineno = int(lineno)
            else:
                lineno = None
            if not py3:
                docname = docname.decode('utf-8')
                issue = issue.decode('utf-8')
                text = text.decode('utf-8')
            rule = Rule(docname, lineno, issue, text)
            rules.append(rule)
        f.close()
        self.logger.info('done, %d rules loaded' % len(self.rules))


def get_lineno(node):
    """Obtain line number information for a node."""
    lineno = None
    while lineno is None and node:
        node = node.parent
        lineno = node.line
    return lineno


def extract_line(text, index):
    """text may be a multiline string; extract
    only the line containing the given character index.

    >>> extract_line("abc\ndefgh\ni", 6)
    >>> 'defgh'
    >>> for i in (0, 2, 3, 4, 10):
    ...   print extract_line("abc\ndefgh\ni", i)
    abc
    abc
    abc
    defgh
    defgh
    i
    """
    p = text.rfind('\n', 0, index) + 1
    q = text.find('\n', index)
    if q < 0:
        q = len(text)
    return text[p:q]


class SuspiciousVisitor(nodes.GenericNodeVisitor):

    lastlineno = 0

    def __init__(self, document, builder):
        nodes.GenericNodeVisitor.__init__(self, document)
        self.builder = builder

    def default_visit(self, node):
        if isinstance(node, (nodes.Text, nodes.image)): # direct text containers
            text = node.astext()
            # lineno seems to go backwards sometimes (?)
            self.lastlineno = lineno = max(get_lineno(node) or 0, self.lastlineno)
            seen = set() # don't report the same issue more than only once per line
            for match in detect_all(text):
                issue = match.group()
                line = extract_line(text, match.start())
                if (issue, line) not in seen:
                    self.builder.check_issue(line, lineno, issue)
                    seen.add((issue, line))

    unknown_visit = default_visit

    def visit_document(self, node):
        self.lastlineno = 0

    def visit_comment(self, node):
        # ignore comments -- too much false positives.
        # (although doing this could miss some errors;
        # there were two sections "commented-out" by mistake
        # in the Python docs that would not be caught)
        raise nodes.SkipNode
