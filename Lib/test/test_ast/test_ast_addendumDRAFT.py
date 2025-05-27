import ast

check_node(
        ast.Add(),
        empty="ast.Add()",
        full="ast.Add()"
)

check_node(
        ast.alias(name='name'),
        empty="ast.alias(name='name')",
        full="ast.alias(name='name', asname=None)"
)

check_node(
        ast.And(),
        empty="ast.And()",
        full="ast.And()"
)

check_node(
        ast.AnnAssign(target=ast.Name(id='target', ctx=ast.Load()), annotation=ast.Name(id='annotation', ctx=ast.Load()), simple=0),
        empty="ast.AnnAssign(target=ast.Name(id='target', ctx=ast.Load()), annotation=ast.Name(id='annotation', ctx=ast.Load()), simple=0)",
        full="ast.AnnAssign(target=ast.Name(id='target', ctx=ast.Load()), annotation=ast.Name(id='annotation', ctx=ast.Load()), value=None, simple=0)"
)

check_node(
        ast.arg(arg='arg'),
        empty="ast.arg(arg='arg')",
        full="ast.arg(arg='arg', annotation=None, type_comment=None)"
)

check_node(
        ast.arguments(),
        empty="ast.arguments()",
        full="ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[])"
)

check_node(
        ast.Assert(test=ast.Constant(value=None)),
        empty="ast.Assert(test=ast.Constant(value=None))",
        full="ast.Assert(test=ast.Constant(value=None, kind=None), msg=None)"
)

check_node(
        ast.Assign(value=ast.Constant(value=None)),
        empty="ast.Assign(value=ast.Constant(value=None))",
        full="ast.Assign(targets=[], value=ast.Constant(value=None, kind=None), type_comment=None)"
)

check_node(
        ast.AsyncFor(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load())),
        empty="ast.AsyncFor(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()))",
        full="ast.AsyncFor(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), body=[], orelse=[], type_comment=None)"
)

check_node(
        ast.AsyncFunctionDef(name='name', args=ast.arguments()),
        empty="ast.AsyncFunctionDef(name='name', args=ast.arguments())",
        full="ast.AsyncFunctionDef(name='name', args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=[], decorator_list=[], returns=None, type_comment=None, type_params=[])"
)

check_node(
        ast.AsyncWith(),
        empty="ast.AsyncWith()",
        full="ast.AsyncWith(items=[], body=[], type_comment=None)"
)

check_node(
        ast.Attribute(value=ast.Name(id='value', ctx=ast.Load()), attr='attr', ctx=ast.Load()),
        empty="ast.Attribute(value=ast.Name(id='value', ctx=ast.Load()), attr='attr', ctx=ast.Load())",
        full="ast.Attribute(value=ast.Name(id='value', ctx=ast.Load()), attr='attr', ctx=ast.Load())"
)

check_node(
        ast.AugAssign(target=ast.Name(id='target', ctx=ast.Load()), op=ast.Add(), value=ast.Constant(value=None)),
        empty="ast.AugAssign(target=ast.Name(id='target', ctx=ast.Load()), op=ast.Add(), value=ast.Constant(value=None))",
        full="ast.AugAssign(target=ast.Name(id='target', ctx=ast.Load()), op=ast.Add(), value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Await(value=ast.Constant(value=None)),
        empty="ast.Await(value=ast.Constant(value=None))",
        full="ast.Await(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.BinOp(left=ast.Constant(value=None), op=ast.Add(), right=ast.Constant(value=None)),
        empty="ast.BinOp(left=ast.Constant(value=None), op=ast.Add(), right=ast.Constant(value=None))",
        full="ast.BinOp(left=ast.Constant(value=None, kind=None), op=ast.Add(), right=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.BitAnd(),
        empty="ast.BitAnd()",
        full="ast.BitAnd()"
)

check_node(
        ast.BitOr(),
        empty="ast.BitOr()",
        full="ast.BitOr()"
)

check_node(
        ast.BitXor(),
        empty="ast.BitXor()",
        full="ast.BitXor()"
)

check_node(
        ast.boolop(),
        empty="ast.boolop()",
        full="ast.boolop()"
)

check_node(
        ast.BoolOp(op=ast.And()),
        empty="ast.BoolOp(op=ast.And())",
        full="ast.BoolOp(op=ast.And(), values=[])"
)

check_node(
        ast.Break(),
        empty="ast.Break()",
        full="ast.Break()"
)

check_node(
        ast.Call(func=ast.Name(id='func', ctx=ast.Load())),
        empty="ast.Call(func=ast.Name(id='func', ctx=ast.Load()))",
        full="ast.Call(func=ast.Name(id='func', ctx=ast.Load()), args=[], keywords=[])"
)

check_node(
        ast.ClassDef(name='name'),
        empty="ast.ClassDef(name='name')",
        full="ast.ClassDef(name='name', bases=[], keywords=[], body=[], decorator_list=[], type_params=[])"
)

check_node(
        ast.cmpop(),
        empty="ast.cmpop()",
        full="ast.cmpop()"
)

check_node(
        ast.Compare(left=ast.Constant(value=None)),
        empty="ast.Compare(left=ast.Constant(value=None))",
        full="ast.Compare(left=ast.Constant(value=None, kind=None), ops=[], comparators=[])"
)

