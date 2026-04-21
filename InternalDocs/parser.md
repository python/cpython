Guide to the parser
===================

Abstract
--------

Python's Parser is currently a
[`PEG` (Parser Expression Grammar)](https://en.wikipedia.org/wiki/Parsing_expression_grammar)
parser. It was introduced in
[PEP 617: New PEG parser for CPython](https://peps.python.org/pep-0617/) to replace
the original [`LL(1)`](https://en.wikipedia.org/wiki/LL_parser) parser.

The code implementing the parser is generated from a grammar definition by a
[parser generator](https://en.wikipedia.org/wiki/Compiler-compiler).
Therefore, changes to the Python language are made by modifying the
[grammar file](../Grammar/python.gram).
Developers rarely need to modify the generator itself.

See [Changing CPython's grammar](changing_grammar.md)
for a detailed description of the grammar and the process for changing it.

How PEG parsers work
====================

A PEG (Parsing Expression Grammar) grammar differs from a
[context-free grammar](https://en.wikipedia.org/wiki/Context-free_grammar)
in that the way it is written more closely reflects how the parser will operate
when parsing. The fundamental technical difference is that the choice operator
is ordered. This means that when writing:

```
    rule: A | B | C
```

a parser that implements a context-free-grammar (such as an `LL(1)` parser) will
generate constructions that, given an input string, *deduce* which alternative
(`A`, `B` or `C`) must be expanded. On the other hand, a PEG parser will
check each alternative, in the order in which they are specified, and select
that first one that succeeds.

This means that in a PEG grammar, the choice operator is not commutative.
Furthermore, unlike context-free grammars, the derivation according to a
PEG grammar cannot be ambiguous: if a string parses, it has exactly one
valid parse tree.

PEG parsers are usually constructed as a recursive descent parser in which every
rule in the grammar corresponds to a function in the program implementing the
parser, and the parsing expression (the "expansion" or "definition" of the rule)
represents the "code" in said function. Each parsing function conceptually takes
an input string as its argument, and yields one of the following results:

* A "success" result. This result indicates that the expression can be parsed by
  that rule and the function may optionally move forward or consume one or more
  characters of the input string supplied to it.
* A "failure" result, in which case no input is consumed.

Note that "failure" results do not imply that the program is incorrect, nor do
they necessarily mean that the parsing has failed. Since the choice operator is
ordered, a failure very often merely indicates "try the following option". A
direct implementation of a PEG parser as a recursive descent parser will present
exponential time performance in the worst case, because PEG parsers have
infinite lookahead (this means that they can consider an arbitrary number of
tokens before deciding for a rule).  Usually, PEG parsers avoid this exponential
time complexity with a technique called
["packrat parsing"](https://pdos.csail.mit.edu/~baford/packrat/thesis/)
which not only loads the entire program in memory before parsing it but also
allows the parser to backtrack arbitrarily. This is made efficient by memoizing
the rules already matched for each position. The cost of the memoization cache
is that the parser will naturally use more memory than a simple `LL(1)` parser,
which normally are table-based.


Key ideas
---------

- Alternatives are ordered ( `A | B` is not the same as `B | A` ).
- If a rule returns a failure, it doesn't mean that the parsing has failed,
  it just means "try something else".
- By default PEG parsers run in exponential time, which can be optimized to linear by
  using memoization.
- If parsing fails completely (no rule succeeds in parsing all the input text), the
  PEG parser doesn't have a concept of "where the
  [`SyntaxError`](https://docs.python.org/3/library/exceptions.html#SyntaxError) is".


> [!IMPORTANT]
> Don't try to reason about a PEG grammar in the same way you would to with an
> [EBNF](https://en.wikipedia.org/wiki/Extended_Backus–Naur_form)
> or context free grammar. PEG is optimized to describe **how** input strings will
> be parsed, while context-free grammars are optimized to generate strings of the
> language they describe (in EBNF, to know whether a given string is in the
> language, you need to do work to find out as it is not immediately obvious from
> the grammar).


Consequences of the ordered choice operator
-------------------------------------------

Although PEG may look like EBNF, its meaning is quite different. The fact
that the alternatives are ordered in a PEG grammar (which is at the core of
how PEG parsers work) has deep consequences, other than removing ambiguity.

If a rule has two alternatives and the first of them succeeds, the second one is
**not** attempted even if the caller rule fails to parse the rest of the input.
Thus the parser is said to be "eager". To illustrate this, consider
the following two rules (in these examples, a token is an individual character):

```
    first_rule:  ( 'a' | 'aa' ) 'a'
    second_rule: ('aa' | 'a'  ) 'a'
```

In a regular EBNF grammar, both rules specify the language `{aa, aaa}` but
in PEG, one of these two rules accepts the string `aaa` but not the string
`aa`. The other does the opposite -- it accepts the string `aa`
but not the string `aaa`. The rule `('a'|'aa')'a'` does
not accept `aaa` because `'a'|'aa'` consumes the first `a`, letting the
final `a` in the rule consume the second, and leaving out the third `a`.
As the rule has succeeded, no attempt is ever made to go back and let
`'a'|'aa'` try the second alternative. The expression `('aa'|'a')'a'` does
not accept `aa` because `'aa'|'a'` accepts all of `aa`, leaving nothing
for the final `a`. Again, the second alternative of `'aa'|'a'` is not
tried.

> [!CAUTION]
> The effects of ordered choice, such as the ones illustrated above, may be
> hidden by many levels of rules.

For this reason, writing rules where an alternative is contained in the next
one is in almost all cases a mistake, for example:

```
    my_rule:
      | 'if' expression 'then' block
      | 'if' expression 'then' block 'else' block
```

In this example, the second alternative will never be tried because the first one will
succeed first (even if the input string has an `'else' block` that follows). To correctly
write this rule you can simply alter the order:

```
    my_rule:
      | 'if' expression 'then' block 'else' block
      | 'if' expression 'then' block
```

In this case, if the input string doesn't have an `'else' block`, the first alternative
will fail and the second will be attempted.

Grammar Syntax
==============

The grammar consists of a sequence of rules of the form:

```
    rule_name: expression
```

Optionally, a type can be included right after the rule name, which
specifies the return type of the C or Python function corresponding to
the rule:

```
    rule_name[return_type]: expression
```

If the return type is omitted, then a `void *` is returned in C and an
`Any` in Python.

Grammar expressions
-------------------

| Expression      | Description and Example                                                                                               |
|-----------------|-----------------------------------------------------------------------------------------------------------------------|
| `# comment`     | Python-style comments.                                                                                                |
| `e1 e2`         | Match `e1`, then match `e2`. <br> `rule_name: first_rule second_rule`                                                 |
| `e1 \| e2`      | Match `e1` or `e2`. <br> `rule_name[return_type]:`<br>`    \| first_alt`<br>`    \| second_alt`                       |
| `( e )`         | Grouping operator: Match `e`. <br> `rule_name: (e)`<br>`rule_name: (e1 e2)*`                                          |
| `[ e ]` or `e?` | Optionally match `e`. <br> `rule_name: [e]`<br>`rule_name: e (',' e)* [',']`                                          |
| `e*`            | Match zero or more occurrences of `e`. <br> `rule_name: (e1 e2)*`                                                     |
| `e+`            | Match one or more occurrences of `e`. <br> `rule_name: (e1 e2)+`                                                      |
| `s.e+`          | Match one or more occurrences of `e`, separated by `s`. <br> `rule_name: ','.e+`                                      |
| `&e`            | Positive lookahead: Succeed if `e` can be parsed, without consuming input.                                            |
| `!e`            | Negative lookahead: Fail if `e` can be parsed, without consuming input. <br> `primary: atom !'.' !'(' !'['`           |
| `~`             | Commit to the current alternative, even if it fails to parse (cut). <br> `rule_name: '(' ~ some_rule ')' \| some_alt` |


Left recursion
--------------

PEG parsers normally do not support left recursion, but CPython's parser
generator implements a technique similar to the one described in
[Medeiros et al.](https://arxiv.org/pdf/1207.0443) but using the memoization
cache instead of static variables. This approach is closer to the one described
in [Warth et al.](http://web.cs.ucla.edu/~todd/research/pepm08.pdf). This
allows us to write not only simple left-recursive rules but also more
complicated rules that involve indirect left-recursion like:

```
    rule1: rule2 | 'a'
    rule2: rule3 | 'b'
    rule3: rule1 | 'c'
```

and "hidden left-recursion" like:

```
    rule: 'optional'? rule '@' some_other_rule
```

Variables in the grammar
------------------------

A sub-expression can be named by preceding it with an identifier and an
`=` sign. The name can then be used in the action (see below), like this:

```
    rule_name[return_type]: '(' a=some_other_rule ')' { a }
```

Grammar actions
---------------

To avoid the intermediate steps that obscure the relationship between the
grammar and the AST generation, the PEG parser allows directly generating AST
nodes for a rule via grammar actions. Grammar actions are language-specific
expressions that are evaluated when a grammar rule is successfully parsed. These
expressions can be written in Python or C depending on the desired output of the
parser generator. This means that if one would want to generate a parser in
Python and another in C, two grammar files should be written, each one with a
different set of actions, keeping everything else apart from said actions
identical in both files. As an example of a grammar with Python actions, the
piece of the parser generator that parses grammar files is bootstrapped from a
meta-grammar file with Python actions that generate the grammar tree as a result
of the parsing.

In the specific case of the PEG grammar for Python, having actions allows
directly describing how the AST is composed in the grammar itself, making it
more clear and maintainable. This AST generation process is supported by the use
of some helper functions that factor out common AST object manipulations and
some other required operations that are not directly related to the grammar.

To indicate these actions each alternative can be followed by the action code
inside curly-braces, which specifies the return value of the alternative:

```
    rule_name[return_type]:
       | first_alt1 first_alt2 { first_alt1 }
       | second_alt1 second_alt2 { second_alt1 }
```

If the action is omitted, a default action is generated:

- If there is a single name in the rule, it gets returned.
- If there are multiple names in the rule, a collection with all parsed
  expressions gets returned (the type of the collection will be different
  in C and Python).

This default behaviour is primarily made for very simple situations and for
debugging purposes.

> [!WARNING]
> It's important that the actions don't mutate any AST nodes that are passed
> into them via variables referring to other rules. The reason for mutation
> being not allowed is that the AST nodes are cached by memoization and could
> potentially be reused in a different context, where the mutation would be
> invalid. If an action needs to change an AST node, it should instead make a
> new copy of the node and change that.

The full meta-grammar for the grammars supported by the PEG generator is:

```
    start[Grammar]: grammar ENDMARKER { grammar }

    grammar[Grammar]:
        | metas rules { Grammar(rules, metas) }
        | rules { Grammar(rules, []) }

    metas[MetaList]:
        | meta metas { [meta] + metas }
        | meta { [meta] }

    meta[MetaTuple]:
        | "@" NAME NEWLINE { (name.string, None) }
        | "@" a=NAME b=NAME NEWLINE { (a.string, b.string) }
        | "@" NAME STRING NEWLINE { (name.string, literal_eval(string.string)) }

    rules[RuleList]:
        | rule rules { [rule] + rules }
        | rule { [rule] }

    rule[Rule]:
        | rulename ":" alts NEWLINE INDENT more_alts DEDENT {
                Rule(rulename[0], rulename[1], Rhs(alts.alts + more_alts.alts)) }
        | rulename ":" NEWLINE INDENT more_alts DEDENT { Rule(rulename[0], rulename[1], more_alts) }
        | rulename ":" alts NEWLINE { Rule(rulename[0], rulename[1], alts) }

    rulename[RuleName]:
        | NAME '[' type=NAME '*' ']' {(name.string, type.string+"*")}
        | NAME '[' type=NAME ']' {(name.string, type.string)}
        | NAME {(name.string, None)}

    alts[Rhs]:
        | alt "|" alts { Rhs([alt] + alts.alts)}
        | alt { Rhs([alt]) }

    more_alts[Rhs]:
        | "|" alts NEWLINE more_alts { Rhs(alts.alts + more_alts.alts) }
        | "|" alts NEWLINE { Rhs(alts.alts) }

    alt[Alt]:
        | items '$' action { Alt(items + [NamedItem(None, NameLeaf('ENDMARKER'))], action=action) }
        | items '$' { Alt(items + [NamedItem(None, NameLeaf('ENDMARKER'))], action=None) }
        | items action { Alt(items, action=action) }
        | items { Alt(items, action=None) }

    items[NamedItemList]:
        | named_item items { [named_item] + items }
        | named_item { [named_item] }

    named_item[NamedItem]:
        | NAME '=' ~ item {NamedItem(name.string, item)}
        | item {NamedItem(None, item)}
        | it=lookahead {NamedItem(None, it)}

    lookahead[LookaheadOrCut]:
        | '&' ~ atom {PositiveLookahead(atom)}
        | '!' ~ atom {NegativeLookahead(atom)}
        | '~' {Cut()}

    item[Item]:
        | '[' ~ alts ']' {Opt(alts)}
        |  atom '?' {Opt(atom)}
        |  atom '*' {Repeat0(atom)}
        |  atom '+' {Repeat1(atom)}
        |  sep=atom '.' node=atom '+' {Gather(sep, node)}
        |  atom {atom}

    atom[Plain]:
        | '(' ~ alts ')' {Group(alts)}
        | NAME {NameLeaf(name.string) }
        | STRING {StringLeaf(string.string)}

    # Mini-grammar for the actions

    action[str]: "{" ~ target_atoms "}" { target_atoms }

    target_atoms[str]:
        | target_atom target_atoms { target_atom + " " + target_atoms }
        | target_atom { target_atom }

    target_atom[str]:
        | "{" ~ target_atoms "}" { "{" + target_atoms + "}" }
        | NAME { name.string }
        | NUMBER { number.string }
        | STRING { string.string }
        | "?" { "?" }
        | ":" { ":" }
```

As an illustrative example this simple grammar file allows directly
generating a full parser that can parse simple arithmetic expressions and that
returns a valid C-based Python AST:

```
    start[mod_ty]: a=expr_stmt* ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
    expr_stmt[stmt_ty]: a=expr NEWLINE { _PyAST_Expr(a, EXTRA) }

    expr[expr_ty]:
        | l=expr '+' r=term { _PyAST_BinOp(l, Add, r, EXTRA) }
        | l=expr '-' r=term { _PyAST_BinOp(l, Sub, r, EXTRA) }
        | term

    term[expr_ty]:
        | l=term '*' r=factor { _PyAST_BinOp(l, Mult, r, EXTRA) }
        | l=term '/' r=factor { _PyAST_BinOp(l, Div, r, EXTRA) }
        | factor

    factor[expr_ty]:
        | '(' e=expr ')' { e }
        | atom

    atom[expr_ty]:
        | NAME
        | NUMBER
```

Here `EXTRA` is a macro that expands to `start_lineno, start_col_offset,
end_lineno, end_col_offset, p->arena`, those being variables automatically
injected by the parser; `p` points to an object that holds on to all state
for the parser.

A similar grammar written to target Python AST objects:

```
    start[ast.Module]: a=expr_stmt* ENDMARKER { ast.Module(body=a or [] }
    expr_stmt: a=expr NEWLINE { ast.Expr(value=a, EXTRA) }

    expr:
        | l=expr '+' r=term { ast.BinOp(left=l, op=ast.Add(), right=r, EXTRA) }
        | l=expr '-' r=term { ast.BinOp(left=l, op=ast.Sub(), right=r, EXTRA) }
        | term

    term:
        | l=term '*' r=factor { ast.BinOp(left=l, op=ast.Mult(), right=r, EXTRA) }
        | l=term '/' r=factor { ast.BinOp(left=l, op=ast.Div(), right=r, EXTRA) }
        | factor

    factor:
        | '(' e=expr ')' { e }
        | atom

    atom:
        | NAME
        | NUMBER
```

Pegen
=====

Pegen is the parser generator used in CPython to produce the final PEG parser
used by the interpreter. It is the program that can be used to read the python
grammar located in [`Grammar/python.gram`](../Grammar/python.gram) and produce
the final C parser. It contains the following pieces:

- A parser generator that can read a grammar file and produce a PEG parser
  written in Python or C that can parse said grammar. The generator is located at
  [`Tools/peg_generator/pegen`](../Tools/peg_generator/pegen).
- A PEG meta-grammar that automatically generates a Python parser which is used
  for the parser generator itself (this means that there are no manually-written
  parsers). The meta-grammar is located at
  [`Tools/peg_generator/pegen/metagrammar.gram`](../Tools/peg_generator/pegen/metagrammar.gram).
- A generated parser (using the parser generator) that can directly produce C and Python AST objects.

The source code for Pegen lives at [`Tools/peg_generator/pegen`](../Tools/peg_generator/pegen)
but normally all typical commands to interact with the parser generator are executed from
the main makefile.

How to regenerate the parser
----------------------------

Once you have made the changes to the grammar files, to regenerate the `C`
parser (the one used by the interpreter) just execute:

```shell
$ make regen-pegen
```

using the `Makefile` in the main directory. If you are on Windows you can
use the Visual Studio project files to regenerate the parser or to execute:

```dos
PCbuild/build.bat --regen
```

The generated parser file is located at [`Parser/parser.c`](../Parser/parser.c).

How to regenerate the meta-parser
---------------------------------

The meta-grammar (the grammar that describes the grammar for the grammar files
themselves) is located at
[`Tools/peg_generator/pegen/metagrammar.gram`](../Tools/peg_generator/pegen/metagrammar.gram).
Although it is very unlikely that you will ever need to modify it, if you make
any modifications to this file (in order to implement new Pegen features) you will
need to regenerate the meta-parser (the parser that parses the grammar files).
To do so just execute:

```shell
$ make regen-pegen-metaparser
```

If you are on Windows you can use the Visual Studio project files
to regenerate the parser or to execute:

```dos
PCbuild/build.bat --regen
```


Grammatical elements and rules
------------------------------

Pegen has some special grammatical elements and rules:

- Strings with single quotes (') (for example, `'class'`) denote KEYWORDS.
- Strings with double quotes (") (for example, `"match"`) denote SOFT KEYWORDS.
- Uppercase names (for example, `NAME`) denote tokens in the
  [`Grammar/Tokens`](../Grammar/Tokens) file.
- Rule names starting with `invalid_` are used for specialized syntax errors.

  - These rules are NOT used in the first pass of the parser.
  - Only if the first pass fails to parse, a second pass including the invalid
    rules will be executed.
  - If the parser fails in the second phase with a generic syntax error, the
    location of the generic failure of the first pass will be used (this avoids
    reporting incorrect locations due to the invalid rules).
  - The order of the alternatives involving invalid rules matter
    (like any rule in PEG).

Tokenization
------------

It is common among PEG parser frameworks that the parser does both the parsing
and the tokenization, but this does not happen in Pegen. The reason is that the
Python language needs a custom tokenizer to handle things like indentation
boundaries, some special keywords like `ASYNC` and `AWAIT` (for
compatibility purposes), backtracking errors (such as unclosed parenthesis),
dealing with encoding, interactive mode and much more. Some of these reasons
are also there for historical purposes, and some others are useful even today.

The list of tokens (all uppercase names in the grammar) that you can use can
be found in the [`Grammar/Tokens`](../Grammar/Tokens)
file. If you change this file to add new tokens, make sure to regenerate the
files by executing:

```shell
$ make regen-token
```

If you are on Windows you can use the Visual Studio project files to regenerate
the tokens or to execute:

```dos
PCbuild/build.bat --regen
```

How tokens are generated and the rules governing this are completely up to the tokenizer
([`Parser/lexer`](../Parser/lexer) and [`Parser/tokenizer`](../Parser/tokenizer));
the parser just receives tokens from it.

Memoization
-----------

As described previously, to avoid exponential time complexity in the parser,
memoization is used.

The C parser used by Python is highly optimized and memoization can be expensive
both in memory and time. Although the memory cost is obvious (the parser needs
memory for storing previous results in the cache) the execution time cost comes
from continuously checking if the given rule has a cache hit or not. In many
situations, just parsing it again can be faster. Pegen **disables memoization
by default** except for rules with the special marker `memo` after the rule
name (and type, if present):

```
rule_name[typr] (memo):
  ...
```

By selectively turning on memoization for a handful of rules, the parser becomes
faster and uses less memory.

> [!NOTE]
> Left-recursive rules always use memoization, since the implementation of
> left-recursion depends on it.

To determine whether a new rule needs memoization or not, benchmarking is required
(comparing execution times and memory usage of some considerably large files with
and without memoization). There is a very simple instrumentation API available
in the generated C parse code that allows to measure how much each rule uses
memoization (check the [`Parser/pegen.c`](../Parser/pegen.c)
file for more information) but it needs to be manually activated.

Automatic variables
-------------------

To make writing actions easier, Pegen injects some automatic variables in the
namespace available when writing actions. In the C parser, some of these
automatic variable names are:

- `p`: The parser structure.
- `EXTRA`: This is a macro that expands to
  `(_start_lineno, _start_col_offset, _end_lineno, _end_col_offset, p->arena)`,
  which is normally used to create AST nodes as almost all constructors need these
  attributes to be provided. All of the location variables are taken from the
  location information of the current token.

Hard and soft keywords
----------------------

> [!NOTE]
> In the grammar files, keywords are defined using **single quotes** (for example,
> `'class'`) while soft keywords are defined using **double quotes** (for example,
> `"match"`).

There are two kinds of keywords allowed in pegen grammars: *hard* and *soft*
keywords. The difference between hard and soft keywords is that hard keywords
are always reserved words, even in positions where they make no sense
(for example, `x = class + 1`), while soft keywords only get a special
meaning in context. Trying to use a hard keyword as a variable will always
fail:

```pycon
>>> class = 3
File "<stdin>", line 1
    class = 3
        ^
SyntaxError: invalid syntax
>>> foo(class=3)
File "<stdin>", line 1
    foo(class=3)
        ^^^^^
SyntaxError: invalid syntax
```

While soft keywords don't have this limitation if used in a context other than
one where they are defined as keywords:

```pycon
>>> match = 45
>>> foo(match="Yeah!")
```

The `match` and `case` keywords are soft keywords, so that they are
recognized as keywords at the beginning of a match statement or case block
respectively, but are allowed to be used in other places as variable or
argument names.

You can get a list of all keywords defined in the grammar from Python:

```pycon
>>> import keyword
>>> keyword.kwlist
['False', 'None', 'True', 'and', 'as', 'assert', 'async', 'await', 'break',
'class', 'continue', 'def', 'del', 'elif', 'else', 'except', 'finally', 'for',
'from', 'global', 'if', 'import', 'in', 'is', 'lambda', 'nonlocal', 'not', 'or',
'pass', 'raise', 'return', 'try', 'while', 'with', 'yield']
```

as well as soft keywords:

```pycon
>>> import keyword
>>> keyword.softkwlist
['_', 'case', 'match']
```

> [!CAUTION]
> Soft keywords can be a bit challenging to manage as they can be accepted in
> places you don't intend, given how the order alternatives behave in PEG
> parsers (see the
> [consequences of ordered choice](#consequences-of-the-ordered-choice-operator)
> section for some background on this). In general, try to define them in places
> where there are not many alternatives.

Error handling
--------------

When a pegen-generated parser detects that an exception is raised, it will
**automatically stop parsing**, no matter what the current state of the parser
is, and it will unwind the stack and report the exception. This means that if a
[rule action](#grammar-actions) raises an exception, all parsing will
stop at that exact point. This is done to allow to correctly propagate any
exception set by calling Python's C API functions. This also includes
[`SyntaxError`](https://docs.python.org/3/library/exceptions.html#SyntaxError)
exceptions and it is the main mechanism the parser uses to report custom syntax
error messages.

> [!NOTE]
> Tokenizer errors are normally reported by raising exceptions but some special
> tokenizer errors such as unclosed parenthesis will be reported only after the
> parser finishes without returning anything.

How syntax errors are reported
------------------------------

As described previously in the [how PEG parsers work](#how-peg-parsers-work)
section, PEG parsers don't have a defined concept of where errors happened
in the grammar, because a rule failure doesn't imply a parsing failure like
in context free grammars. This means that a heuristic has to be used to report
generic errors unless something is explicitly declared as an error in the
grammar.

To report generic syntax errors, pegen uses a common heuristic in PEG parsers:
the location of *generic* syntax errors is reported to be the furthest token that
was attempted to be matched but failed. This is only done if parsing has failed
(the parser returns `NULL` in C or `None` in Python) but no exception has
been raised.

As the Python grammar was primordially written as an `LL(1)` grammar, this heuristic
has an extremely high success rate, but some PEG features, such as lookaheads,
can impact this.

> [!CAUTION]
> Positive and negative lookaheads will try to match a token so they will affect
> the location of generic syntax errors. Use them carefully at boundaries
> between rules.

To generate more precise syntax errors, custom rules are used. This is a common
practice also in context free grammars: the parser will try to accept some
construct that is known to be incorrect just to report a specific syntax error
for that construct. In pegen grammars, these rules start with the `invalid_`
prefix. This is because trying to match these rules normally has a performance
impact on parsing (and can also affect the 'correct' grammar itself in some
tricky cases, depending on the ordering of the rules) so the generated parser
acts in two phases:

1. The first phase will try to parse the input stream without taking into
   account rules that start with the `invalid_` prefix. If the parsing
   succeeds it will return the generated AST and the second phase will be
   skipped.

2. If the first phase failed, a second parsing attempt is done including the
   rules that start with an `invalid_` prefix. By design this attempt
   **cannot succeed** and is only executed to give to the invalid rules a
   chance to detect specific situations where custom, more precise, syntax
   errors can be raised. This also allows to trade a bit of performance for
   precision reporting errors: given that we know that the input text is
   invalid, there is typically no need to be fast because execution is going
   to stop anyway.

> [!IMPORTANT]
> When defining invalid rules:
>
> - Make sure all custom invalid rules raise
>   [`SyntaxError`](https://docs.python.org/3/library/exceptions.html#SyntaxError)
>   exceptions (or a subclass of it).
> - Make sure **all** invalid rules start with the `invalid_` prefix to not
>   impact performance of parsing correct Python code.
> - Make sure the parser doesn't behave differently for regular rules when you introduce invalid rules
>   (see the [how PEG parsers work](#how-peg-parsers-work) section for more information).

You can find a collection of macros to raise specialized syntax errors in the
[`Parser/pegen.h`](../Parser/pegen.h)
header file. These macros allow also to report ranges for
the custom errors, which will be highlighted in the tracebacks that will be
displayed when the error is reported.


> [!TIP]
> A good way to test whether an invalid rule will be triggered when you expect
> is to test if introducing a syntax error **after** valid code triggers the
> rule or not. For example:

```
<valid python code> $ 42
```

should trigger the syntax error in the `$` character. If your rule is not correctly defined this
won't happen. As another example, suppose that you try to define a rule to match Python 2 style
`print` statements in order to create a better error message and you define it as:

```
invalid_print: "print" expression
```

This will **seem** to work because the parser will correctly parse `print(something)` because it is valid
code and the second phase will never execute but if you try to parse `print(something) $ 3` the first pass
of the parser will fail (because of the `$`) and in the second phase, the rule will match the
`print(something)` as `print` followed by the variable `something` between parentheses and the error
will be reported there instead of the `$` character.

Generating AST objects
----------------------

The output of the C parser used by CPython, which is generated from the
[grammar file](../Grammar/python.gram), is a Python AST object (using C
structures). This means that the actions in the grammar file generate AST
objects when they succeed. Constructing these objects can be quite cumbersome
(see the [AST compiler section](compiler.md#abstract-syntax-trees-ast)
for more information on how these objects are constructed and how they are used
by the compiler), so special helper functions are used. These functions are
declared in the [`Parser/pegen.h`](../Parser/pegen.h) header file and defined
in the [`Parser/action_helpers.c`](../Parser/action_helpers.c) file. The
helpers include functions that join AST sequences, get specific elements
from them or to perform extra processing on the generated tree.


> [!CAUTION]
> Actions must **never** be used to accept or reject rules. It may be tempting
> in some situations to write a very generic rule and then check the generated
> AST to decide whether it is valid or not, but this will render the
> (official grammar)[https://docs.python.org/3/reference/grammar.html] partially
> incorrect (because it does not include actions) and will make it more difficult
> for other Python implementations to adapt the grammar to their own needs.

As a general rule, if an action spawns multiple lines or requires something more
complicated than a single expression of C code, is normally better to create a
custom helper in [`Parser/action_helpers.c`](../Parser/action_helpers.c)
and expose it in the [`Parser/pegen.h`](../Parser/pegen.h) header file so that
it can be used from the grammar.

When parsing succeeds, the parser **must** return a **valid** AST object.

Testing
=======

There are three files that contain tests for the grammar and the parser:

- [test_grammar.py](../Lib/test/test_grammar.py)
- [test_syntax.py](../Lib/test/test_syntax.py)
- [test_exceptions.py](../Lib/test/test_exceptions.py)

Check the contents of these files to know which is the best place for new
tests, depending on the nature of the new feature you are adding.

Tests for the parser generator itself can be found in the
[test_peg_generator](../Lib/test/test_peg_generator) directory.


Debugging generated parsers
===========================

Making experiments
------------------

As the generated C parser is the one used by Python, this means that if
something goes wrong when adding some new rules to the grammar, you cannot
correctly compile and execute Python anymore. This makes it a bit challenging
to debug when something goes wrong, especially when experimenting.

For this reason it is a good idea to experiment first by generating a Python
parser. To do this, you can go to the [Tools/peg_generator](../Tools/peg_generator)
directory on the CPython repository and manually call the parser generator by executing:

```shell
$ python -m pegen python <PATH TO YOUR GRAMMAR FILE>
```

This will generate a file called `parse.py` in the same directory that you
can use to parse some input:

```shell
$ python parse.py file_with_source_code_to_test.py
```

As the generated `parse.py` file is just Python code, you can modify it
and add breakpoints to debug or better understand some complex situations.


Verbose mode
------------

When Python is compiled in debug mode (by adding `--with-pydebug` when
running the configure step in Linux or by adding `-d` when calling the
[PCbuild/build.bat](../PCbuild/build.bat)), it is possible to activate a
**very** verbose mode in the generated parser. This is very useful to
debug the generated parser and to understand how it works, but it
can be a bit hard to understand at first.

> [!NOTE]
> When activating verbose mode in the Python parser, it is better to not use
> interactive mode as it can be much harder to understand, because interactive
> mode involves some special steps compared to regular parsing.

To activate verbose mode you can add the `-d` flag when executing Python:

```shell
$ python -d file_to_test.py
```

This will print **a lot** of output to `stderr` so it is probably better to dump
it to a file for further analysis. The output consists of trace lines with the
following structure::

```
<indentation> ('>'|'-'|'+'|'!') <rule_name>[<token_location>]: <alternative> ...
```

Every line is indented by a different amount (`<indentation>`) depending on how
deep the call stack is. The next character marks the type of the trace:

- `>` indicates that a rule is going to be attempted to be parsed.
- `-` indicates that a rule has failed to be parsed.
- `+` indicates that a rule has been parsed correctly.
- `!` indicates that an exception or an error has been detected and the parser is unwinding.

The `<token_location>` part indicates the current index in the token array,
the `<rule_name>` part indicates what rule is being parsed and
the `<alternative>` part indicates what alternative within that rule
is being attempted.


> [!NOTE]
> **Document history**
>
>   Pablo Galindo Salgado - Original author
>
>   Irit Katriel and Jacob Coffee - Convert to Markdown
