import types
from consts import CO_VARARGS, CO_VARKEYWORDS

nodes = {}

def flatten(list):
  l = []
  for elt in list:
    if type(elt) is types.TupleType:
      for elt2 in flatten(elt):
        l.append(elt2)
    elif type(elt) is types.ListType:
      for elt2 in flatten(elt):
        l.append(elt2)
    else:
      l.append(elt)
  return l

def asList(nodes):
  l = []
  for item in nodes:
    if hasattr(item, "asList"):
      l.append(item.asList())
    else:
      if type(item) is types.TupleType:
        l.append(tuple(asList(item)))
      elif type(item) is types.ListType:
        l.append(asList(item))
      else:
        l.append(item)
  return l

class Node:
  def __init__(self, *args):
    self._children = args
    self.lineno = None
  def __getitem__(self, index):
    return self._children[index]
  def __repr__(self):
    return "<Node %s>" % self._children[0]
  def __len__(self):
    return len(self._children)
  def __getslice__(self, low, high):
    return self._children[low:high]
  def getChildren(self):
    return tuple(flatten(self._children[1:]))
  def getType(self):
    return self._children[0]
  def asList(self):
    return tuple(asList(self._children))

class EmptyNode(Node):
  def __init__(self):
    self.lineno = None

class Module(Node):
  nodes['module'] = 'Module'

  def __init__(self, doc, node):
    self.doc = doc
    self.node = node
    self._children = ('module', doc, node)

  def __repr__(self):
    return "Module(%s,%s)" % self._children[1:]

class Stmt(Node):
  nodes['stmt'] = 'Stmt'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('stmt', nodes)

  def __repr__(self):
    return "Stmt(%s)" % self._children[1:]

class Function(Node):
  nodes['function'] = 'Function'

  def __init__(self, name, argnames, defaults, flags, doc, code):
    self.name = name
    self.argnames = argnames
    self.defaults = defaults
    self.flags = flags
    self.doc = doc
    self.code = code
    self._children = ('function',
                       name, argnames, defaults, flags, doc, code)
    self.varargs = self.kwargs = None
    if flags & CO_VARARGS:
      self.varargs = 1
    if flags & CO_VARKEYWORDS:
      self.kwargs = 1
    

  def __repr__(self):
    return "Function(%s,%s,%s,%s,%s,%s)" % self._children[1:]

class Lambda(Node):
  nodes['lambda'] = 'Lambda'

  def __init__(self, argnames, defaults, flags, code):
    self.argnames = argnames
    self.defaults = defaults
    self.flags = flags
    self.code = code
    self._children = ('lambda', argnames, defaults, flags, code)
    self.varargs = self.kwargs = None
    if flags & CO_VARARGS:
      self.varargs = 1
    if flags & CO_VARKEYWORDS:
      self.kwargs = 1

  def __repr__(self):
    return "Lambda(%s,%s,%s,%s)" % self._children[1:]

class Class(Node):
  nodes['class'] = 'Class'

  def __init__(self, name, bases, doc, code):
    self.name = name
    self.bases = bases
    self.doc = doc
    self.code = code
    self._children = ('class', name, bases, doc, code)

  def __repr__(self):
    return "Class(%s,%s,%s,%s)" % self._children[1:]

class Pass(EmptyNode):
  nodes['pass'] = 'Pass'
  _children = ('pass',)
  def __repr__(self):
    return "Pass()"

class Break(EmptyNode):
  nodes['break'] = 'Break'
  _children = ('break',)
  def __repr__(self):
    return "Break()"

class Continue(EmptyNode):
  nodes['continue'] = 'Continue'
  _children = ('continue',)
  def __repr__(self):
    return "Continue()"

class For(Node):
  nodes['for'] = 'For'

  def __init__(self, assign, list, body, else_):
    self.assign = assign
    self.list = list
    self.body = body
    self.else_ = else_
    self._children = ('for', assign, list, body, else_)

  def __repr__(self):
    return "For(%s,%s,%s,%s)" % self._children[1:]

class While(Node):
  nodes['while'] = 'While'

  def __init__(self, test, body, else_):
    self.test = test
    self.body = body
    self.else_ = else_
    self._children = ('while', test, body, else_)

  def __repr__(self):
    return "While(%s,%s,%s)" % self._children[1:]