check_node(
        ast.comprehension(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), is_async=0),
        empty="ast.comprehension(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), is_async=0)",
        full="ast.comprehension(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), ifs=[], is_async=0)"
)

check_node(
        ast.Constant(value=None),
        empty="ast.Constant(value=None)",
        full="ast.Constant(value=None, kind=None)"
)

check_node(
        ast.Continue(),
        empty="ast.Continue()",
        full="ast.Continue()"
)

check_node(
        ast.Del(),
        empty="ast.Del()",
        full="ast.Del()"
)

check_node(
        ast.Delete(),
        empty="ast.Delete()",
        full="ast.Delete(targets=[])"
)

check_node(
        ast.Dict(),
        empty="ast.Dict()",
        full="ast.Dict(keys=[], values=[])"
)

check_node(
        ast.DictComp(key=ast.Constant(value=None), value=ast.Constant(value=None)),
        empty="ast.DictComp(key=ast.Constant(value=None), value=ast.Constant(value=None))",
        full="ast.DictComp(key=ast.Constant(value=None, kind=None), value=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Div(),
        empty="ast.Div()",
        full="ast.Div()"
)

check_node(
        ast.Eq(),
        empty="ast.Eq()",
        full="ast.Eq()"
)

check_node(
        ast.excepthandler(),
        empty="ast.excepthandler()",
        full="ast.excepthandler()"
)

check_node(
        ast.ExceptHandler(),
        empty="ast.ExceptHandler()",
        full="ast.ExceptHandler(type=None, name=None, body=[])"
)

check_node(
        ast.expr_context(),
        empty="ast.expr_context()",
        full="ast.expr_context()"
)

check_node(
        ast.expr(),
        empty="ast.expr()",
        full="ast.expr()"
)

check_node(
        ast.Expr(value=ast.Constant(value=None)),
        empty="ast.Expr(value=ast.Constant(value=None))",
        full="ast.Expr(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Expression(body=ast.Constant(value=None)),
        empty="ast.Expression(body=ast.Constant(value=None))",
        full="ast.Expression(body=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.FloorDiv(),
        empty="ast.FloorDiv()",
        full="ast.FloorDiv()"
)

check_node(
        ast.For(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load())),
        empty="ast.For(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()))",
        full="ast.For(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), body=[], orelse=[], type_comment=None)"
)

check_node(
        ast.FormattedValue(value=ast.Constant(value=None), conversion=0),
        empty="ast.FormattedValue(value=ast.Constant(value=None), conversion=0)",
        full="ast.FormattedValue(value=ast.Constant(value=None, kind=None), conversion=0, format_spec=None)"
)

check_node(
        ast.FunctionDef(name='name', args=ast.arguments()),
        empty="ast.FunctionDef(name='name', args=ast.arguments())",
        full="ast.FunctionDef(name='name', args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=[], decorator_list=[], returns=None, type_comment=None, type_params=[])"
)

check_node(
        ast.FunctionType(returns=ast.Constant(value=None)),
        empty="ast.FunctionType(returns=ast.Constant(value=None))",
        full="ast.FunctionType(argtypes=[], returns=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.GeneratorExp(elt=ast.Constant(value=None)),
        empty="ast.GeneratorExp(elt=ast.Constant(value=None))",
        full="ast.GeneratorExp(elt=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Global(),
        empty="ast.Global()",
        full="ast.Global(names=[])"
)

check_node(
        ast.Gt(),
        empty="ast.Gt()",
        full="ast.Gt()"
)

check_node(
        ast.GtE(),
        empty="ast.GtE()",
        full="ast.GtE()"
)

check_node(
        ast.If(test=ast.Constant(value=None)),
        empty="ast.If(test=ast.Constant(value=None))",
        full="ast.If(test=ast.Constant(value=None, kind=None), body=[], orelse=[])"
)

check_node(
        ast.IfExp(test=ast.Constant(value=None), body=ast.Constant(value=None), orelse=ast.Constant(value=None)),
        empty="ast.IfExp(test=ast.Constant(value=None), body=ast.Constant(value=None), orelse=ast.Constant(value=None))",
        full="ast.IfExp(test=ast.Constant(value=None, kind=None), body=ast.Constant(value=None, kind=None), orelse=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Import(),
        empty="ast.Import()",
        full="ast.Import(names=[])"
)

check_node(
        ast.ImportFrom(level=0),
        empty="ast.ImportFrom(level=0)",
        full="ast.ImportFrom(module=None, names=[], level=0)"
)

check_node(
        ast.In(),
        empty="ast.In()",
        full="ast.In()"
)

check_node(
        ast.Interactive(),
        empty="ast.Interactive()",
        full="ast.Interactive(body=[])"
)

check_node(
        ast.Invert(),
        empty="ast.Invert()",
        full="ast.Invert()"
)

check_node(
        ast.Is(),
        empty="ast.Is()",
        full="ast.Is()"
)

check_node(
        ast.IsNot(),
        empty="ast.IsNot()",
        full="ast.IsNot()"
)

check_node(
        ast.JoinedStr(),
        empty="ast.JoinedStr()",
        full="ast.JoinedStr(values=[])"
)

