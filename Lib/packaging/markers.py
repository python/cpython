"""Parser for the environment markers micro-language defined in PEP 345."""

import sys
import platform
import os

from tokenize import tokenize, NAME, OP, STRING, ENDMARKER, ENCODING
from io import BytesIO

__all__ = ['interpret']


# allowed operators
_OPERATORS = {'==': lambda x, y: x == y,
              '!=': lambda x, y: x != y,
              '>': lambda x, y: x > y,
              '>=': lambda x, y: x >= y,
              '<': lambda x, y: x < y,
              '<=': lambda x, y: x <= y,
              'in': lambda x, y: x in y,
              'not in': lambda x, y: x not in y}


def _operate(operation, x, y):
    return _OPERATORS[operation](x, y)


# restricted set of variables
_VARS = {'sys.platform': sys.platform,
         'python_version': sys.version[:3],
         'python_full_version': sys.version.split(' ', 1)[0],
         'os.name': os.name,
         'platform.version': platform.version(),
         'platform.machine': platform.machine(),
         'platform.python_implementation': platform.python_implementation()}


class _Operation:

    def __init__(self, execution_context=None):
        self.left = None
        self.op = None
        self.right = None
        if execution_context is None:
            execution_context = {}
        self.execution_context = execution_context

    def _get_var(self, name):
        if name in self.execution_context:
            return self.execution_context[name]
        return _VARS[name]

    def __repr__(self):
        return '%s %s %s' % (self.left, self.op, self.right)

    def _is_string(self, value):
        if value is None or len(value) < 2:
            return False
        for delimiter in '"\'':
            if value[0] == value[-1] == delimiter:
                return True
        return False

    def _is_name(self, value):
        return value in _VARS

    def _convert(self, value):
        if value in _VARS:
            return self._get_var(value)
        return value.strip('"\'')

    def _check_name(self, value):
        if value not in _VARS:
            raise NameError(value)

    def _nonsense_op(self):
        msg = 'This operation is not supported : "%s"' % self
        raise SyntaxError(msg)

    def __call__(self):
        # make sure we do something useful
        if self._is_string(self.left):
            if self._is_string(self.right):
                self._nonsense_op()
            self._check_name(self.right)
        else:
            if not self._is_string(self.right):
                self._nonsense_op()
            self._check_name(self.left)

        if self.op not in _OPERATORS:
            raise TypeError('Operator not supported "%s"' % self.op)

        left = self._convert(self.left)
        right = self._convert(self.right)
        return _operate(self.op, left, right)


class _OR:
    def __init__(self, left, right=None):
        self.left = left
        self.right = right

    def filled(self):
        return self.right is not None

    def __repr__(self):
        return 'OR(%r, %r)' % (self.left, self.right)

    def __call__(self):
        return self.left() or self.right()


class _AND:
    def __init__(self, left, right=None):
        self.left = left
        self.right = right

    def filled(self):
        return self.right is not None

    def __repr__(self):
        return 'AND(%r, %r)' % (self.left, self.right)

    def __call__(self):
        return self.left() and self.right()


def interpret(marker, execution_context=None):
    """Interpret a marker and return a result depending on environment."""
    marker = marker.strip().encode()
    ops = []
    op_starting = True
    for token in tokenize(BytesIO(marker).readline):
        # Unpack token
        toktype, tokval, rowcol, line, logical_line = token
        if toktype not in (NAME, OP, STRING, ENDMARKER, ENCODING):
            raise SyntaxError('Type not supported "%s"' % tokval)

        if op_starting:
            op = _Operation(execution_context)
            if len(ops) > 0:
                last = ops[-1]
                if isinstance(last, (_OR, _AND)) and not last.filled():
                    last.right = op
                else:
                    ops.append(op)
            else:
                ops.append(op)
            op_starting = False
        else:
            op = ops[-1]

        if (toktype == ENDMARKER or
            (toktype == NAME and tokval in ('and', 'or'))):
            if toktype == NAME and tokval == 'and':
                ops.append(_AND(ops.pop()))
            elif toktype == NAME and tokval == 'or':
                ops.append(_OR(ops.pop()))
            op_starting = True
            continue

        if isinstance(op, (_OR, _AND)) and op.right is not None:
            op = op.right

        if ((toktype in (NAME, STRING) and tokval not in ('in', 'not'))
            or (toktype == OP and tokval == '.')):
            if op.op is None:
                if op.left is None:
                    op.left = tokval
                else:
                    op.left += tokval
            else:
                if op.right is None:
                    op.right = tokval
                else:
                    op.right += tokval
        elif toktype == OP or tokval in ('in', 'not'):
            if tokval == 'in' and op.op == 'not':
                op.op = 'not in'
            else:
                op.op = tokval

    for op in ops:
        if not op():
            return False
    return True