class If(Node):
  """if: [ (testNode, suiteNode), ...], elseNode"""
  nodes['if'] = 'If'

  def __init__(self, tests, else_):
    self.tests = tests
    self.else_ = else_
    self._children = ('if', tests, else_)

  def __repr__(self):
    return "If(%s,%s)" % self._children[1:]

class Exec(Node):
  nodes['exec'] = 'Exec'

  def __init__(self, expr, locals, globals):
    self.expr = expr
    self.locals = locals
    self.globals = globals
    self._children = ('exec', expr, locals, globals)

  def __repr__(self):
    return "Exec(%s,%s,%s)" % self._children[1:]

class From(Node):
  nodes['from'] = 'From'

  def __init__(self, modname, names):
    self.modname = modname
    self.names = names
    self._children = ('from', modname, names)

  def __repr__(self):
    return "From(%s,%s)" % self._children[1:]

class Import(Node):
  nodes['import'] = 'Import'

  def __init__(self, names):
    self.names = names
    self._children = ('import', names)

  def __repr__(self):
    return "Import(%s)" % self._children[1:]

class Raise(Node):
  nodes['raise'] = 'Raise'

  def __init__(self, expr1, expr2, expr3):
    self.expr1 = expr1
    self.expr2 = expr2
    self.expr3 = expr3
    self._children = ('raise', expr1, expr2, expr3)

  def __repr__(self):
    return "Raise(%s,%s,%s)" % self._children[1:]

class TryFinally(Node):
  nodes['tryfinally'] = 'TryFinally'

  def __init__(self, body, final):
    self.body = body
    self.final = final
    self._children = ('tryfinally', body, final)

  def __repr__(self):
    return "TryFinally(%s,%s)" % self._children[1:]

class TryExcept(Node):
  """Try/Except body and handlers

  The handlers attribute is a sequence of tuples.  The elements of the
  tuple are the exception name, the name to bind the exception to, and
  the body of the except clause.
  """
  nodes['tryexcept'] = 'TryExcept'

  def __init__(self, body, handlers, else_):
    self.body = body
    self.handlers = handlers
    self.else_ = else_
    self._children = ('tryexcept', body, handlers, else_)

  def __repr__(self):
    return "TryExcept(%s,%s,%s)" % self._children[1:]

class Return(Node):
  nodes['return'] = 'Return'

  def __init__(self, value):
    self.value = value
    self._children = ('return', value)

  def __repr__(self):
    return "Return(%s)" % self._children[1:]

class Const(Node):
  nodes['const'] = 'Const'

  def __init__(self, value):
    self.value = value
    self._children = ('const', value)

  def __repr__(self):
    return "Const(%s)" % self._children[1:]

class Print(Node):
  nodes['print'] = 'Print'

  def __init__(self, nodes, dest):
    self.nodes = nodes
    self.dest = dest
    self._children = ('print', nodes, dest)

  def __repr__(self):
    return "Print(%s, %s)" % (self._children[1:-1], self._children[-1])

class Printnl(Node):
  nodes['printnl'] = 'Printnl'

  def __init__(self, nodes, dest):
    self.nodes = nodes
    self.dest = dest
    self._children = ('printnl', nodes, dest)

  def __repr__(self):
    return "Printnl(%s, %s)" % (self._children[1:-1], self._children[-1])

class Discard(Node):
  nodes['discard'] = 'Discard'

  def __init__(self, expr):
    self.expr = expr
    self._children = ('discard', expr)

  def __repr__(self):
    return "Discard(%s)" % self._children[1:]

class AugAssign(Node):
  nodes['augassign'] = 'AugAssign'

  def __init__(self, node, op, expr):
    self.node = node
    self.op = op
    self.expr = expr
    self._children = ('augassign', node, op, expr)

  def __repr__(self):
    return "AugAssign(%s)" % str(self._children[1:])

class Assign(Node):
  nodes['assign'] = 'Assign'

  def __init__(self, nodes, expr):
    self.nodes = nodes
    self.expr = expr
    self._children = ('assign', nodes, expr)

  def __repr__(self):
    return "Assign(%s,%s)" % self._children[1:]

class AssTuple(Node):
  nodes['ass_tuple'] = 'AssTuple'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('ass_tuple', nodes)

  def __repr__(self):
    return "AssTuple(%s)" % self._children[1:]

class AssList(Node):
  nodes['ass_list'] = 'AssList'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('ass_list', nodes)

  def __repr__(self):
    return "AssList(%s)" % self._children[1:]