check_node(
        ast.keyword(value=ast.Constant(value=None)),
        empty="ast.keyword(value=ast.Constant(value=None))",
        full="ast.keyword(arg=None, value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Lambda(args=ast.arguments(), body=ast.Constant(value=None)),
        empty="ast.Lambda(args=ast.arguments(), body=ast.Constant(value=None))",
        full="ast.Lambda(args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.List(ctx=ast.Load()),
        empty="ast.List(ctx=ast.Load())",
        full="ast.List(elts=[], ctx=ast.Load())"
)

check_node(
        ast.ListComp(elt=ast.Constant(value=None)),
        empty="ast.ListComp(elt=ast.Constant(value=None))",
        full="ast.ListComp(elt=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Load(),
        empty="ast.Load()",
        full="ast.Load()"
)

check_node(
        ast.LShift(),
        empty="ast.LShift()",
        full="ast.LShift()"
)

check_node(
        ast.Lt(),
        empty="ast.Lt()",
        full="ast.Lt()"
)

check_node(
        ast.LtE(),
        empty="ast.LtE()",
        full="ast.LtE()"
)

check_node(
        ast.match_case(pattern=ast.MatchValue(value=ast.Constant(value=None))),
        empty="ast.match_case(pattern=ast.MatchValue(value=ast.Constant(value=None)))",
        full="ast.match_case(pattern=ast.MatchValue(value=ast.Constant(value=None, kind=None)), guard=None, body=[])"
)

check_node(
        ast.Match(subject=ast.Constant(value=None)),
        empty="ast.Match(subject=ast.Constant(value=None))",
        full="ast.Match(subject=ast.Constant(value=None, kind=None), cases=[])"
)

check_node(
        ast.MatchAs(),
        empty="ast.MatchAs()",
        full="ast.MatchAs(pattern=None, name=None)"
)

check_node(
        ast.MatchClass(cls=ast.Name(id='cls', ctx=ast.Load())),
        empty="ast.MatchClass(cls=ast.Name(id='cls', ctx=ast.Load()))",
        full="ast.MatchClass(cls=ast.Name(id='cls', ctx=ast.Load()), patterns=[], kwd_attrs=[], kwd_patterns=[])"
)

check_node(
        ast.MatchMapping(),
        empty="ast.MatchMapping()",
        full="ast.MatchMapping(keys=[], patterns=[], rest=None)"
)

check_node(
        ast.MatchOr(),
        empty="ast.MatchOr()",
        full="ast.MatchOr(patterns=[])"
)

check_node(
        ast.MatchSequence(),
        empty="ast.MatchSequence()",
        full="ast.MatchSequence(patterns=[])"
)

check_node(
        ast.MatchSingleton(value=None),
        empty="ast.MatchSingleton(value=None)",
        full="ast.MatchSingleton(value=None)"
)

check_node(
        ast.MatchStar(),
        empty="ast.MatchStar()",
        full="ast.MatchStar(name=None)"
)

check_node(
        ast.MatchValue(value=ast.Constant(value=None)),
        empty="ast.MatchValue(value=ast.Constant(value=None))",
        full="ast.MatchValue(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.MatMult(),
        empty="ast.MatMult()",
        full="ast.MatMult()"
)

check_node(
        ast.Mod(),
        empty="ast.Mod()",
        full="ast.Mod()"
)

check_node(
        ast.mod(),
        empty="ast.mod()",
        full="ast.mod()"
)

check_node(
        ast.Module(),
        empty="ast.Module()",
        full="ast.Module(body=[], type_ignores=[])"
)

check_node(
        ast.Mult(),
        empty="ast.Mult()",
        full="ast.Mult()"
)

check_node(
        ast.Name(id='id', ctx=ast.Load()),
        empty="ast.Name(id='id', ctx=ast.Load())",
        full="ast.Name(id='id', ctx=ast.Load())"
)

check_node(
        ast.NamedExpr(target=ast.Name(id='target', ctx=ast.Load()), value=ast.Constant(value=None)),
        empty="ast.NamedExpr(target=ast.Name(id='target', ctx=ast.Load()), value=ast.Constant(value=None))",
        full="ast.NamedExpr(target=ast.Name(id='target', ctx=ast.Load()), value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Nonlocal(),
        empty="ast.Nonlocal()",
        full="ast.Nonlocal(names=[])"
)

check_node(
        ast.Not(),
        empty="ast.Not()",
        full="ast.Not()"
)

check_node(
        ast.NotEq(),
        empty="ast.NotEq()",
        full="ast.NotEq()"
)

check_node(
        ast.NotIn(),
        empty="ast.NotIn()",
        full="ast.NotIn()"
)

check_node(
        ast.operator(),
        empty="ast.operator()",
        full="ast.operator()"
)

check_node(
        ast.Or(),
        empty="ast.Or()",
        full="ast.Or()"
)

check_node(
        ast.ParamSpec(name='name'),
        empty="ast.ParamSpec(name='name')",
        full="ast.ParamSpec(name='name', default_value=None)"
)

check_node(
        ast.Pass(),
        empty="ast.Pass()",
        full="ast.Pass()"
)

check_node(
        ast.pattern(),
        empty="ast.pattern()",
        full="ast.pattern()"
)

check_node(
        ast.Pow(),
        empty="ast.Pow()",
        full="ast.Pow()"
)

check_node(
        ast.Raise(),
        empty="ast.Raise()",
        full="ast.Raise(exc=None, cause=None)"
)

