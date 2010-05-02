" Auto-generated Vim syntax file for Python (trunk: r80490).
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
  syn keyword pythonBuiltin    Ellipsis False None NotImplemented True __debug__
  syn keyword pythonBuiltin    __import__ abs all any apply basestring bin bool
  syn keyword pythonBuiltin    buffer bytearray bytes callable chr classmethod
  syn keyword pythonBuiltin    cmp coerce compile complex copyright credits
  syn keyword pythonBuiltin    delattr dict dir divmod enumerate eval execfile
  syn keyword pythonBuiltin    exit file filter float format frozenset getattr
  syn keyword pythonBuiltin    globals hasattr hash help hex id input int intern
  syn keyword pythonBuiltin    isinstance issubclass iter len license list
  syn keyword pythonBuiltin    locals long map max memoryview min next object
  syn keyword pythonBuiltin    oct open ord pow print property quit range
  syn keyword pythonBuiltin    raw_input reduce reload repr reversed round set
  syn keyword pythonBuiltin    setattr slice sorted staticmethod str sum super
  syn keyword pythonBuiltin    tuple type unichr unicode vars xrange zip

endif


if exists("python_highlight_exceptions")
  syn keyword pythonException    ArithmeticError AssertionError AttributeError
  syn keyword pythonException    BaseException BufferError BytesWarning
  syn keyword pythonException    DeprecationWarning EOFError EnvironmentError
  syn keyword pythonException    Exception FloatingPointError FutureWarning
  syn keyword pythonException    GeneratorExit IOError ImportError ImportWarning
  syn keyword pythonException    IndentationError IndexError KeyError
  syn keyword pythonException    KeyboardInterrupt LookupError MemoryError
  syn keyword pythonException    NameError NotImplementedError OSError
  syn keyword pythonException    OverflowError PendingDeprecationWarning
  syn keyword pythonException    ReferenceError RuntimeError RuntimeWarning
  syn keyword pythonException    StandardError StopIteration SyntaxError
  syn keyword pythonException    SyntaxWarning SystemError SystemExit TabError
  syn keyword pythonException    TypeError UnboundLocalError UnicodeDecodeError
  syn keyword pythonException    UnicodeEncodeError UnicodeError
  syn keyword pythonException    UnicodeTranslateError UnicodeWarning
  syn keyword pythonException    UserWarning ValueError Warning
  syn keyword pythonException    ZeroDivisionError

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