class AssName(Node):
  nodes['ass_name'] = 'AssName'

  def __init__(self, name, flags):
    self.name = name
    self.flags = flags
    self._children = ('ass_name', name, flags)

  def __repr__(self):
    return "AssName(%s,%s)" % self._children[1:]

class AssAttr(Node):
  nodes['ass_attr'] = 'AssAttr'

  def __init__(self, expr, attrname, flags):
    self.expr = expr
    self.attrname = attrname
    self.flags = flags
    self._children = ('ass_attr', expr, attrname, flags)

  def __repr__(self):
    return "AssAttr(%s,%s,%s)" % self._children[1:]

class ListComp(Node):
  nodes['listcomp'] = 'ListComp'

  def __init__(self, expr, quals):
    self.expr = expr
    self.quals = quals
    self._children = ('listcomp', expr, quals)

  def __repr__(self):
    return "ListComp(%s, %s)" % self._children[1:]

class ListCompFor(Node):
  nodes['listcomp_for'] = 'ListCompFor'

  # transformer fills in ifs after node is created

  def __init__(self, assign, list, ifs):
    self.assign = assign
    self.list = list
    self.ifs = ifs
    self._children = ('listcomp_for', assign, list, ifs)

  def __repr__(self):
    return "ListCompFor(%s, %s, %s)" % self._children[1:]

class ListCompIf(Node):
  nodes['listcomp_if'] = 'ListCompIf'

  def __init__(self, test):
    self.test = test
    self._children = ('listcomp_if', test)

  def __repr__(self):
    return "ListCompIf(%s)" % self._children[1:]

class List(Node):
  nodes['list'] = 'List'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('list', nodes)

  def __repr__(self):
    return "List(%s)" % self._children[1:]

class Dict(Node):
  nodes['dict'] = 'Dict'

  def __init__(self, items):
    self.items = items
    self._children = ('dict', items)

  def __repr__(self):
    return "Dict(%s)" % self._children[1:]

class Not(Node):
  nodes['not'] = 'Not'

  def __init__(self, expr):
    self.expr = expr
    self._children = ('not', expr)

  def __repr__(self):
    return "Not(%s)" % self._children[1:]

class Compare(Node):
  nodes['compare'] = 'Compare'

  def __init__(self, expr, ops):
    self.expr = expr
    self.ops = ops
    self._children = ('compare', expr, ops)

  def __repr__(self):
    return "Compare(%s,%s)" % self._children[1:]

class Name(Node):
  nodes['name'] = 'Name'

  def __init__(self, name):
    self.name = name
    self._children = ('name', name)

  def __repr__(self):
    return "Name(%s)" % self._children[1:]

class Global(Node):
  nodes['global'] = 'Global'

  def __init__(self, names):
    self.names = names
    self._children = ('global', names)

  def __repr__(self):
    return "Global(%s)" % self._children[1:]

class Backquote(Node):
  nodes['backquote'] = 'Backquote'

  def __init__(self, node):
    self.expr = node
    self._children = ('backquote', node)

  def __repr__(self):
    return "Backquote(%s)" % self._children[1:]

class Getattr(Node):
  nodes['getattr'] = 'Getattr'

  def __init__(self, expr, attrname):
    self.expr = expr
    self.attrname = attrname
    self._children = ('getattr', expr, attrname)

  def __repr__(self):
    return "Getattr(%s,%s)" % self._children[1:]

class CallFunc(Node):
  nodes['call_func'] = 'CallFunc'

  def __init__(self, node, args, star_args = None, dstar_args = None):
    self.node = node
    self.args = args
    self.star_args = star_args
    self.dstar_args = dstar_args
    self._children = ('call_func', node, args, star_args, dstar_args)

  def __repr__(self):
    return "CallFunc(%s,%s,*%s, **%s)" % self._children[1:]

class Keyword(Node):
  nodes['keyword'] = 'Keyword'

  def __init__(self, name, expr):
    self.name = name
    self.expr = expr
    self._children = ('keyword', name, expr)

  def __repr__(self):
    return "Keyword(%s,%s)" % self._children[1:]

class Subscript(Node):
  nodes['subscript'] = 'Subscript'

  def __init__(self, expr, flags, subs):
    self.expr = expr
    self.flags = flags
    self.subs = subs
    self._children = ('subscript', expr, flags, subs)

  def __repr__(self):
    return "Subscript(%s,%s,%s)" % self._children[1:]