check_node(
        ast.Return(),
        empty="ast.Return()",
        full="ast.Return(value=None)"
)

check_node(
        ast.RShift(),
        empty="ast.RShift()",
        full="ast.RShift()"
)

check_node(
        ast.Set(),
        empty="ast.Set()",
        full="ast.Set(elts=[])"
)

check_node(
        ast.SetComp(elt=ast.Constant(value=None)),
        empty="ast.SetComp(elt=ast.Constant(value=None))",
        full="ast.SetComp(elt=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Slice(),
        empty="ast.Slice()",
        full="ast.Slice(lower=None, upper=None, step=None)"
)

check_node(
        ast.Starred(value=ast.Name(id='value', ctx=ast.Load()), ctx=ast.Load()),
        empty="ast.Starred(value=ast.Name(id='value', ctx=ast.Load()), ctx=ast.Load())",
        full="ast.Starred(value=ast.Name(id='value', ctx=ast.Load()), ctx=ast.Load())"
)

check_node(
        ast.stmt(),
        empty="ast.stmt()",
        full="ast.stmt()"
)

check_node(
        ast.Store(),
        empty="ast.Store()",
        full="ast.Store()"
)

check_node(
        ast.Sub(),
        empty="ast.Sub()",
        full="ast.Sub()"
)

check_node(
        ast.Subscript(value=ast.Name(id='value', ctx=ast.Load()), slice=ast.Constant(value=None), ctx=ast.Load()),
        empty="ast.Subscript(value=ast.Name(id='value', ctx=ast.Load()), slice=ast.Constant(value=None), ctx=ast.Load())",
        full="ast.Subscript(value=ast.Name(id='value', ctx=ast.Load()), slice=ast.Constant(value=None, kind=None), ctx=ast.Load())"
)

check_node(
        ast.Try(),
        empty="ast.Try()",
        full="ast.Try(body=[], handlers=[], orelse=[], finalbody=[])"
)

check_node(
        ast.TryStar(),
        empty="ast.TryStar()",
        full="ast.TryStar(body=[], handlers=[], orelse=[], finalbody=[])"
)

check_node(
        ast.Tuple(ctx=ast.Load()),
        empty="ast.Tuple(ctx=ast.Load())",
        full="ast.Tuple(elts=[], ctx=ast.Load())"
)

check_node(
        ast.type_ignore(),
        empty="ast.type_ignore()",
        full="ast.type_ignore()"
)

check_node(
        ast.type_param(),
        empty="ast.type_param()",
        full="ast.type_param()"
)

check_node(
        ast.TypeAlias(name=ast.Name(id='name', ctx=ast.Load()), value=ast.Name(id='value', ctx=ast.Load())),
        empty="ast.TypeAlias(name=ast.Name(id='name', ctx=ast.Load()), value=ast.Name(id='value', ctx=ast.Load()))",
        full="ast.TypeAlias(name=ast.Name(id='name', ctx=ast.Load()), type_params=[], value=ast.Name(id='value', ctx=ast.Load()))"
)

check_node(
        ast.TypeIgnore(lineno=1, tag=''),
        empty="ast.TypeIgnore(lineno=1, tag='')",
        full="ast.TypeIgnore(lineno=1, tag='')"
)

check_node(
        ast.TypeVar(name='name'),
        empty="ast.TypeVar(name='name')",
        full="ast.TypeVar(name='name', bound=None, default_value=None)"
)

check_node(
        ast.TypeVarTuple(name='name'),
        empty="ast.TypeVarTuple(name='name')",
        full="ast.TypeVarTuple(name='name', default_value=None)"
)

check_node(
        ast.UAdd(),
        empty="ast.UAdd()",
        full="ast.UAdd()"
)

check_node(
        ast.unaryop(),
        empty="ast.unaryop()",
        full="ast.unaryop()"
)

check_node(
        ast.UnaryOp(op=ast.Not(), operand=ast.Constant(value=None)),
        empty="ast.UnaryOp(op=ast.Not(), operand=ast.Constant(value=None))",
        full="ast.UnaryOp(op=ast.Not(), operand=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.USub(),
        empty="ast.USub()",
        full="ast.USub()"
)

check_node(
        ast.While(test=ast.Constant(value=None)),
        empty="ast.While(test=ast.Constant(value=None))",
        full="ast.While(test=ast.Constant(value=None, kind=None), body=[], orelse=[])"
)

check_node(
        ast.With(),
        empty="ast.With()",
        full="ast.With(items=[], body=[], type_comment=None)"
)

check_node(
        ast.withitem(context_expr=ast.Name(id='name', ctx=ast.Load())),
        empty="ast.withitem(context_expr=ast.Name(id='name', ctx=ast.Load()))",
        full="ast.withitem(context_expr=ast.Name(id='name', ctx=ast.Load()), optional_vars=None)"
)

check_node(
        ast.Yield(),
        empty="ast.Yield()",
        full="ast.Yield(value=None)"
)

check_node(
        ast.YieldFrom(value=ast.Constant(value=None)),
        empty="ast.YieldFrom(value=ast.Constant(value=None))",
        full="ast.YieldFrom(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Add(),
        empty="ast.Add()",
        full="ast.Add()"
)

