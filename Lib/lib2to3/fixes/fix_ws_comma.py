"""Fixer that changes 'a ,b' into 'a, b'.

This also changes '{a :b}' into '{a: b}', but does not touch other
uses of colons.  It does not touch other uses of whitespace.

"""

from .. import pytree
from ..pgen2 import token
from .import basefix

class FixWsComma(basefix.BaseFix):

  explicit = True # The user must ask for this fixers

  PATTERN = """
  any<(not(',') any)+ ',' ((not(',') any)+ ',')* [not(',') any]>
  """

  COMMA = pytree.Leaf(token.COMMA, ",")
  COLON = pytree.Leaf(token.COLON, ":")
  SEPS = (COMMA, COLON)

  def transform(self, node, results):
    new = node.clone()
    comma = False
    for child in new.children:
      if child in self.SEPS:
        prefix = child.get_prefix()
        if prefix.isspace() and "\n" not in prefix:
          child.set_prefix("")
        comma = True
      else:
        if comma:
          prefix = child.get_prefix()
          if not prefix:
            child.set_prefix(" ")
        comma = False
    return new
