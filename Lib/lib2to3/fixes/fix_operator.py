"""Fixer for operator.{isCallable,sequenceIncludes}

operator.isCallable(obj) -> hasattr(obj, '__call__')
operator.sequenceIncludes(obj) -> operator.contains(obj)
"""

# Local imports
from .. import fixer_base
from ..fixer_util import Call, Name, String

class FixOperator(fixer_base.BaseFix):

    methods = "method=('isCallable'|'sequenceIncludes')"
    func = "'(' func=any ')'"
    PATTERN = """
              power< module='operator'
                trailer< '.' %(methods)s > trailer< %(func)s > >
              |
              power< %(methods)s trailer< %(func)s > >
              """ % dict(methods=methods, func=func)

    def transform(self, node, results):
        method = results["method"][0]

        if method.value == u"sequenceIncludes":
            if "module" not in results:
                # operator may not be in scope, so we can't make a change.
                self.warning(node, "You should use operator.contains here.")
            else:
                method.value = u"contains"
                method.changed()
        elif method.value == u"isCallable":
            if "module" not in results:
                self.warning(node,
                             "You should use hasattr(%s, '__call__') here." %
                             results["func"].value)
            else:
                func = results["func"]
                args = [func.clone(), String(u", "), String(u"'__call__'")]
                return Call(Name(u"hasattr"), args, prefix=node.prefix)