check_node(
        ast.alias(name='name'),
        empty="ast.alias(name='name')",
        full="ast.alias(name='name', asname=None)"
)

check_node(
        ast.And(),
        empty="ast.And()",
        full="ast.And()"
)

check_node(
        ast.AnnAssign(target=ast.Name(id='target', ctx=ast.Load()), annotation=ast.Name(id='annotation', ctx=ast.Load()), simple=0),
        empty="ast.AnnAssign(target=ast.Name(id='target', ctx=ast.Load()), annotation=ast.Name(id='annotation', ctx=ast.Load()), simple=0)",
        full="ast.AnnAssign(target=ast.Name(id='target', ctx=ast.Load()), annotation=ast.Name(id='annotation', ctx=ast.Load()), value=None, simple=0)"
)

check_node(
        ast.arg(arg='arg'),
        empty="ast.arg(arg='arg')",
        full="ast.arg(arg='arg', annotation=None, type_comment=None)"
)

check_node(
        ast.arguments(),
        empty="ast.arguments()",
        full="ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[])"
)

check_node(
        ast.Assert(test=ast.Constant(value=None)),
        empty="ast.Assert(test=ast.Constant(value=None))",
        full="ast.Assert(test=ast.Constant(value=None, kind=None), msg=None)"
)

check_node(
        ast.Assign(value=ast.Constant(value=None)),
        empty="ast.Assign(value=ast.Constant(value=None))",
        full="ast.Assign(targets=[], value=ast.Constant(value=None, kind=None), type_comment=None)"
)

check_node(
        ast.AsyncFor(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load())),
        empty="ast.AsyncFor(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()))",
        full="ast.AsyncFor(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), body=[], orelse=[], type_comment=None)"
)

check_node(
        ast.AsyncFunctionDef(name='name', args=ast.arguments()),
        empty="ast.AsyncFunctionDef(name='name', args=ast.arguments())",
        full="ast.AsyncFunctionDef(name='name', args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=[], decorator_list=[], returns=None, type_comment=None, type_params=[])"
)

check_node(
        ast.AsyncWith(),
        empty="ast.AsyncWith()",
        full="ast.AsyncWith(items=[], body=[], type_comment=None)"
)

check_node(
        ast.Attribute(value=ast.Name(id='value', ctx=ast.Load()), attr='attr', ctx=ast.Load()),
        empty="ast.Attribute(value=ast.Name(id='value', ctx=ast.Load()), attr='attr', ctx=ast.Load())",
        full="ast.Attribute(value=ast.Name(id='value', ctx=ast.Load()), attr='attr', ctx=ast.Load())"
)

check_node(
        ast.AugAssign(target=ast.Name(id='target', ctx=ast.Load()), op=ast.Add(), value=ast.Constant(value=None)),
        empty="ast.AugAssign(target=ast.Name(id='target', ctx=ast.Load()), op=ast.Add(), value=ast.Constant(value=None))",
        full="ast.AugAssign(target=ast.Name(id='target', ctx=ast.Load()), op=ast.Add(), value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Await(value=ast.Constant(value=None)),
        empty="ast.Await(value=ast.Constant(value=None))",
        full="ast.Await(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.BinOp(left=ast.Constant(value=None), op=ast.Add(), right=ast.Constant(value=None)),
        empty="ast.BinOp(left=ast.Constant(value=None), op=ast.Add(), right=ast.Constant(value=None))",
        full="ast.BinOp(left=ast.Constant(value=None, kind=None), op=ast.Add(), right=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.BitAnd(),
        empty="ast.BitAnd()",
        full="ast.BitAnd()"
)

check_node(
        ast.BitOr(),
        empty="ast.BitOr()",
        full="ast.BitOr()"
)

check_node(
        ast.BitXor(),
        empty="ast.BitXor()",
        full="ast.BitXor()"
)

check_node(
        ast.boolop(),
        empty="ast.boolop()",
        full="ast.boolop()"
)

check_node(
        ast.BoolOp(op=ast.And()),
        empty="ast.BoolOp(op=ast.And())",
        full="ast.BoolOp(op=ast.And(), values=[])"
)

check_node(
        ast.Break(),
        empty="ast.Break()",
        full="ast.Break()"
)

check_node(
        ast.Call(func=ast.Name(id='func', ctx=ast.Load())),
        empty="ast.Call(func=ast.Name(id='func', ctx=ast.Load()))",
        full="ast.Call(func=ast.Name(id='func', ctx=ast.Load()), args=[], keywords=[])"
)

check_node(
        ast.ClassDef(name='name'),
        empty="ast.ClassDef(name='name')",
        full="ast.ClassDef(name='name', bases=[], keywords=[], body=[], decorator_list=[], type_params=[])"
)

check_node(
        ast.cmpop(),
        empty="ast.cmpop()",
        full="ast.cmpop()"
)

check_node(
        ast.Compare(left=ast.Constant(value=None)),
        empty="ast.Compare(left=ast.Constant(value=None))",
        full="ast.Compare(left=ast.Constant(value=None, kind=None), ops=[], comparators=[])"
)

check_node(
        ast.comprehension(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), is_async=0),
        empty="ast.comprehension(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), is_async=0)",
        full="ast.comprehension(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), ifs=[], is_async=0)"
)

