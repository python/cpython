" Auto-generated Vim syntax file for Python
"
" To use: copy or symlink to ~/.vim/syntax/python.vim


if exists("b:current_syntax")
  finish
endif

if exists("python_highlight_all")
  let python_highlight_numbers = 1
  let python_highlight_builtins = 1
  let python_highlight_exceptions = 1
  let python_highlight_space_errors = 1
endif

syn keyword pythonStatement    as assert break continue del except exec finally
syn keyword pythonStatement    global lambda pass print raise return try with
syn keyword pythonStatement    yield

syn keyword pythonStatement    def class nextgroup=pythonFunction skipwhite

syn match pythonFunction    "[a-zA-Z_][a-zA-Z0-9_]*" contained

syn keyword pythonRepeat    for while

syn keyword pythonConditional    if elif else

syn keyword pythonOperator    and in is not or

syn keyword pythonPreCondit    import from

syn match pythonComment    "#.*$" contains=pythonTodo

syn keyword pythonTodo    TODO FIXME XXX contained

syn region pythonString    matchgroup=Normal start=+[uU]\='+ end=+'+ skip=+\\\\\|\\'+ contains=pythonEscape
syn region pythonString    matchgroup=Normal start=+[uU]\="+ end=+"+ skip=+\\\\\|\\"+ contains=pythonEscape
syn region pythonString    matchgroup=Normal start=+[uU]\="""+ end=+"""+  contains=pythonEscape
syn region pythonString    matchgroup=Normal start=+[uU]\='''+ end=+'''+  contains=pythonEscape
syn region pythonString    matchgroup=Normal start=+[uU]\=[rR]'+ end=+'+ skip=+\\\\\|\\'+ 
syn region pythonString    matchgroup=Normal start=+[uU]\=[rR]"+ end=+"+ skip=+\\\\\|\\"+ 
syn region pythonString    matchgroup=Normal start=+[uU]\=[rR]"""+ end=+"""+  
syn region pythonString    matchgroup=Normal start=+[uU]\=[rR]'''+ end=+'''+  

syn match pythonEscape    +\\[abfnrtv\'"\\]+ contained
syn match pythonEscape    "\\\o\{1,3}" contained
syn match pythonEscape    "\\x\x\{2}" contained
syn match pythonEscape    "\(\\u\x\{4}\|\\U\x\{8}\)" contained

syn match pythonEscape    "\\$"


if exists("python_highlight_numbers")
  syn match pythonNumber    "\<0x\x\+[Ll]\=\>"
  syn match pythonNumber    "\<\d\+[LljJ]\=\>"
  syn match pythonNumber    "\.\d\+\([eE][+-]\=\d\+\)\=[jJ]\=\>"
  syn match pythonNumber    "\<\d\+\.\([eE][+-]\=\d\+\)\=[jJ]\=\>"
  syn match pythonNumber    "\<\d\+\.\d\+\([eE][+-]\=\d\+\)\=[jJ]\=\>"

endif


if exists("python_highlight_builtins")
  syn keyword pythonBuiltin    unichr all set abs vars int __import__ unicode
  syn keyword pythonBuiltin    enumerate reduce exit issubclass
  syn keyword pythonBuiltin    divmod file Ellipsis isinstance open any
  syn keyword pythonBuiltin    locals help filter basestring slice copyright min
  syn keyword pythonBuiltin    super sum tuple hex execfile long id chr
  syn keyword pythonBuiltin    complex bool zip pow dict True oct NotImplemented
  syn keyword pythonBuiltin    map None float hash getattr buffer max reversed
  syn keyword pythonBuiltin    object quit len repr callable credits setattr
  syn keyword pythonBuiltin    eval frozenset sorted ord __debug__ hasattr
  syn keyword pythonBuiltin    delattr False license classmethod type
  syn keyword pythonBuiltin    list iter reload range globals
  syn keyword pythonBuiltin    staticmethod str property round dir cmp

endif


if exists("python_highlight_exceptions")
  syn keyword pythonException    GeneratorExit ImportError RuntimeError
  syn keyword pythonException    UnicodeTranslateError MemoryError StopIteration
  syn keyword pythonException    PendingDeprecationWarning EnvironmentError
  syn keyword pythonException    LookupError OSError DeprecationWarning
  syn keyword pythonException    UnicodeError UnicodeEncodeError
  syn keyword pythonException    FloatingPointError ReferenceError NameError
  syn keyword pythonException    IOError SyntaxError
  syn keyword pythonException    FutureWarning ImportWarning SystemExit
  syn keyword pythonException    Exception EOFError StandardError ValueError
  syn keyword pythonException    TabError KeyError ZeroDivisionError SystemError
  syn keyword pythonException    UnicodeDecodeError IndentationError
  syn keyword pythonException    AssertionError TypeError IndexError
  syn keyword pythonException    RuntimeWarning KeyboardInterrupt UserWarning
  syn keyword pythonException    SyntaxWarning UnboundLocalError ArithmeticError
  syn keyword pythonException    Warning NotImplementedError AttributeError
  syn keyword pythonException    OverflowError BaseException

endif


if exists("python_highlight_space_errors")
  syn match pythonSpaceError    display excludenl "\S\s\+$"ms=s+1
  syn match pythonSpaceError    display " \+\t"
  syn match pythonSpaceError    display "\t\+ "

endif


  hi def link pythonStatement Statement
  hi def link pythonStatement Statement
  hi def link pythonFunction Function
  hi def link pythonRepeat Repeat
  hi def link pythonConditional Conditional
  hi def link pythonOperator Operator
  hi def link pythonPreCondit PreCondit
  hi def link pythonComment Comment
  hi def link pythonTodo Todo
  hi def link pythonString String
  hi def link pythonEscape Special
  hi def link pythonEscape Special

  if exists("python_highlight_numbers")
    hi def link pythonNumber Number
  endif

  if exists("python_highlight_builtins")
    hi def link pythonBuiltin Function
  endif

  if exists("python_highlight_exceptions")
    hi def link pythonException Exception
  endif

  if exists("python_highlight_space_errors")
    hi def link pythonSpaceError Error
  endif


" Uncomment the 'minlines' statement line and comment out the 'maxlines'
" statement line; changes behaviour to look at least 2000 lines previously for
" syntax matches instead of at most 200 lines
syn sync match pythonSync grouphere NONE "):$"
syn sync maxlines=200
"syn sync minlines=2000

let b:current_syntax = "python"