class Ellipsis(EmptyNode):
  nodes['ellipsis'] = 'Ellipsis'
  _children = ('ellipsis',)
  def __repr__(self):
    return "Ellipsis()"

class Sliceobj(Node):
  nodes['sliceobj'] = 'Sliceobj'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('sliceobj', nodes)

  def __repr__(self):
    return "Sliceobj(%s)" % self._children[1:]

class Slice(Node):
  nodes['slice'] = 'Slice'

  def __init__(self, expr, flags, lower, upper):
    self.expr = expr
    self.flags = flags
    self.lower = lower
    self.upper = upper
    self._children = ('slice', expr, flags, lower, upper)

  def __repr__(self):
    return "Slice(%s,%s,%s,%s)" % self._children[1:]

class Assert(Node):
  nodes['assert'] = 'Assert'

  def __init__(self, test, fail):
    self.test = test
    self.fail = fail
    self._children = ('assert', test, fail)

  def __repr__(self):
    return "Assert(%s,%s)" % self._children[1:]

class Tuple(Node):
  nodes['tuple'] = 'Tuple'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('tuple', nodes)

  def __repr__(self):
    return "Tuple(%s)" % self._children[1:]

class Or(Node):
  nodes['or'] = 'Or'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('or', nodes)

  def __repr__(self):
    return "Or(%s)" % self._children[1:]

class And(Node):
  nodes['and'] = 'And'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('and', nodes)

  def __repr__(self):
    return "And(%s)" % self._children[1:]

class Bitor(Node):
  nodes['bitor'] = 'Bitor'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('bitor', nodes)

  def __repr__(self):
    return "Bitor(%s)" % self._children[1:]

class Bitxor(Node):
  nodes['bitxor'] = 'Bitxor'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('bitxor', nodes)

  def __repr__(self):
    return "Bitxor(%s)" % self._children[1:]

class Bitand(Node):
  nodes['bitand'] = 'Bitand'

  def __init__(self, nodes):
    self.nodes = nodes
    self._children = ('bitand', nodes)

  def __repr__(self):
    return "Bitand(%s)" % self._children[1:]

class LeftShift(Node):
  nodes['<<'] = 'LeftShift'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('<<', (left, right))

  def __repr__(self):
    return "LeftShift(%s)" % self._children[1:]

class RightShift(Node):
  nodes['>>'] = 'RightShift'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('>>', (left, right))

  def __repr__(self):
    return "RightShift(%s)" % self._children[1:]

class Add(Node):
  nodes['+'] = 'Add'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('+', (left, right))

  def __repr__(self):
    return "Add(%s)" % self._children[1:]

class Sub(Node):
  nodes['-'] = 'Sub'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('-', (left, right))

  def __repr__(self):
    return "Sub(%s)" % self._children[1:]

class Mul(Node):
  nodes['*'] = 'Mul'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('*', (left, right))

  def __repr__(self):
    return "Mul(%s)" % self._children[1:]

class Div(Node):
  nodes['/'] = 'Div'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('/', (left, right))

  def __repr__(self):
    return "Div(%s)" % self._children[1:]

class Mod(Node):
  nodes['%'] = 'Mod'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('%', (left, right))

  def __repr__(self):
    return "Mod(%s)" % self._children[1:]

class Power(Node):
  nodes['power'] = 'Power'

  def __init__(self, (left, right)):
    self.left = left
    self.right = right
    self._children = ('power', (left, right))

  def __repr__(self):
    return "Power(%s)" % self._children[1:]

class UnaryAdd(Node):
  nodes['unary+'] = 'UnaryAdd'

  def __init__(self, node):
    self.expr = node
    self._children = ('unary+', node)

  def __repr__(self):
    return "UnaryAdd(%s)" % self._children[1:]

class UnarySub(Node):
  nodes['unary-'] = 'UnarySub'

  def __init__(self, node):
    self.expr = node
    self._children = ('unary-', node)

  def __repr__(self):
    return "UnarySub(%s)" % self._children[1:]

class Invert(Node):
  nodes['invert'] = 'Invert'

  def __init__(self, node):
    self.expr = node
    self._children = ('invert', node)

  def __repr__(self):
    return "Invert(%s)" % self._children[1:]

# now clean up the nodes dictionary
klasses = globals()
for k in nodes.keys():
  nodes[k] = klasses[nodes[k]]

# Local Variables:          
# mode:python               
# indent-tabs-mode: nil     
# py-indent-offset: 2       
# py-smart-indentation: nil 
# End:                      