check_node(
        ast.Constant(value=None),
        empty="ast.Constant(value=None)",
        full="ast.Constant(value=None, kind=None)"
)

check_node(
        ast.Continue(),
        empty="ast.Continue()",
        full="ast.Continue()"
)

check_node(
        ast.Del(),
        empty="ast.Del()",
        full="ast.Del()"
)

check_node(
        ast.Delete(),
        empty="ast.Delete()",
        full="ast.Delete(targets=[])"
)

check_node(
        ast.Dict(),
        empty="ast.Dict()",
        full="ast.Dict(keys=[], values=[])"
)

check_node(
        ast.DictComp(key=ast.Constant(value=None), value=ast.Constant(value=None)),
        empty="ast.DictComp(key=ast.Constant(value=None), value=ast.Constant(value=None))",
        full="ast.DictComp(key=ast.Constant(value=None, kind=None), value=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Div(),
        empty="ast.Div()",
        full="ast.Div()"
)

check_node(
        ast.Eq(),
        empty="ast.Eq()",
        full="ast.Eq()"
)

check_node(
        ast.excepthandler(),
        empty="ast.excepthandler()",
        full="ast.excepthandler()"
)

check_node(
        ast.ExceptHandler(),
        empty="ast.ExceptHandler()",
        full="ast.ExceptHandler(type=None, name=None, body=[])"
)

check_node(
        ast.expr_context(),
        empty="ast.expr_context()",
        full="ast.expr_context()"
)

check_node(
        ast.expr(),
        empty="ast.expr()",
        full="ast.expr()"
)

check_node(
        ast.Expr(value=ast.Constant(value=None)),
        empty="ast.Expr(value=ast.Constant(value=None))",
        full="ast.Expr(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Expression(body=ast.Constant(value=None)),
        empty="ast.Expression(body=ast.Constant(value=None))",
        full="ast.Expression(body=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.FloorDiv(),
        empty="ast.FloorDiv()",
        full="ast.FloorDiv()"
)

check_node(
        ast.For(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load())),
        empty="ast.For(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()))",
        full="ast.For(target=ast.Name(id='target', ctx=ast.Load()), iter=ast.Name(id='iter', ctx=ast.Load()), body=[], orelse=[], type_comment=None)"
)

check_node(
        ast.FormattedValue(value=ast.Constant(value=None), conversion=0),
        empty="ast.FormattedValue(value=ast.Constant(value=None), conversion=0)",
        full="ast.FormattedValue(value=ast.Constant(value=None, kind=None), conversion=0, format_spec=None)"
)

check_node(
        ast.FunctionDef(name='name', args=ast.arguments()),
        empty="ast.FunctionDef(name='name', args=ast.arguments())",
        full="ast.FunctionDef(name='name', args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=[], decorator_list=[], returns=None, type_comment=None, type_params=[])"
)

check_node(
        ast.FunctionType(returns=ast.Constant(value=None)),
        empty="ast.FunctionType(returns=ast.Constant(value=None))",
        full="ast.FunctionType(argtypes=[], returns=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.GeneratorExp(elt=ast.Constant(value=None)),
        empty="ast.GeneratorExp(elt=ast.Constant(value=None))",
        full="ast.GeneratorExp(elt=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Global(),
        empty="ast.Global()",
        full="ast.Global(names=[])"
)

check_node(
        ast.Gt(),
        empty="ast.Gt()",
        full="ast.Gt()"
)

check_node(
        ast.GtE(),
        empty="ast.GtE()",
        full="ast.GtE()"
)

check_node(
        ast.If(test=ast.Constant(value=None)),
        empty="ast.If(test=ast.Constant(value=None))",
        full="ast.If(test=ast.Constant(value=None, kind=None), body=[], orelse=[])"
)

check_node(
        ast.IfExp(test=ast.Constant(value=None), body=ast.Constant(value=None), orelse=ast.Constant(value=None)),
        empty="ast.IfExp(test=ast.Constant(value=None), body=ast.Constant(value=None), orelse=ast.Constant(value=None))",
        full="ast.IfExp(test=ast.Constant(value=None, kind=None), body=ast.Constant(value=None, kind=None), orelse=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Import(),
        empty="ast.Import()",
        full="ast.Import(names=[])"
)

check_node(
        ast.ImportFrom(),
        empty="ast.ImportFrom()",
        full="ast.ImportFrom(module=None, names=[], level=None)"
)

check_node(
        ast.In(),
        empty="ast.In()",
        full="ast.In()"
)

check_node(
        ast.Interactive(),
        empty="ast.Interactive()",
        full="ast.Interactive(body=[])"
)

check_node(
        ast.Invert(),
        empty="ast.Invert()",
        full="ast.Invert()"
)

check_node(
        ast.Is(),
        empty="ast.Is()",
        full="ast.Is()"
)

check_node(
        ast.IsNot(),
        empty="ast.IsNot()",
        full="ast.IsNot()"
)

check_node(
        ast.JoinedStr(),
        empty="ast.JoinedStr()",
        full="ast.JoinedStr(values=[])"
)

