"""Fixer for operator functions.

operator.isCallable(obj)       -> hasattr(obj, '__call__')
operator.sequenceIncludes(obj) -> operator.contains(obj)
operator.isSequenceType(obj)   -> isinstance(obj, collections.Sequence)
operator.isMappingType(obj)    -> isinstance(obj, collections.Mapping)
operator.isNumberType(obj)     -> isinstance(obj, numbers.Number)
operator.repeat(obj, n)        -> operator.mul(obj, n)
operator.irepeat(obj, n)       -> operator.imul(obj, n)
"""

import collections

# Local imports
from lib2to3 import fixer_base
from lib2to3.fixer_util import Call, Name, String, touch_import


class FixOperator(fixer_base.BaseFix):

    methods = """
              method=('isCallable'|'sequenceIncludes'
                     |'isSequenceType'|'isMappingType'|'isNumberType'
                     |'repeat'|'irepeat')
              """
    obj = "'(' obj=any ')'"
    PATTERN = """
              power< module='operator'
                trailer< '.' %(methods)s > trailer< %(obj)s > >
              |
              power< %(methods)s trailer< %(obj)s > >
              """ % dict(methods=methods, obj=obj)

    def transform(self, node, results):
        method = self._check_method(node, results)
        if method is not None:
            return method(node, results)

    def _sequenceIncludes(self, node, results):
        """operator.contains(%s)"""
        return self._handle_rename(node, results, "contains")

    def _isCallable(self, node, results):
        """hasattr(%s, '__call__')"""
        obj = results["obj"]
        args = [obj.clone(), String(", "), String("'__call__'")]
        return Call(Name("hasattr"), args, prefix=node.prefix)

    def _repeat(self, node, results):
        """operator.mul(%s)"""
        return self._handle_rename(node, results, "mul")

    def _irepeat(self, node, results):
        """operator.imul(%s)"""
        return self._handle_rename(node, results, "imul")

    def _isSequenceType(self, node, results):
        """isinstance(%s, collections.Sequence)"""
        return self._handle_type2abc(node, results, "collections", "Sequence")

    def _isMappingType(self, node, results):
        """isinstance(%s, collections.Mapping)"""
        return self._handle_type2abc(node, results, "collections", "Mapping")

    def _isNumberType(self, node, results):
        """isinstance(%s, numbers.Number)"""
        return self._handle_type2abc(node, results, "numbers", "Number")

    def _handle_rename(self, node, results, name):
        method = results["method"][0]
        method.value = name
        method.changed()

    def _handle_type2abc(self, node, results, module, abc):
        touch_import(None, module, node)
        obj = results["obj"]
        args = [obj.clone(), String(", " + ".".join([module, abc]))]
        return Call(Name("isinstance"), args, prefix=node.prefix)

    def _check_method(self, node, results):
        method = getattr(self, "_" + results["method"][0].value)
        if isinstance(method, collections.Callable):
            if "module" in results:
                return method
            else:
                sub = (str(results["obj"]),)
                invocation_str = str(method.__doc__) % sub
                self.warning(node, "You should use '%s' here." % invocation_str)
        return None
