# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer for execfile.

This converts usages of the execfile function into calls to the built-in
exec() function.
"""

from .. import pytree
from . import basefix
from .util import Comma, Name, Call, LParen, RParen, Dot


class FixExecfile(basefix.BaseFix):

    PATTERN = """
    power< 'execfile' trailer< '(' arglist< filename=any [',' globals=any [',' locals=any ] ] > ')' > >
    |
    power< 'execfile' trailer< '(' filename=any ')' > >
    """

    def transform(self, node, results):
        assert results
        syms = self.syms
        filename = results["filename"]
        globals = results.get("globals")
        locals = results.get("locals")
        args = [Name('open'), LParen(), filename.clone(), RParen(), Dot(),
                Name('read'), LParen(), RParen()]
        args[0].set_prefix("")
        if globals is not None:
            args.extend([Comma(), globals.clone()])
        if locals is not None:
            args.extend([Comma(), locals.clone()])

        return Call(Name("exec"), args, prefix=node.get_prefix())