check_node(
        ast.keyword(value=ast.Constant(value=None)),
        empty="ast.keyword(value=ast.Constant(value=None))",
        full="ast.keyword(arg=None, value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Lambda(args=ast.arguments(), body=ast.Constant(value=None)),
        empty="ast.Lambda(args=ast.arguments(), body=ast.Constant(value=None))",
        full="ast.Lambda(args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.List(ctx=ast.Load()),
        empty="ast.List(ctx=ast.Load())",
        full="ast.List(elts=[], ctx=ast.Load())"
)

check_node(
        ast.ListComp(elt=ast.Constant(value=None)),
        empty="ast.ListComp(elt=ast.Constant(value=None))",
        full="ast.ListComp(elt=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Load(),
        empty="ast.Load()",
        full="ast.Load()"
)

check_node(
        ast.LShift(),
        empty="ast.LShift()",
        full="ast.LShift()"
)

check_node(
        ast.Lt(),
        empty="ast.Lt()",
        full="ast.Lt()"
)

check_node(
        ast.LtE(),
        empty="ast.LtE()",
        full="ast.LtE()"
)

check_node(
        ast.match_case(pattern=ast.MatchValue(value=ast.Constant(value=None))),
        empty="ast.match_case(pattern=ast.MatchValue(value=ast.Constant(value=None)))",
        full="ast.match_case(pattern=ast.MatchValue(value=ast.Constant(value=None, kind=None)), guard=None, body=[])"
)

check_node(
        ast.Match(subject=ast.Constant(value=None)),
        empty="ast.Match(subject=ast.Constant(value=None))",
        full="ast.Match(subject=ast.Constant(value=None, kind=None), cases=[])"
)

check_node(
        ast.MatchAs(),
        empty="ast.MatchAs()",
        full="ast.MatchAs(pattern=None, name=None)"
)

check_node(
        ast.MatchClass(cls=ast.Name(id='cls', ctx=ast.Load())),
        empty="ast.MatchClass(cls=ast.Name(id='cls', ctx=ast.Load()))",
        full="ast.MatchClass(cls=ast.Name(id='cls', ctx=ast.Load()), patterns=[], kwd_attrs=[], kwd_patterns=[])"
)

check_node(
        ast.MatchMapping(),
        empty="ast.MatchMapping()",
        full="ast.MatchMapping(keys=[], patterns=[], rest=None)"
)

check_node(
        ast.MatchOr(),
        empty="ast.MatchOr()",
        full="ast.MatchOr(patterns=[])"
)

check_node(
        ast.MatchSequence(),
        empty="ast.MatchSequence()",
        full="ast.MatchSequence(patterns=[])"
)

check_node(
        ast.MatchSingleton(value=None),
        empty="ast.MatchSingleton(value=None)",
        full="ast.MatchSingleton(value=None)"
)

check_node(
        ast.MatchStar(),
        empty="ast.MatchStar()",
        full="ast.MatchStar(name=None)"
)

check_node(
        ast.MatchValue(value=ast.Constant(value=None)),
        empty="ast.MatchValue(value=ast.Constant(value=None))",
        full="ast.MatchValue(value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.MatMult(),
        empty="ast.MatMult()",
        full="ast.MatMult()"
)

check_node(
        ast.Mod(),
        empty="ast.Mod()",
        full="ast.Mod()"
)

check_node(
        ast.mod(),
        empty="ast.mod()",
        full="ast.mod()"
)

check_node(
        ast.Module(),
        empty="ast.Module()",
        full="ast.Module(body=[], type_ignores=[])"
)

check_node(
        ast.Mult(),
        empty="ast.Mult()",
        full="ast.Mult()"
)

check_node(
        ast.Name(id='id', ctx=ast.Load()),
        empty="ast.Name(id='id', ctx=ast.Load())",
        full="ast.Name(id='id', ctx=ast.Load())"
)

check_node(
        ast.NamedExpr(target=ast.Name(id='target', ctx=ast.Load()), value=ast.Constant(value=None)),
        empty="ast.NamedExpr(target=ast.Name(id='target', ctx=ast.Load()), value=ast.Constant(value=None))",
        full="ast.NamedExpr(target=ast.Name(id='target', ctx=ast.Load()), value=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.Nonlocal(),
        empty="ast.Nonlocal()",
        full="ast.Nonlocal(names=[])"
)

check_node(
        ast.Not(),
        empty="ast.Not()",
        full="ast.Not()"
)

check_node(
        ast.NotEq(),
        empty="ast.NotEq()",
        full="ast.NotEq()"
)

check_node(
        ast.NotIn(),
        empty="ast.NotIn()",
        full="ast.NotIn()"
)

check_node(
        ast.operator(),
        empty="ast.operator()",
        full="ast.operator()"
)

check_node(
        ast.Or(),
        empty="ast.Or()",
        full="ast.Or()"
)

check_node(
        ast.ParamSpec(name='name'),
        empty="ast.ParamSpec(name='name')",
        full="ast.ParamSpec(name='name', default_value=None)"
)

check_node(
        ast.Pass(),
        empty="ast.Pass()",
        full="ast.Pass()"
)

check_node(
        ast.pattern(),
        empty="ast.pattern()",
        full="ast.pattern()"
)

check_node(
        ast.Pow(),
        empty="ast.Pow()",
        full="ast.Pow()"
)

check_node(
        ast.Raise(),
        empty="ast.Raise()",
        full="ast.Raise(exc=None, cause=None)"
)

