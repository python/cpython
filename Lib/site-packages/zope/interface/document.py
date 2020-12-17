##############################################################################
#
# Copyright (c) 2001, 2002 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
""" Pretty-Print an Interface object as structured text (Yum)

This module provides a function, asStructuredText, for rendering an
interface as structured text.
"""
import zope.interface

__all__ = [
    'asReStructuredText',
    'asStructuredText',
]

def asStructuredText(I, munge=0, rst=False):
    """ Output structured text format.  Note, this will whack any existing
    'structured' format of the text.

    If `rst=True`, then the output will quote all code as inline literals in
    accordance with 'reStructuredText' markup principles.
    """

    if rst:
        inline_literal = lambda s: "``%s``" % (s,)
    else:
        inline_literal = lambda s: s

    r = [inline_literal(I.getName())]
    outp = r.append
    level = 1

    if I.getDoc():
        outp(_justify_and_indent(_trim_doc_string(I.getDoc()), level))

    bases = [base
             for base in I.__bases__
             if base is not zope.interface.Interface
             ]
    if bases:
        outp(_justify_and_indent("This interface extends:", level, munge))
        level += 1
        for b in bases:
            item = "o %s" % inline_literal(b.getName())
            outp(_justify_and_indent(_trim_doc_string(item), level, munge))
        level -= 1

    namesAndDescriptions = sorted(I.namesAndDescriptions())

    outp(_justify_and_indent("Attributes:", level, munge))
    level += 1
    for name, desc in namesAndDescriptions:
        if not hasattr(desc, 'getSignatureString'):   # ugh...
            item = "%s -- %s" % (inline_literal(desc.getName()),
                                 desc.getDoc() or 'no documentation')
            outp(_justify_and_indent(_trim_doc_string(item), level, munge))
    level -= 1

    outp(_justify_and_indent("Methods:", level, munge))
    level += 1
    for name, desc in namesAndDescriptions:
        if hasattr(desc, 'getSignatureString'):   # ugh...
            _call = "%s%s" % (desc.getName(), desc.getSignatureString())
            item = "%s -- %s" % (inline_literal(_call),
                                 desc.getDoc() or 'no documentation')
            outp(_justify_and_indent(_trim_doc_string(item), level, munge))

    return "\n\n".join(r) + "\n\n"


def asReStructuredText(I, munge=0):
    """ Output reStructuredText format.  Note, this will whack any existing
    'structured' format of the text."""
    return asStructuredText(I, munge=munge, rst=True)


def _trim_doc_string(text):
    """ Trims a doc string to make it format
    correctly with structured text. """

    lines = text.replace('\r\n', '\n').split('\n')
    nlines = [lines.pop(0)]
    if lines:
        min_indent = min([len(line) - len(line.lstrip())
                          for line in lines])
        for line in lines:
            nlines.append(line[min_indent:])

    return '\n'.join(nlines)


def _justify_and_indent(text, level, munge=0, width=72):
    """ indent and justify text, rejustify (munge) if specified """

    indent = " " * level

    if munge:
        lines = []
        line = indent
        text = text.split()

        for word in text:
            line = ' '.join([line, word])
            if len(line) > width:
                lines.append(line)
                line = indent
        else:
            lines.append(line)

        return '\n'.join(lines)

    else:
        return indent + \
            text.strip().replace("\r\n", "\n") .replace("\n", "\n" + indent)
