# Copyright 2006 Georg Brandl.
# Licensed to PSF under a Contributor Agreement.

"""Fixer for intern().

intern(s) -> sys.intern(s)"""

# Local imports
from .. import pytree
from .. import fixer_base
from ..fixer_util import Name, Attr, touch_import


class FixIntern(fixer_base.BaseFix):
    BM_compatible = True
    order = "pre"

    PATTERN = """
    power< 'intern'
           trailer< lpar='('
                    ( not(arglist | argument<any '=' any>) obj=any
                      | obj=arglist<(not argument<any '=' any>) any ','> )
                    rpar=')' >
           after=any*
    >
    """

    def transform(self, node, results):
        if results:
            # I feel like we should be able to express this logic in the
            # PATTERN above but I don't know how to do it so...
            obj = results['obj']
            if obj:
                if obj.type == self.syms.star_expr:
                    return  # Make no change.
                if (obj.type == self.syms.argument and
                    obj.children[0].value == '**'):
                    return  # Make no change.
        syms = self.syms
        obj = results["obj"].clone()
        if obj.type == syms.arglist:
            newarglist = obj.clone()
        else:
            newarglist = pytree.Node(syms.arglist, [obj.clone()])
        after = results["after"]
        if after:
            after = [n.clone() for n in after]
        new = pytree.Node(syms.power,
                          Attr(Name(u"sys"), Name(u"intern")) +
                          [pytree.Node(syms.trailer,
                                       [results["lpar"].clone(),
                                        newarglist,
                                        results["rpar"].clone()])] + after)
        new.prefix = node.prefix
        touch_import(None, u'sys', node)
        return new