check_node(
        ast.Return(),
        empty="ast.Return()",
        full="ast.Return(value=None)"
)

check_node(
        ast.RShift(),
        empty="ast.RShift()",
        full="ast.RShift()"
)

check_node(
        ast.Set(),
        empty="ast.Set()",
        full="ast.Set(elts=[])"
)

check_node(
        ast.SetComp(elt=ast.Constant(value=None)),
        empty="ast.SetComp(elt=ast.Constant(value=None))",
        full="ast.SetComp(elt=ast.Constant(value=None, kind=None), generators=[])"
)

check_node(
        ast.Slice(),
        empty="ast.Slice()",
        full="ast.Slice(lower=None, upper=None, step=None)"
)

check_node(
        ast.Starred(value=ast.Name(id='value', ctx=ast.Load()), ctx=ast.Load()),
        empty="ast.Starred(value=ast.Name(id='value', ctx=ast.Load()), ctx=ast.Load())",
        full="ast.Starred(value=ast.Name(id='value', ctx=ast.Load()), ctx=ast.Load())"
)

check_node(
        ast.stmt(),
        empty="ast.stmt()",
        full="ast.stmt()"
)

check_node(
        ast.Store(),
        empty="ast.Store()",
        full="ast.Store()"
)

check_node(
        ast.Sub(),
        empty="ast.Sub()",
        full="ast.Sub()"
)

check_node(
        ast.Subscript(value=ast.Name(id='value', ctx=ast.Load()), slice=ast.Constant(value=None), ctx=ast.Load()),
        empty="ast.Subscript(value=ast.Name(id='value', ctx=ast.Load()), slice=ast.Constant(value=None), ctx=ast.Load())",
        full="ast.Subscript(value=ast.Name(id='value', ctx=ast.Load()), slice=ast.Constant(value=None, kind=None), ctx=ast.Load())"
)

check_node(
        ast.Try(),
        empty="ast.Try()",
        full="ast.Try(body=[], handlers=[], orelse=[], finalbody=[])"
)

check_node(
        ast.TryStar(),
        empty="ast.TryStar()",
        full="ast.TryStar(body=[], handlers=[], orelse=[], finalbody=[])"
)

check_node(
        ast.Tuple(ctx=ast.Load()),
        empty="ast.Tuple(ctx=ast.Load())",
        full="ast.Tuple(elts=[], ctx=ast.Load())"
)

check_node(
        ast.type_ignore(),
        empty="ast.type_ignore()",
        full="ast.type_ignore()"
)

check_node(
        ast.type_param(),
        empty="ast.type_param()",
        full="ast.type_param()"
)

check_node(
        ast.TypeAlias(name=ast.Name(id='name', ctx=ast.Load()), value=ast.Name(id='value', ctx=ast.Load())),
        empty="ast.TypeAlias(name=ast.Name(id='name', ctx=ast.Load()), value=ast.Name(id='value', ctx=ast.Load()))",
        full="ast.TypeAlias(name=ast.Name(id='name', ctx=ast.Load()), type_params=[], value=ast.Name(id='value', ctx=ast.Load()))"
)

check_node(
        ast.TypeIgnore(lineno=1, tag=''),
        empty="ast.TypeIgnore(lineno=1, tag='')",
        full="ast.TypeIgnore(lineno=1, tag='')"
)

check_node(
        ast.TypeVar(name='name'),
        empty="ast.TypeVar(name='name')",
        full="ast.TypeVar(name='name', bound=None, default_value=None)"
)

check_node(
        ast.TypeVarTuple(name='name'),
        empty="ast.TypeVarTuple(name='name')",
        full="ast.TypeVarTuple(name='name', default_value=None)"
)

check_node(
        ast.UAdd(),
        empty="ast.UAdd()",
        full="ast.UAdd()"
)

check_node(
        ast.unaryop(),
        empty="ast.unaryop()",
        full="ast.unaryop()"
)

check_node(
        ast.UnaryOp(op=ast.Not(), operand=ast.Constant(value=None)),
        empty="ast.UnaryOp(op=ast.Not(), operand=ast.Constant(value=None))",
        full="ast.UnaryOp(op=ast.Not(), operand=ast.Constant(value=None, kind=None))"
)

check_node(
        ast.USub(),
        empty="ast.USub()",
        full="ast.USub()"
)

check_node(
        ast.While(test=ast.Constant(value=None)),
        empty="ast.While(test=ast.Constant(value=None))",
        full="ast.While(test=ast.Constant(value=None, kind=None), body=[], orelse=[])"
)

check_node(
        ast.With(),
        empty="ast.With()",
        full="ast.With(items=[], body=[], type_comment=None)"
)

check_node(
        ast.withitem(context_expr=ast.Name(id='name', ctx=ast.Load())),
        empty="ast.withitem(context_expr=ast.Name(id='name', ctx=ast.Load()))",
        full="ast.withitem(context_expr=ast.Name(id='name', ctx=ast.Load()), optional_vars=None)"
)

check_node(
        ast.Yield(),
        empty="ast.Yield()",
        full="ast.Yield(value=None)"
)

check_node(
        ast.YieldFrom(value=ast.Constant(value=None)),
        empty="ast.YieldFrom(value=ast.Constant(value=None))",
        full="ast.YieldFrom(value=ast.Constant(value=None, kind=None))"
)
