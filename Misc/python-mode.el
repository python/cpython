;;; python-mode.el --- Major mode for editing Python programs

;; Copyright (C) 1992,1993,1994  Tim Peters

;; Author: 1995-2002 Barry A. Warsaw
;;         1992-1994 Tim Peters
;; Maintainer: python-mode@python.org
;; Created:    Feb 1992
;; Keywords:   python languages oop

(defconst py-version "$Revision$"
  "`python-mode' version number.")

;; This software is provided as-is, without express or implied
;; warranty.  Permission to use, copy, modify, distribute or sell this
;; software, without fee, for any purpose and by any individual or
;; organization, is hereby granted, provided that the above copyright
;; notice and this paragraph appear in all copies.

;;; Commentary:

;; This is a major mode for editing Python programs.  It was developed
;; by Tim Peters after an original idea by Michael A. Guravage.  Tim
;; subsequently left the net; in 1995, Barry Warsaw inherited the mode
;; and is the current maintainer.  Tim's now back but disavows all
;; responsibility for the mode.  Smart Tim :-)

;; pdbtrack support contributed by Ken Manheimer, April 2001.

;; Please use the SourceForge Python project to submit bugs or
;; patches:
;;
;;     http://sourceforge.net/projects/python

;; FOR MORE INFORMATION:

;; There is some information on python-mode.el at

;;     http://www.python.org/emacs/python-mode/
;;
;; It does contain links to other packages that you might find useful,
;; such as pdb interfaces, OO-Browser links, etc.

;; BUG REPORTING:

;; As mentioned above, please use the SourceForge Python project for
;; submitting bug reports or patches.  The old recommendation, to use
;; C-c C-b will still work, but those reports have a higher chance of
;; getting buried in my mailbox.  Please include a complete, but
;; concise code sample and a recipe for reproducing the bug.  Send
;; suggestions and other comments to python-mode@python.org.

;; When in a Python mode buffer, do a C-h m for more help.  It's
;; doubtful that a texinfo manual would be very useful, but if you
;; want to contribute one, I'll certainly accept it!

;;; Code:

(require 'comint)
(require 'custom)
(require 'cl)
(require 'compile)


;; user definable variables
;; vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

(defgroup python nil
  "Support for the Python programming language, <http://www.python.org/>"
  :group 'languages
  :prefix "py-")

(defcustom py-python-command "python"
  "*Shell command used to start Python interpreter."
  :type 'string
  :group 'python)

(defcustom py-jpython-command "jpython"
  "*Shell command used to start the JPython interpreter."
  :type 'string
  :group 'python
  :tag "JPython Command")

(defcustom py-default-interpreter 'cpython
  "*Which Python interpreter is used by default.
The value for this variable can be either `cpython' or `jpython'.

When the value is `cpython', the variables `py-python-command' and
`py-python-command-args' are consulted to determine the interpreter
and arguments to use.

When the value is `jpython', the variables `py-jpython-command' and
`py-jpython-command-args' are consulted to determine the interpreter
and arguments to use.

Note that this variable is consulted only the first time that a Python
mode buffer is visited during an Emacs session.  After that, use
\\[py-toggle-shells] to change the interpreter shell."
  :type '(choice (const :tag "Python (a.k.a. CPython)" cpython)
		 (const :tag "JPython" jpython))
  :group 'python)

(defcustom py-python-command-args '("-i")
  "*List of string arguments to be used when starting a Python shell."
  :type '(repeat string)
  :group 'python)

(defcustom py-jpython-command-args '("-i")
  "*List of string arguments to be used when starting a JPython shell."
  :type '(repeat string)
  :group 'python
  :tag "JPython Command Args")

(defcustom py-indent-offset 4
  "*Amount of offset per level of indentation.
`\\[py-guess-indent-offset]' can usually guess a good value when
you're editing someone else's Python code."
  :type 'integer
  :group 'python)

(defcustom py-continuation-offset 4
  "*Additional amount of offset to give for some continuation lines.
Continuation lines are those that immediately follow a backslash
terminated line.  Only those continuation lines for a block opening
statement are given this extra offset."
  :type 'integer
  :group 'python)

(defcustom py-smart-indentation t
  "*Should `python-mode' try to automagically set some indentation variables?
When this variable is non-nil, two things happen when a buffer is set
to `python-mode':

    1. `py-indent-offset' is guessed from existing code in the buffer.
       Only guessed values between 2 and 8 are considered.  If a valid
       guess can't be made (perhaps because you are visiting a new
       file), then the value in `py-indent-offset' is used.

    2. `indent-tabs-mode' is turned off if `py-indent-offset' does not
       equal `tab-width' (`indent-tabs-mode' is never turned on by
       Python mode).  This means that for newly written code, tabs are
       only inserted in indentation if one tab is one indentation
       level, otherwise only spaces are used.

Note that both these settings occur *after* `python-mode-hook' is run,
so if you want to defeat the automagic configuration, you must also
set `py-smart-indentation' to nil in your `python-mode-hook'."
  :type 'boolean
  :group 'python)

(defcustom py-align-multiline-strings-p t
  "*Flag describing how multi-line triple quoted strings are aligned.
When this flag is non-nil, continuation lines are lined up under the
preceding line's indentation.  When this flag is nil, continuation
lines are aligned to column zero."
  :type '(choice (const :tag "Align under preceding line" t)
		 (const :tag "Align to column zero" nil))
  :group 'python)

(defcustom py-block-comment-prefix "##"
  "*String used by \\[comment-region] to comment out a block of code.
This should follow the convention for non-indenting comment lines so
that the indentation commands won't get confused (i.e., the string
should be of the form `#x...' where `x' is not a blank or a tab, and
`...' is arbitrary).  However, this string should not end in whitespace."
  :type 'string
  :group 'python)

(defcustom py-honor-comment-indentation t
  "*Controls how comment lines influence subsequent indentation.

When nil, all comment lines are skipped for indentation purposes, and
if possible, a faster algorithm is used (i.e. X/Emacs 19 and beyond).

When t, lines that begin with a single `#' are a hint to subsequent
line indentation.  If the previous line is such a comment line (as
opposed to one that starts with `py-block-comment-prefix'), then its
indentation is used as a hint for this line's indentation.  Lines that
begin with `py-block-comment-prefix' are ignored for indentation
purposes.

When not nil or t, comment lines that begin with a single `#' are used
as indentation hints, unless the comment character is in column zero."
  :type '(choice
	  (const :tag "Skip all comment lines (fast)" nil)
	  (const :tag "Single # `sets' indentation for next line" t)
	  (const :tag "Single # `sets' indentation except at column zero"
		 other)
	  )
  :group 'python)

(defcustom py-temp-directory
  (let ((ok '(lambda (x)
	       (and x
		    (setq x (expand-file-name x)) ; always true
		    (file-directory-p x)
		    (file-writable-p x)
		    x))))
    (or (funcall ok (getenv "TMPDIR"))
	(funcall ok "/usr/tmp")
	(funcall ok "/tmp")
	(funcall ok "/var/tmp")
	(funcall ok  ".")
	(error
	 "Couldn't find a usable temp directory -- set `py-temp-directory'")))
  "*Directory used for temporary files created by a *Python* process.
By default, the first directory from this list that exists and that you
can write into: the value (if any) of the environment variable TMPDIR,
/usr/tmp, /tmp, /var/tmp, or the current directory."
  :type 'string
  :group 'python)

(defcustom py-beep-if-tab-change t
  "*Ring the bell if `tab-width' is changed.
If a comment of the form

  \t# vi:set tabsize=<number>:

is found before the first code line when the file is entered, and the
current value of (the general Emacs variable) `tab-width' does not
equal <number>, `tab-width' is set to <number>, a message saying so is
displayed in the echo area, and if `py-beep-if-tab-change' is non-nil
the Emacs bell is also rung as a warning."
  :type 'boolean
  :group 'python)

(defcustom py-jump-on-exception t
  "*Jump to innermost exception frame in *Python Output* buffer.
When this variable is non-nil and an exception occurs when running
Python code synchronously in a subprocess, jump immediately to the
source code of the innermost traceback frame."
  :type 'boolean
  :group 'python)

(defcustom py-ask-about-save t
  "If not nil, ask about which buffers to save before executing some code.
Otherwise, all modified buffers are saved without asking."
  :type 'boolean
  :group 'python)

(defcustom py-backspace-function 'backward-delete-char-untabify
  "*Function called by `py-electric-backspace' when deleting backwards."
  :type 'function
  :group 'python)

(defcustom py-delete-function 'delete-char
  "*Function called by `py-electric-delete' when deleting forwards."
  :type 'function
  :group 'python)

(defcustom py-imenu-show-method-args-p nil 
  "*Controls echoing of arguments of functions & methods in the Imenu buffer.
When non-nil, arguments are printed."
  :type 'boolean
  :group 'python)
(make-variable-buffer-local 'py-indent-offset)

(defcustom py-pdbtrack-do-tracking-p t
  "*Controls whether the pdbtrack feature is enabled or not.
When non-nil, pdbtrack is enabled in all comint-based buffers,
e.g. shell buffers and the *Python* buffer.  When using pdb to debug a
Python program, pdbtrack notices the pdb prompt and displays the
source file and line that the program is stopped at, much the same way
as gud-mode does for debugging C programs with gdb."
  :type 'boolean
  :group 'python)
(make-variable-buffer-local 'py-pdbtrack-do-tracking-p)

(defcustom py-pdbtrack-minor-mode-string " PDB"
  "*String to use in the minor mode list when pdbtrack is enabled."
  :type 'string
  :group 'python)

(defcustom py-import-check-point-max
  20000
  "Maximum number of characters to search for a Java-ish import statement.
When `python-mode' tries to calculate the shell to use (either a
CPython or a JPython shell), it looks at the so-called `shebang' line
-- i.e. #! line.  If that's not available, it looks at some of the
file heading imports to see if they look Java-like."
  :type 'integer
  :group 'python
  )

(defcustom py-jpython-packages
  '("java" "javax" "org" "com")
  "Imported packages that imply `jpython-mode'."
  :type '(repeat string)
  :group 'python)
  
;; Not customizable
(defvar py-master-file nil
  "If non-nil, execute the named file instead of the buffer's file.
The intent is to allow you to set this variable in the file's local
variable section, e.g.:

    # Local Variables:
    # py-master-file: \"master.py\"
    # End:

so that typing \\[py-execute-buffer] in that buffer executes the named
master file instead of the buffer's file.  If the file name has a
relative path, the value of variable `default-directory' for the
buffer is prepended to come up with a file name.")
(make-variable-buffer-local 'py-master-file)

(defcustom py-pychecker-command "pychecker"
  "*Shell command used to run Pychecker."
  :type 'string
  :group 'python
  :tag "Pychecker Command")

(defcustom py-pychecker-command-args '("--stdlib")
  "*List of string arguments to be passed to pychecker."
  :type '(repeat string)
  :group 'python
  :tag "Pychecker Command Args")

(defvar py-shell-alist
  '(("jpython" . 'jpython)
    ("jython" . 'jpython)
    ("python" . 'cpython))
  "*Alist of interpreters and python shells. Used by `py-choose-shell'
to select the appropriate python interpreter mode for a file.")


;; ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
;; NO USER DEFINABLE VARIABLES BEYOND THIS POINT

(defconst py-emacs-features
  (let (features)
   features)
  "A list of features extant in the Emacs you are using.
There are many flavors of Emacs out there, with different levels of
support for features needed by `python-mode'.")

;; Face for None, True, False, self, and Ellipsis
(defvar py-pseudo-keyword-face 'py-pseudo-keyword-face
  "Face for pseudo keywords in Python mode, like self, True, False, Ellipsis.")
(make-face 'py-pseudo-keyword-face)

(defun py-font-lock-mode-hook ()
  (or (face-differs-from-default-p 'py-pseudo-keyword-face)
      (copy-face 'font-lock-keyword-face 'py-pseudo-keyword-face)))
(add-hook 'font-lock-mode-hook 'py-font-lock-mode-hook)

(defvar python-font-lock-keywords
  (let ((kw1 (mapconcat 'identity
			'("and"      "assert"   "break"   "class"
			  "continue" "def"      "del"     "elif"
			  "else"     "except"   "exec"    "for"
			  "from"     "global"   "if"      "import"
			  "in"       "is"       "lambda"  "not"
			  "or"       "pass"     "print"   "raise"
			  "return"   "while"    "yield"
			  )
			"\\|"))
	(kw2 (mapconcat 'identity
			'("else:" "except:" "finally:" "try:")
			"\\|"))
	(kw3 (mapconcat 'identity
			'("ArithmeticError" "AssertionError"
			  "AttributeError" "DeprecationWarning" "EOFError"
			  "Ellipsis" "EnvironmentError" "Exception" "False"
			  "FloatingPointError" "FutureWarning" "IOError"
			  "ImportError" "IndentationError" "IndexError"
			  "KeyError" "KeyboardInterrupt" "LookupError"
			  "MemoryError" "NameError" "None" "NotImplemented"
			  "NotImplementedError" "OSError" "OverflowError"
			  "OverflowWarning" "PendingDeprecationWarning"
			  "ReferenceError" "RuntimeError" "RuntimeWarning"
			  "StopIteration" "SyntaxError"
			  "SyntaxWarning" "SystemError" "SystemExit"
			  "TabError" "True" "TypeError" "UnboundLocalError"
			  "UnicodeDecodeError" "UnicodeEncodeError"
			  "UnicodeError" "UnicodeTranslateError"
			  "UserWarning" "ValueError" "Warning"
			  "ZeroDivisionError" "__debug__"
			  "__import__" "__name__" "abs" "apply" "basestring"
			  "bool" "buffer" "callable" "chr" "classmethod"
			  "cmp" "compile" "complex" "copyright"
			  "delattr" "dict" "dir" "divmod"
			  "enumerate" "eval" "exit" "file"
			  "filter" "float" "getattr" "globals" "hasattr"
			  "hash" "hex" "id" "int"
			  "isinstance" "issubclass" "iter" "len" "license"
			  "list" "locals" "long" "map" "max" "min" "object"
			  "oct" "open" "ord" "pow" "property" "range"
			  "reload" "repr" "round"
			  "setattr" "slice" "staticmethod" "str" "sum"
			  "super" "tuple" "type" "unichr" "unicode" "vars"
			  "zip")
			"\\|"))
	)
    (list
     ;; keywords
     (cons (concat "\\b\\(" kw1 "\\)\\b[ \n\t(]") 1)
     ;; builtins when they don't appear as object attributes
     (cons (concat "\\(\\b\\|[.]\\)\\(" kw3 "\\)\\b[ \n\t(]") 2)
     ;; block introducing keywords with immediately following colons.
     ;; Yes "except" is in both lists.
     (cons (concat "\\b\\(" kw2 "\\)[ \n\t(]") 1)
     ;; `as' but only in "import foo as bar"
     '("[ \t]*\\(\\bfrom\\b.*\\)?\\bimport\\b.*\\b\\(as\\)\\b" . 2)
     ;; classes
     '("\\bclass[ \t]+\\([a-zA-Z_]+[a-zA-Z0-9_]*\\)"
       1 font-lock-type-face)
     ;; functions
     '("\\bdef[ \t]+\\([a-zA-Z_]+[a-zA-Z0-9_]*\\)"
       1 font-lock-function-name-face)
     ;; pseudo-keywords
     '("\\b\\(self\\|None\\|True\\|False\\|Ellipsis\\)\\b"
       1 py-pseudo-keyword-face)
     ))
  "Additional expressions to highlight in Python mode.")
(put 'python-mode 'font-lock-defaults '(python-font-lock-keywords))

;; have to bind py-file-queue before installing the kill-emacs-hook
(defvar py-file-queue nil
  "Queue of Python temp files awaiting execution.
Currently-active file is at the head of the list.")

(defvar py-pdbtrack-is-tracking-p nil)
(defvar py-pdbtrack-last-grubbed-buffer nil
  "Record of the last buffer used when the source path was invalid.

This buffer is consulted before the buffer-list history for satisfying
`py-pdbtrack-grub-for-buffer', since it's the most often the likely
prospect as debugging continues.")
(make-variable-buffer-local 'py-pdbtrack-last-grubbed-buffer)
(defvar py-pychecker-history nil)



;; Constants

(defconst py-stringlit-re
  (concat
   ;; These fail if backslash-quote ends the string (not worth
   ;; fixing?).  They precede the short versions so that the first two
   ;; quotes don't look like an empty short string.
   ;;
   ;; (maybe raw), long single quoted triple quoted strings (SQTQ),
   ;; with potential embedded single quotes
   "[rR]?'''[^']*\\(\\('[^']\\|''[^']\\)[^']*\\)*'''"
   "\\|"
   ;; (maybe raw), long double quoted triple quoted strings (DQTQ),
   ;; with potential embedded double quotes
   "[rR]?\"\"\"[^\"]*\\(\\(\"[^\"]\\|\"\"[^\"]\\)[^\"]*\\)*\"\"\""
   "\\|"
   "[rR]?'\\([^'\n\\]\\|\\\\.\\)*'"	; single-quoted
   "\\|"				; or
   "[rR]?\"\\([^\"\n\\]\\|\\\\.\\)*\""	; double-quoted
   )
  "Regular expression matching a Python string literal.")

(defconst py-continued-re
  ;; This is tricky because a trailing backslash does not mean
  ;; continuation if it's in a comment
  (concat
   "\\(" "[^#'\"\n\\]" "\\|" py-stringlit-re "\\)*"
   "\\\\$")
  "Regular expression matching Python backslash continuation lines.")
  
(defconst py-blank-or-comment-re "[ \t]*\\($\\|#\\)"
  "Regular expression matching a blank or comment line.")

(defconst py-outdent-re
  (concat "\\(" (mapconcat 'identity
			   '("else:"
			     "except\\(\\s +.*\\)?:"
			     "finally:"
			     "elif\\s +.*:")
			   "\\|")
	  "\\)")
  "Regular expression matching statements to be dedented one level.")
  
(defconst py-block-closing-keywords-re
  "\\(return\\|raise\\|break\\|continue\\|pass\\)"
  "Regular expression matching keywords which typically close a block.")

(defconst py-no-outdent-re
  (concat
   "\\("
   (mapconcat 'identity
	      (list "try:"
		    "except\\(\\s +.*\\)?:"
		    "while\\s +.*:"
		    "for\\s +.*:"
		    "if\\s +.*:"
		    "elif\\s +.*:"
		    (concat py-block-closing-keywords-re "[ \t\n]")
		    )
	      "\\|")
	  "\\)")
  "Regular expression matching lines not to dedent after.")

(defconst py-defun-start-re
  "^\\([ \t]*\\)def[ \t]+\\([a-zA-Z_0-9]+\\)\\|\\(^[a-zA-Z_0-9]+\\)[ \t]*="
  ;; If you change this, you probably have to change py-current-defun
  ;; as well.  This is only used by py-current-defun to find the name
  ;; for add-log.el.
  "Regular expression matching a function, method, or variable assignment.")

(defconst py-class-start-re "^class[ \t]*\\([a-zA-Z_0-9]+\\)"
  ;; If you change this, you probably have to change py-current-defun
  ;; as well.  This is only used by py-current-defun to find the name
  ;; for add-log.el.
  "Regular expression for finding a class name.")

(defconst py-traceback-line-re
  "[ \t]+File \"\\([^\"]+\\)\", line \\([0-9]+\\)"
  "Regular expression that describes tracebacks.")

;; pdbtrack contants
(defconst py-pdbtrack-stack-entry-regexp
;  "^> \\([^(]+\\)(\\([0-9]+\\))\\([?a-zA-Z0-9_]+\\)()"
  "^> \\(.*\\)(\\([0-9]+\\))\\([?a-zA-Z0-9_]+\\)()"
  "Regular expression pdbtrack uses to find a stack trace entry.")

(defconst py-pdbtrack-input-prompt "\n[(<]*pdb[>)]+ "
  "Regular expression pdbtrack uses to recognize a pdb prompt.")

(defconst py-pdbtrack-track-range 10000
  "Max number of characters from end of buffer to search for stack entry.")



;; Major mode boilerplate

;; define a mode-specific abbrev table for those who use such things
(defvar python-mode-abbrev-table nil
  "Abbrev table in use in `python-mode' buffers.")
(define-abbrev-table 'python-mode-abbrev-table nil)

(defvar python-mode-hook nil
  "*Hook called by `python-mode'.")

(defvar jpython-mode-hook nil
  "*Hook called by `jpython-mode'. `jpython-mode' also calls
`python-mode-hook'.")

(defvar py-shell-hook nil
  "*Hook called by `py-shell'.")

;; In previous version of python-mode.el, the hook was incorrectly
;; called py-mode-hook, and was not defvar'd.  Deprecate its use.
(and (fboundp 'make-obsolete-variable)
     (make-obsolete-variable 'py-mode-hook 'python-mode-hook))

(defvar py-mode-map ()
  "Keymap used in `python-mode' buffers.")
(if py-mode-map
    nil
  (setq py-mode-map (make-sparse-keymap))
  ;; electric keys
  (define-key py-mode-map ":" 'py-electric-colon)
  ;; indentation level modifiers
  (define-key py-mode-map "\C-c\C-l"  'py-shift-region-left)
  (define-key py-mode-map "\C-c\C-r"  'py-shift-region-right)
  (define-key py-mode-map "\C-c<"     'py-shift-region-left)
  (define-key py-mode-map "\C-c>"     'py-shift-region-right)
  ;; paragraph and string filling
  (define-key py-mode-map "\eq"       'py-fill-paragraph)
  ;; subprocess commands
  (define-key py-mode-map "\C-c\C-c"  'py-execute-buffer)
  (define-key py-mode-map "\C-c\C-m"  'py-execute-import-or-reload)
  (define-key py-mode-map "\C-c\C-s"  'py-execute-string)
  (define-key py-mode-map "\C-c|"     'py-execute-region)
  (define-key py-mode-map "\e\C-x"    'py-execute-def-or-class)
  (define-key py-mode-map "\C-c!"     'py-shell)
  (define-key py-mode-map "\C-c\C-t"  'py-toggle-shells)
  ;; Caution!  Enter here at your own risk.  We are trying to support
  ;; several behaviors and it gets disgusting. :-( This logic ripped
  ;; largely from CC Mode.
  ;;
  ;; In XEmacs 19, Emacs 19, and Emacs 20, we use this to bind
  ;; backwards deletion behavior to DEL, which both Delete and
  ;; Backspace get translated to.  There's no way to separate this
  ;; behavior in a clean way, so deal with it!  Besides, it's been
  ;; this way since the dawn of time.
  (if (not (boundp 'delete-key-deletes-forward))
      (define-key py-mode-map "\177" 'py-electric-backspace)
    ;; However, XEmacs 20 actually achieved enlightenment.  It is
    ;; possible to sanely define both backward and forward deletion
    ;; behavior under X separately (TTYs are forever beyond hope, but
    ;; who cares?  XEmacs 20 does the right thing with these too).
    (define-key py-mode-map [delete]    'py-electric-delete)
    (define-key py-mode-map [backspace] 'py-electric-backspace))
  ;; Separate M-BS from C-M-h.  The former should remain
  ;; backward-kill-word.
  (define-key py-mode-map [(control meta h)] 'py-mark-def-or-class)
  (define-key py-mode-map "\C-c\C-k"  'py-mark-block)
  ;; Miscellaneous
  (define-key py-mode-map "\C-c:"     'py-guess-indent-offset)
  (define-key py-mode-map "\C-c\t"    'py-indent-region)
  (define-key py-mode-map "\C-c\C-d"  'py-pdbtrack-toggle-stack-tracking)
  (define-key py-mode-map "\C-c\C-n"  'py-next-statement)
  (define-key py-mode-map "\C-c\C-p"  'py-previous-statement)
  (define-key py-mode-map "\C-c\C-u"  'py-goto-block-up)
  (define-key py-mode-map "\C-c#"     'py-comment-region)
  (define-key py-mode-map "\C-c?"     'py-describe-mode)
  (define-key py-mode-map "\C-c\C-h"  'py-help-at-point)
  (define-key py-mode-map "\e\C-a"    'py-beginning-of-def-or-class)
  (define-key py-mode-map "\e\C-e"    'py-end-of-def-or-class)
  (define-key py-mode-map "\C-c-"     'py-up-exception)
  (define-key py-mode-map "\C-c="     'py-down-exception)
  ;; stuff that is `standard' but doesn't interface well with
  ;; python-mode, which forces us to rebind to special commands
  (define-key py-mode-map "\C-xnd"    'py-narrow-to-defun)
  ;; information
  (define-key py-mode-map "\C-c\C-b" 'py-submit-bug-report)
  (define-key py-mode-map "\C-c\C-v" 'py-version)
  (define-key py-mode-map "\C-c\C-w" 'py-pychecker-run)
  ;; shadow global bindings for newline-and-indent w/ the py- version.
  ;; BAW - this is extremely bad form, but I'm not going to change it
  ;; for now.
  (mapcar #'(lambda (key)
	      (define-key py-mode-map key 'py-newline-and-indent))
	  (where-is-internal 'newline-and-indent))
  ;; Force RET to be py-newline-and-indent even if it didn't get
  ;; mapped by the above code.  motivation: Emacs' default binding for
  ;; RET is `newline' and C-j is `newline-and-indent'.  Most Pythoneers
  ;; expect RET to do a `py-newline-and-indent' and any Emacsers who
  ;; dislike this are probably knowledgeable enough to do a rebind.
  ;; However, we do *not* change C-j since many Emacsers have already
  ;; swapped RET and C-j and they don't want C-j bound to `newline' to 
  ;; change.
  (define-key py-mode-map "\C-m" 'py-newline-and-indent)
  )

(defvar py-mode-output-map nil
  "Keymap used in *Python Output* buffers.")
(if py-mode-output-map
    nil
  (setq py-mode-output-map (make-sparse-keymap))
  (define-key py-mode-output-map [button2]  'py-mouseto-exception)
  (define-key py-mode-output-map "\C-c\C-c" 'py-goto-exception)
  ;; TBD: Disable all self-inserting keys.  This is bogus, we should
  ;; really implement this as *Python Output* buffer being read-only
  (mapcar #' (lambda (key)
	       (define-key py-mode-output-map key
		 #'(lambda () (interactive) (beep))))
	     (where-is-internal 'self-insert-command))
  )

(defvar py-shell-map nil
  "Keymap used in *Python* shell buffers.")
(if py-shell-map
    nil
  (setq py-shell-map (copy-keymap comint-mode-map))
  (define-key py-shell-map [tab]   'tab-to-tab-stop)
  (define-key py-shell-map "\C-c-" 'py-up-exception)
  (define-key py-shell-map "\C-c=" 'py-down-exception)
  )

(defvar py-mode-syntax-table nil
  "Syntax table used in `python-mode' buffers.")
(when (not py-mode-syntax-table)
  (setq py-mode-syntax-table (make-syntax-table))
  (modify-syntax-entry ?\( "()" py-mode-syntax-table)
  (modify-syntax-entry ?\) ")(" py-mode-syntax-table)
  (modify-syntax-entry ?\[ "(]" py-mode-syntax-table)
  (modify-syntax-entry ?\] ")[" py-mode-syntax-table)
  (modify-syntax-entry ?\{ "(}" py-mode-syntax-table)
  (modify-syntax-entry ?\} "){" py-mode-syntax-table)
  ;; Add operator symbols misassigned in the std table
  (modify-syntax-entry ?\$ "."  py-mode-syntax-table)
  (modify-syntax-entry ?\% "."  py-mode-syntax-table)
  (modify-syntax-entry ?\& "."  py-mode-syntax-table)
  (modify-syntax-entry ?\* "."  py-mode-syntax-table)
  (modify-syntax-entry ?\+ "."  py-mode-syntax-table)
  (modify-syntax-entry ?\- "."  py-mode-syntax-table)
  (modify-syntax-entry ?\/ "."  py-mode-syntax-table)
  (modify-syntax-entry ?\< "."  py-mode-syntax-table)
  (modify-syntax-entry ?\= "."  py-mode-syntax-table)
  (modify-syntax-entry ?\> "."  py-mode-syntax-table)
  (modify-syntax-entry ?\| "."  py-mode-syntax-table)
  ;; For historical reasons, underscore is word class instead of
  ;; symbol class.  GNU conventions say it should be symbol class, but
  ;; there's a natural conflict between what major mode authors want
  ;; and what users expect from `forward-word' and `backward-word'.
  ;; Guido and I have hashed this out and have decided to keep
  ;; underscore in word class.  If you're tempted to change it, try
  ;; binding M-f and M-b to py-forward-into-nomenclature and
  ;; py-backward-into-nomenclature instead.  This doesn't help in all
  ;; situations where you'd want the different behavior
  ;; (e.g. backward-kill-word).
  (modify-syntax-entry ?\_ "w"  py-mode-syntax-table)
  ;; Both single quote and double quote are string delimiters
  (modify-syntax-entry ?\' "\"" py-mode-syntax-table)
  (modify-syntax-entry ?\" "\"" py-mode-syntax-table)
  ;; comment delimiters
  (modify-syntax-entry ?\# "<"  py-mode-syntax-table)
  (modify-syntax-entry ?\n ">"  py-mode-syntax-table)
  )

;; An auxiliary syntax table which places underscore and dot in the
;; symbol class for simplicity
(defvar py-dotted-expression-syntax-table nil
  "Syntax table used to identify Python dotted expressions.")
(when (not py-dotted-expression-syntax-table)
  (setq py-dotted-expression-syntax-table
	(copy-syntax-table py-mode-syntax-table))
  (modify-syntax-entry ?_ "_" py-dotted-expression-syntax-table)
  (modify-syntax-entry ?. "_" py-dotted-expression-syntax-table))



;; Utilities
(defmacro py-safe (&rest body)
  "Safely execute BODY, return nil if an error occurred."
  (` (condition-case nil
	 (progn (,@ body))
       (error nil))))

(defsubst py-keep-region-active ()
  "Keep the region active in XEmacs."
  ;; Ignore byte-compiler warnings you might see.  Also note that
  ;; FSF's Emacs 19 does it differently; its policy doesn't require us
  ;; to take explicit action.
  (and (boundp 'zmacs-region-stays)
       (setq zmacs-region-stays t)))

(defsubst py-point (position)
  "Returns the value of point at certain commonly referenced POSITIONs.
POSITION can be one of the following symbols:

  bol  -- beginning of line
  eol  -- end of line
  bod  -- beginning of def or class
  eod  -- end of def or class
  bob  -- beginning of buffer
  eob  -- end of buffer
  boi  -- back to indentation
  bos  -- beginning of statement

This function does not modify point or mark."
  (let ((here (point)))
    (cond
     ((eq position 'bol) (beginning-of-line))
     ((eq position 'eol) (end-of-line))
     ((eq position 'bod) (py-beginning-of-def-or-class))
     ((eq position 'eod) (py-end-of-def-or-class))
     ;; Kind of funny, I know, but useful for py-up-exception.
     ((eq position 'bob) (beginning-of-buffer))
     ((eq position 'eob) (end-of-buffer))
     ((eq position 'boi) (back-to-indentation))
     ((eq position 'bos) (py-goto-initial-line))
     (t (error "Unknown buffer position requested: %s" position))
     )
    (prog1
	(point)
      (goto-char here))))

(defsubst py-highlight-line (from to file line)
  (cond
   ((fboundp 'make-extent)
    ;; XEmacs
    (let ((e (make-extent from to)))
      (set-extent-property e 'mouse-face 'highlight)
      (set-extent-property e 'py-exc-info (cons file line))
      (set-extent-property e 'keymap py-mode-output-map)))
   (t
    ;; Emacs -- Please port this!
    )
   ))

(defun py-in-literal (&optional lim)
  "Return non-nil if point is in a Python literal (a comment or string).
Optional argument LIM indicates the beginning of the containing form,
i.e. the limit on how far back to scan."
  ;; This is the version used for non-XEmacs, which has a nicer
  ;; interface.
  ;;
  ;; WARNING: Watch out for infinite recursion.
  (let* ((lim (or lim (py-point 'bod)))
	 (state (parse-partial-sexp lim (point))))
    (cond
     ((nth 3 state) 'string)
     ((nth 4 state) 'comment)
     (t nil))))

;; XEmacs has a built-in function that should make this much quicker.
;; In this case, lim is ignored
(defun py-fast-in-literal (&optional lim)
  "Fast version of `py-in-literal', used only by XEmacs.
Optional LIM is ignored."
  ;; don't have to worry about context == 'block-comment
  (buffer-syntactic-context))

(if (fboundp 'buffer-syntactic-context)
    (defalias 'py-in-literal 'py-fast-in-literal))



;; Menu definitions, only relevent if you have the easymenu.el package
;; (standard in the latest Emacs 19 and XEmacs 19 distributions).
(defvar py-menu nil
  "Menu for Python Mode.
This menu will get created automatically if you have the `easymenu'
package.  Note that the latest X/Emacs releases contain this package.")

(and (py-safe (require 'easymenu) t)
     (easy-menu-define
      py-menu py-mode-map "Python Mode menu"
      '("Python"
	["Comment Out Region"   py-comment-region  (mark)]
	["Uncomment Region"     (py-comment-region (point) (mark) '(4)) (mark)]
	"-"
	["Mark current block"   py-mark-block t]
	["Mark current def"     py-mark-def-or-class t]
	["Mark current class"   (py-mark-def-or-class t) t]
	"-"
	["Shift region left"    py-shift-region-left (mark)]
	["Shift region right"   py-shift-region-right (mark)]
	"-"
	["Import/reload file"   py-execute-import-or-reload t]
	["Execute buffer"       py-execute-buffer t]
	["Execute region"       py-execute-region (mark)]
	["Execute def or class" py-execute-def-or-class (mark)]
	["Execute string"       py-execute-string t]
	["Start interpreter..." py-shell t]
	"-"
	["Go to start of block" py-goto-block-up t]
	["Go to start of class" (py-beginning-of-def-or-class t) t]
	["Move to end of class" (py-end-of-def-or-class t) t]
	["Move to start of def" py-beginning-of-def-or-class t]
	["Move to end of def"   py-end-of-def-or-class t]
	"-"
	["Describe mode"        py-describe-mode t]
	)))



;; Imenu definitions
(defvar py-imenu-class-regexp
  (concat				; <<classes>>
   "\\("				;
   "^[ \t]*"				; newline and maybe whitespace
   "\\(class[ \t]+[a-zA-Z0-9_]+\\)"	; class name
					; possibly multiple superclasses
   "\\([ \t]*\\((\\([a-zA-Z0-9_,. \t\n]\\)*)\\)?\\)"
   "[ \t]*:"				; and the final :
   "\\)"				; >>classes<<
   )
  "Regexp for Python classes for use with the Imenu package."
  )

(defvar py-imenu-method-regexp
  (concat                               ; <<methods and functions>>
   "\\("                                ; 
   "^[ \t]*"                            ; new line and maybe whitespace
   "\\(def[ \t]+"                       ; function definitions start with def
   "\\([a-zA-Z0-9_]+\\)"                ;   name is here
					;   function arguments...
;;   "[ \t]*(\\([-+/a-zA-Z0-9_=,\* \t\n.()\"'#]*\\))"
   "[ \t]*(\\([^:#]*\\))"
   "\\)"                                ; end of def
   "[ \t]*:"                            ; and then the :
   "\\)"                                ; >>methods and functions<<
   )
  "Regexp for Python methods/functions for use with the Imenu package."
  )

(defvar py-imenu-method-no-arg-parens '(2 8)
  "Indices into groups of the Python regexp for use with Imenu.

Using these values will result in smaller Imenu lists, as arguments to
functions are not listed.

See the variable `py-imenu-show-method-args-p' for more
information.")

(defvar py-imenu-method-arg-parens '(2 7)
  "Indices into groups of the Python regexp for use with imenu.
Using these values will result in large Imenu lists, as arguments to
functions are listed.

See the variable `py-imenu-show-method-args-p' for more
information.")

;; Note that in this format, this variable can still be used with the
;; imenu--generic-function. Otherwise, there is no real reason to have
;; it.
(defvar py-imenu-generic-expression
  (cons
   (concat 
    py-imenu-class-regexp
    "\\|"				; or...
    py-imenu-method-regexp
    )
   py-imenu-method-no-arg-parens)
  "Generic Python expression which may be used directly with Imenu.
Used by setting the variable `imenu-generic-expression' to this value.
Also, see the function \\[py-imenu-create-index] for a better
alternative for finding the index.")

;; These next two variables are used when searching for the Python
;; class/definitions. Just saving some time in accessing the
;; generic-python-expression, really.
(defvar py-imenu-generic-regexp nil)
(defvar py-imenu-generic-parens nil)


(defun py-imenu-create-index-function ()
  "Python interface function for the Imenu package.
Finds all Python classes and functions/methods. Calls function
\\[py-imenu-create-index-engine].  See that function for the details
of how this works."
  (setq py-imenu-generic-regexp (car py-imenu-generic-expression)
	py-imenu-generic-parens (if py-imenu-show-method-args-p
				    py-imenu-method-arg-parens
				  py-imenu-method-no-arg-parens))
  (goto-char (point-min))
  ;; Warning: When the buffer has no classes or functions, this will
  ;; return nil, which seems proper according to the Imenu API, but
  ;; causes an error in the XEmacs port of Imenu.  Sigh.
  (py-imenu-create-index-engine nil))

(defun py-imenu-create-index-engine (&optional start-indent)
  "Function for finding Imenu definitions in Python.

Finds all definitions (classes, methods, or functions) in a Python
file for the Imenu package.

Returns a possibly nested alist of the form

	(INDEX-NAME . INDEX-POSITION)

The second element of the alist may be an alist, producing a nested
list as in

	(INDEX-NAME . INDEX-ALIST)

This function should not be called directly, as it calls itself
recursively and requires some setup.  Rather this is the engine for
the function \\[py-imenu-create-index-function].

It works recursively by looking for all definitions at the current
indention level.  When it finds one, it adds it to the alist.  If it
finds a definition at a greater indentation level, it removes the
previous definition from the alist. In its place it adds all
definitions found at the next indentation level.  When it finds a
definition that is less indented then the current level, it returns
the alist it has created thus far.

The optional argument START-INDENT indicates the starting indentation
at which to continue looking for Python classes, methods, or
functions.  If this is not supplied, the function uses the indentation
of the first definition found."
  (let (index-alist
	sub-method-alist
	looking-p
	def-name prev-name
	cur-indent def-pos
	(class-paren (first  py-imenu-generic-parens)) 
	(def-paren   (second py-imenu-generic-parens)))
    (setq looking-p
	  (re-search-forward py-imenu-generic-regexp (point-max) t))
    (while looking-p
      (save-excursion
	;; used to set def-name to this value but generic-extract-name
	;; is new to imenu-1.14. this way it still works with
	;; imenu-1.11
	;;(imenu--generic-extract-name py-imenu-generic-parens))
	(let ((cur-paren (if (match-beginning class-paren)
			     class-paren def-paren)))
	  (setq def-name
		(buffer-substring-no-properties (match-beginning cur-paren)
						(match-end cur-paren))))
	(save-match-data
	  (py-beginning-of-def-or-class 'either))
	(beginning-of-line)
	(setq cur-indent (current-indentation)))
      ;; HACK: want to go to the next correct definition location.  We
      ;; explicitly list them here but it would be better to have them
      ;; in a list.
      (setq def-pos
	    (or (match-beginning class-paren)
		(match-beginning def-paren)))
      ;; if we don't have a starting indent level, take this one
      (or start-indent
	  (setq start-indent cur-indent))
      ;; if we don't have class name yet, take this one
      (or prev-name
	  (setq prev-name def-name))
      ;; what level is the next definition on?  must be same, deeper
      ;; or shallower indentation
      (cond
       ;; Skip code in comments and strings
       ((py-in-literal))
       ;; at the same indent level, add it to the list...
       ((= start-indent cur-indent)
	(push (cons def-name def-pos) index-alist))
       ;; deeper indented expression, recurse
       ((< start-indent cur-indent)
	;; the point is currently on the expression we're supposed to
	;; start on, so go back to the last expression. The recursive
	;; call will find this place again and add it to the correct
	;; list
	(re-search-backward py-imenu-generic-regexp (point-min) 'move)
	(setq sub-method-alist (py-imenu-create-index-engine cur-indent))
	(if sub-method-alist
	    ;; we put the last element on the index-alist on the start
	    ;; of the submethod alist so the user can still get to it.
	    (let ((save-elmt (pop index-alist)))
	      (push (cons prev-name
			  (cons save-elmt sub-method-alist))
		    index-alist))))
       ;; found less indented expression, we're done.
       (t 
	(setq looking-p nil)
	(re-search-backward py-imenu-generic-regexp (point-min) t)))
      ;; end-cond
      (setq prev-name def-name)
      (and looking-p
	   (setq looking-p
		 (re-search-forward py-imenu-generic-regexp
				    (point-max) 'move))))
    (nreverse index-alist)))



(defun py-choose-shell-by-shebang ()
  "Choose CPython or JPython mode by looking at #! on the first line.
Returns the appropriate mode function.
Used by `py-choose-shell', and similar to but distinct from
`set-auto-mode', though it uses `auto-mode-interpreter-regexp' (if available)."
  ;; look for an interpreter specified in the first line
  ;; similar to set-auto-mode (files.el)
  (let* ((re (if (boundp 'auto-mode-interpreter-regexp)
		 auto-mode-interpreter-regexp
	       ;; stolen from Emacs 21.2
	       "#![ \t]?\\([^ \t\n]*/bin/env[ \t]\\)?\\([^ \t\n]+\\)"))
	 (interpreter (save-excursion
			(goto-char (point-min))
			(if (looking-at re)
			    (match-string 2)
			  "")))
	 elt)
    ;; Map interpreter name to a mode.
    (setq elt (assoc (file-name-nondirectory interpreter)
		     py-shell-alist))
    (and elt (caddr elt))))



(defun py-choose-shell-by-import ()
  "Choose CPython or JPython mode based imports.
If a file imports any packages in `py-jpython-packages', within
`py-import-check-point-max' characters from the start of the file,
return `jpython', otherwise return nil."
  (let (mode)
    (save-excursion
      (goto-char (point-min))
      (while (and (not mode)
		  (search-forward-regexp
		   "^\\(\\(from\\)\\|\\(import\\)\\) \\([^ \t\n.]+\\)"
		   py-import-check-point-max t))
	(setq mode (and (member (match-string 4) py-jpython-packages)
			'jpython
			))))
    mode))


(defun py-choose-shell ()
  "Choose CPython or JPython mode. Returns the appropriate mode function.
This does the following:
 - look for an interpreter with `py-choose-shell-by-shebang'
 - examine imports using `py-choose-shell-by-import'
 - default to the variable `py-default-interpreter'"
  (interactive)
  (or (py-choose-shell-by-shebang)
      (py-choose-shell-by-import)
      py-default-interpreter
;      'cpython ;; don't use to py-default-interpreter, because default
;               ;; is only way to choose CPython
      ))


;;;###autoload
(defun python-mode ()
  "Major mode for editing Python files.
To submit a problem report, enter `\\[py-submit-bug-report]' from a
`python-mode' buffer.  Do `\\[py-describe-mode]' for detailed
documentation.  To see what version of `python-mode' you are running,
enter `\\[py-version]'.

This mode knows about Python indentation, tokens, comments and
continuation lines.  Paragraphs are separated by blank lines only.

COMMANDS
\\{py-mode-map}
VARIABLES

py-indent-offset\t\tindentation increment
py-block-comment-prefix\t\tcomment string used by `comment-region'
py-python-command\t\tshell command to invoke Python interpreter
py-temp-directory\t\tdirectory used for temp files (if needed)
py-beep-if-tab-change\t\tring the bell if `tab-width' is changed"
  (interactive)
  ;; set up local variables
  (kill-all-local-variables)
  (make-local-variable 'font-lock-defaults)
  (make-local-variable 'paragraph-separate)
  (make-local-variable 'paragraph-start)
  (make-local-variable 'require-final-newline)
  (make-local-variable 'comment-start)
  (make-local-variable 'comment-end)
  (make-local-variable 'comment-start-skip)
  (make-local-variable 'comment-column)
  (make-local-variable 'comment-indent-function)
  (make-local-variable 'indent-region-function)
  (make-local-variable 'indent-line-function)
  (make-local-variable 'add-log-current-defun-function)
  ;;
  (set-syntax-table py-mode-syntax-table)
  (setq major-mode              'python-mode
	mode-name               "Python"
	local-abbrev-table      python-mode-abbrev-table
	font-lock-defaults      '(python-font-lock-keywords)
	paragraph-separate      "^[ \t]*$"
	paragraph-start         "^[ \t]*$"
	require-final-newline   t
	comment-start           "# "
	comment-end             ""
	comment-start-skip      "# *"
	comment-column          40
	comment-indent-function 'py-comment-indent-function
	indent-region-function  'py-indent-region
	indent-line-function    'py-indent-line
	;; tell add-log.el how to find the current function/method/variable
	add-log-current-defun-function 'py-current-defun
	)
  (use-local-map py-mode-map)
  ;; add the menu
  (if py-menu
      (easy-menu-add py-menu))
  ;; Emacs 19 requires this
  (if (boundp 'comment-multi-line)
      (setq comment-multi-line nil))
  ;; Install Imenu if available
  (when (py-safe (require 'imenu))
    (setq imenu-create-index-function #'py-imenu-create-index-function)
    (setq imenu-generic-expression py-imenu-generic-expression)
    (if (fboundp 'imenu-add-to-menubar)
	(imenu-add-to-menubar (format "%s-%s" "IM" mode-name)))
    )
  ;; Run the mode hook.  Note that py-mode-hook is deprecated.
  (if python-mode-hook
      (run-hooks 'python-mode-hook)
    (run-hooks 'py-mode-hook))
  ;; Now do the automagical guessing
  (if py-smart-indentation
    (let ((offset py-indent-offset))
      ;; It's okay if this fails to guess a good value
      (if (and (py-safe (py-guess-indent-offset))
	       (<= py-indent-offset 8)
	       (>= py-indent-offset 2))
	  (setq offset py-indent-offset))
      (setq py-indent-offset offset)
      ;; Only turn indent-tabs-mode off if tab-width !=
      ;; py-indent-offset.  Never turn it on, because the user must
      ;; have explicitly turned it off.
      (if (/= tab-width py-indent-offset)
	  (setq indent-tabs-mode nil))
      ))
  ;; Set the default shell if not already set
  (when (null py-which-shell)
    (py-toggle-shells (py-choose-shell))))


(defun jpython-mode ()
  "Major mode for editing JPython/Jython files.
This is a simple wrapper around `python-mode'.
It runs `jpython-mode-hook' then calls `python-mode.'
It is added to `interpreter-mode-alist' and `py-choose-shell'.
"
  (interactive)
  (python-mode)
  (py-toggle-shells 'jpython)
  (when jpython-mode-hook
      (run-hooks 'jpython-mode-hook)))


;; It's handy to add recognition of Python files to the
;; interpreter-mode-alist and to auto-mode-alist.  With the former, we
;; can specify different `derived-modes' based on the #! line, but
;; with the latter, we can't.  So we just won't add them if they're
;; already added.
(let ((modes '(("jpython" . jpython-mode)
	       ("jython" . jpython-mode)
	       ("python" . python-mode))))
  (while modes
    (when (not (assoc (car modes) interpreter-mode-alist))
      (push (car modes) interpreter-mode-alist))
    (setq modes (cdr modes))))

(when (not (or (rassq 'python-mode auto-mode-alist)
	       (rassq 'jpython-mode auto-mode-alist)))
  (push '("\\.py$" . python-mode) auto-mode-alist))



;; electric characters
(defun py-outdent-p ()
  "Returns non-nil if the current line should dedent one level."
  (save-excursion
    (and (progn (back-to-indentation)
		(looking-at py-outdent-re))
	 ;; short circuit infloop on illegal construct
	 (not (bobp))
	 (progn (forward-line -1)
		(py-goto-initial-line)
		(back-to-indentation)
		(while (or (looking-at py-blank-or-comment-re)
			   (bobp))
		  (backward-to-indentation 1))
		(not (looking-at py-no-outdent-re)))
	 )))

(defun py-electric-colon (arg)
  "Insert a colon.
In certain cases the line is dedented appropriately.  If a numeric
argument ARG is provided, that many colons are inserted
non-electrically.  Electric behavior is inhibited inside a string or
comment."
  (interactive "*P")
  (self-insert-command (prefix-numeric-value arg))
  ;; are we in a string or comment?
  (if (save-excursion
	(let ((pps (parse-partial-sexp (save-excursion
					 (py-beginning-of-def-or-class)
					 (point))
				       (point))))
	  (not (or (nth 3 pps) (nth 4 pps)))))
      (save-excursion
	(let ((here (point))
	      (outdent 0)
	      (indent (py-compute-indentation t)))
	  (if (and (not arg)
		   (py-outdent-p)
		   (= indent (save-excursion
			       (py-next-statement -1)
			       (py-compute-indentation t)))
		   )
	      (setq outdent py-indent-offset))
	  ;; Don't indent, only dedent.  This assumes that any lines
	  ;; that are already dedented relative to
	  ;; py-compute-indentation were put there on purpose.  It's
	  ;; highly annoying to have `:' indent for you.  Use TAB, C-c
	  ;; C-l or C-c C-r to adjust.  TBD: Is there a better way to
	  ;; determine this???
	  (if (< (current-indentation) indent) nil
	    (goto-char here)
	    (beginning-of-line)
	    (delete-horizontal-space)
	    (indent-to (- indent outdent))
	    )))))


;; Python subprocess utilities and filters
(defun py-execute-file (proc filename)
  "Send to Python interpreter process PROC \"exec(open('FILENAME').read())\".
Make that process's buffer visible and force display.  Also make
comint believe the user typed this string so that
`kill-output-from-shell' does The Right Thing."
  (let ((curbuf (current-buffer))
	(procbuf (process-buffer proc))
;	(comint-scroll-to-bottom-on-output t)
	(msg (format "## working on region in file %s...\n" filename))
	(cmd (format "exec(open(r'%s').read())\n" filename)))
    (unwind-protect
	(save-excursion
	  (set-buffer procbuf)
	  (goto-char (point-max))
	  (move-marker (process-mark proc) (point))
	  (funcall (process-filter proc) proc msg))
      (set-buffer curbuf))
    (process-send-string proc cmd)))

(defun py-comint-output-filter-function (string)
  "Watch output for Python prompt and exec next file waiting in queue.
This function is appropriate for `comint-output-filter-functions'."
  ;; TBD: this should probably use split-string
  (when (and (or (string-equal string ">>> ")
		 (and (>= (length string) 5)
		      (string-equal (substring string -5) "\n>>> ")))
	     py-file-queue)
    (pop-to-buffer (current-buffer))
    (py-safe (delete-file (car py-file-queue)))
    (setq py-file-queue (cdr py-file-queue))
    (if py-file-queue
	(let ((pyproc (get-buffer-process (current-buffer))))
	  (py-execute-file pyproc (car py-file-queue))))
    ))

(defun py-pdbtrack-overlay-arrow (activation)
  "Activate or de arrow at beginning-of-line in current buffer."
  ;; This was derived/simplified from edebug-overlay-arrow
  (cond (activation
	 (setq overlay-arrow-position (make-marker))
	 (setq overlay-arrow-string "=>")
	 (set-marker overlay-arrow-position (py-point 'bol) (current-buffer))
	 (setq py-pdbtrack-is-tracking-p t))
	(overlay-arrow-position
	 (setq overlay-arrow-position nil)
	 (setq py-pdbtrack-is-tracking-p nil))
	))

(defun py-pdbtrack-track-stack-file (text)
  "Show the file indicated by the pdb stack entry line, in a separate window.

Activity is disabled if the buffer-local variable
`py-pdbtrack-do-tracking-p' is nil.

We depend on the pdb input prompt matching `py-pdbtrack-input-prompt'
at the beginning of the line.

If the traceback target file path is invalid, we look for the most
recently visited python-mode buffer which either has the name of the
current function \(or class) or which defines the function \(or
class).  This is to provide for remote scripts, eg, Zope's 'Script
(Python)' - put a _copy_ of the script in a buffer named for the
script, and set to python-mode, and pdbtrack will find it.)"
  ;; Instead of trying to piece things together from partial text
  ;; (which can be almost useless depending on Emacs version), we
  ;; monitor to the point where we have the next pdb prompt, and then
  ;; check all text from comint-last-input-end to process-mark.
  ;;
  ;; Also, we're very conservative about clearing the overlay arrow,
  ;; to minimize residue.  This means, for instance, that executing
  ;; other pdb commands wipe out the highlight.  You can always do a
  ;; 'where' (aka 'w') command to reveal the overlay arrow.
  (let* ((origbuf (current-buffer))
	 (currproc (get-buffer-process origbuf)))

    (if (not (and currproc py-pdbtrack-do-tracking-p))
        (py-pdbtrack-overlay-arrow nil)

      (let* ((procmark (process-mark currproc))
             (block (buffer-substring (max comint-last-input-end
                                           (- procmark
                                              py-pdbtrack-track-range))
                                      procmark))
             target target_fname target_lineno)

        (if (not (string-match (concat py-pdbtrack-input-prompt "$") block))
            (py-pdbtrack-overlay-arrow nil)

          (setq target (py-pdbtrack-get-source-buffer block))

          (if (stringp target)
              (message "pdbtrack: %s" target)

            (setq target_lineno (car target))
            (setq target_buffer (cadr target))
            (setq target_fname (buffer-file-name target_buffer))
            (switch-to-buffer-other-window target_buffer)
            (goto-line target_lineno)
            (message "pdbtrack: line %s, file %s" target_lineno target_fname)
            (py-pdbtrack-overlay-arrow t)
            (pop-to-buffer origbuf t)

            )))))
  )

(defun py-pdbtrack-get-source-buffer (block)
  "Return line number and buffer of code indicated by block's traceback text.

We look first to visit the file indicated in the trace.

Failing that, we look for the most recently visited python-mode buffer
with the same name or having 
having the named function.

If we're unable find the source code we return a string describing the
problem as best as we can determine."

  (if (not (string-match py-pdbtrack-stack-entry-regexp block))

      "Traceback cue not found"

    (let* ((filename (match-string 1 block))
           (lineno (string-to-int (match-string 2 block)))
           (funcname (match-string 3 block))
           funcbuffer)

      (cond ((file-exists-p filename)
             (list lineno (find-file-noselect filename)))

            ((setq funcbuffer (py-pdbtrack-grub-for-buffer funcname lineno))
             (if (string-match "/Script (Python)$" filename)
                 ;; Add in number of lines for leading '##' comments:
                 (setq lineno
                       (+ lineno
                          (save-excursion
                            (set-buffer funcbuffer)
                            (count-lines
                             (point-min)
                             (max (point-min)
                                  (string-match "^\\([^#]\\|#[^#]\\|#$\\)"
                                                (buffer-substring (point-min)
                                                                  (point-max)))
                                  ))))))
             (list lineno funcbuffer))

            ((= (elt filename 0) ?\<)
             (format "(Non-file source: '%s')" filename))

            (t (format "Not found: %s(), %s" funcname filename)))
      )
    )
  )

(defun py-pdbtrack-grub-for-buffer (funcname lineno)
  "Find most recent buffer itself named or having function funcname.

We first check the last buffer this function found, if any, then walk
throught the buffer-list history for python-mode buffers that are
named for funcname or define a function funcname."
  (let ((buffers (buffer-list))
        curbuf
        got)
    (while (and buffers (not got))
      (setq buf (car buffers)
            buffers (cdr buffers))
      (if (and (save-excursion (set-buffer buf)
                               (string= major-mode "python-mode"))
               (or (string-match funcname (buffer-name buf))
                   (string-match (concat "^\\s-*\\(def\\|class\\)\\s-+"
                                         funcname "\\s-*(")
                                 (save-excursion
                                   (set-buffer buf)
                                   (buffer-substring (point-min)
                                                     (point-max))))))
          (setq got buf)))
    (setq py-pdbtrack-last-grubbed-buffer got)))

(defun py-postprocess-output-buffer (buf)
  "Highlight exceptions found in BUF.
If an exception occurred return t, otherwise return nil.  BUF must exist."
  (let (line file bol err-p)
    (save-excursion
      (set-buffer buf)
      (beginning-of-buffer)
      (while (re-search-forward py-traceback-line-re nil t)
	(setq file (match-string 1)
	      line (string-to-int (match-string 2))
	      bol (py-point 'bol))
	(py-highlight-line bol (py-point 'eol) file line)))
    (when (and py-jump-on-exception line)
      (beep)
      (py-jump-to-exception file line)
      (setq err-p t))
    err-p))



;;; Subprocess commands

;; only used when (memq 'broken-temp-names py-emacs-features)
(defvar py-serial-number 0)
(defvar py-exception-buffer nil)
(defconst py-output-buffer "*Python Output*")
(make-variable-buffer-local 'py-output-buffer)

;; for toggling between CPython and JPython
(defvar py-which-shell nil)
(defvar py-which-args  py-python-command-args)
(defvar py-which-bufname "Python")
(make-variable-buffer-local 'py-which-shell)
(make-variable-buffer-local 'py-which-args)
(make-variable-buffer-local 'py-which-bufname)

(defun py-toggle-shells (arg)
  "Toggles between the CPython and JPython shells.

With positive argument ARG (interactively \\[universal-argument]),
uses the CPython shell, with negative ARG uses the JPython shell, and
with a zero argument, toggles the shell.

Programmatically, ARG can also be one of the symbols `cpython' or
`jpython', equivalent to positive arg and negative arg respectively."
  (interactive "P")
  ;; default is to toggle
  (if (null arg)
      (setq arg 0))
  ;; preprocess arg
  (cond
   ((equal arg 0)
    ;; toggle
    (if (string-equal py-which-bufname "Python")
	(setq arg -1)
      (setq arg 1)))
   ((equal arg 'cpython) (setq arg 1))
   ((equal arg 'jpython) (setq arg -1)))
  (let (msg)
    (cond
     ((< 0 arg)
      ;; set to CPython
      (setq py-which-shell py-python-command
	    py-which-args py-python-command-args
	    py-which-bufname "Python"
	    msg "CPython"
	    mode-name "Python"))
     ((> 0 arg)
      (setq py-which-shell py-jpython-command
	    py-which-args py-jpython-command-args
	    py-which-bufname "JPython"
	    msg "JPython"
	    mode-name "JPython"))
     )
    (message "Using the %s shell" msg)
    (setq py-output-buffer (format "*%s Output*" py-which-bufname))))

;;;###autoload
(defun py-shell (&optional argprompt)
  "Start an interactive Python interpreter in another window.
This is like Shell mode, except that Python is running in the window
instead of a shell.  See the `Interactive Shell' and `Shell Mode'
sections of the Emacs manual for details, especially for the key
bindings active in the `*Python*' buffer.

With optional \\[universal-argument], the user is prompted for the
flags to pass to the Python interpreter.  This has no effect when this
command is used to switch to an existing process, only when a new
process is started.  If you use this, you will probably want to ensure
that the current arguments are retained (they will be included in the
prompt).  This argument is ignored when this function is called
programmatically, or when running in Emacs 19.34 or older.

Note: You can toggle between using the CPython interpreter and the
JPython interpreter by hitting \\[py-toggle-shells].  This toggles
buffer local variables which control whether all your subshell
interactions happen to the `*JPython*' or `*Python*' buffers (the
latter is the name used for the CPython buffer).

Warning: Don't use an interactive Python if you change sys.ps1 or
sys.ps2 from their default values, or if you're running code that
prints `>>> ' or `... ' at the start of a line.  `python-mode' can't
distinguish your output from Python's output, and assumes that `>>> '
at the start of a line is a prompt from Python.  Similarly, the Emacs
Shell mode code assumes that both `>>> ' and `... ' at the start of a
line are Python prompts.  Bad things can happen if you fool either
mode.

Warning:  If you do any editing *in* the process buffer *while* the
buffer is accepting output from Python, do NOT attempt to `undo' the
changes.  Some of the output (nowhere near the parts you changed!) may
be lost if you do.  This appears to be an Emacs bug, an unfortunate
interaction between undo and process filters; the same problem exists in
non-Python process buffers using the default (Emacs-supplied) process
filter."
  (interactive "P")
  ;; Set the default shell if not already set
  (when (null py-which-shell)
    (py-toggle-shells py-default-interpreter))
  (let ((args py-which-args))
    (when (and argprompt
	       (interactive-p)
	       (fboundp 'split-string))
      ;; TBD: Perhaps force "-i" in the final list?
      (setq args (split-string
		  (read-string (concat py-which-bufname
				       " arguments: ")
			       (concat
				(mapconcat 'identity py-which-args " ") " ")
			       ))))
    (switch-to-buffer-other-window
     (apply 'make-comint py-which-bufname py-which-shell nil args))
    (make-local-variable 'comint-prompt-regexp)
    (setq comint-prompt-regexp "^>>> \\|^[.][.][.] \\|^(pdb) ")
    (add-hook 'comint-output-filter-functions
	      'py-comint-output-filter-function)
    ;; pdbtrack
    (add-hook 'comint-output-filter-functions 'py-pdbtrack-track-stack-file)
    (setq py-pdbtrack-do-tracking-p t)
    (set-syntax-table py-mode-syntax-table)
    (use-local-map py-shell-map)
    (run-hooks 'py-shell-hook)
    ))

(defun py-clear-queue ()
  "Clear the queue of temporary files waiting to execute."
  (interactive)
  (let ((n (length py-file-queue)))
    (mapcar 'delete-file py-file-queue)
    (setq py-file-queue nil)
    (message "%d pending files de-queued." n)))


(defun py-execute-region (start end &optional async)
  "Execute the region in a Python interpreter.

The region is first copied into a temporary file (in the directory
`py-temp-directory').  If there is no Python interpreter shell
running, this file is executed synchronously using
`shell-command-on-region'.  If the program is long running, use
\\[universal-argument] to run the command asynchronously in its own
buffer.

When this function is used programmatically, arguments START and END
specify the region to execute, and optional third argument ASYNC, if
non-nil, specifies to run the command asynchronously in its own
buffer.

If the Python interpreter shell is running, the region is exec()'d
in that shell.  If you try to execute regions too quickly,
`python-mode' will queue them up and execute them one at a time when
it sees a `>>> ' prompt from Python.  Each time this happens, the
process buffer is popped into a window (if it's not already in some
window) so you can see it, and a comment of the form

    \t## working on region in file <name>...

is inserted at the end.  See also the command `py-clear-queue'."
  (interactive "r\nP")
  ;; Skip ahead to the first non-blank line
  (let* ((proc (get-process py-which-bufname))
	 (temp (if (memq 'broken-temp-names py-emacs-features)
		   (let
		       ((sn py-serial-number)
			(pid (and (fboundp 'emacs-pid) (emacs-pid))))
		     (setq py-serial-number (1+ py-serial-number))
		     (if pid
			 (format "python-%d-%d" sn pid)
		       (format "python-%d" sn)))
		 (make-temp-name "python-")))
	 (file (concat (expand-file-name temp py-temp-directory) ".py"))
	 (cur (current-buffer))
	 (buf (get-buffer-create file))
	 shell)
    ;; Write the contents of the buffer, watching out for indented regions.
    (save-excursion
      (goto-char start)
      (beginning-of-line)
      (while (and (looking-at "\\s *$")
		  (< (point) end))
	(forward-line 1))
      (setq start (point))
      (or (< start end)
	  (error "Region is empty"))
      (let ((needs-if (/= (py-point 'bol) (py-point 'boi))))
	(set-buffer buf)
	(python-mode)
	(when needs-if
	  (insert "if 1:\n"))
	(insert-buffer-substring cur start end)
	;; Set the shell either to the #! line command, or to the
	;; py-which-shell buffer local variable.
	(setq shell (or (py-choose-shell-by-shebang)
			(py-choose-shell-by-import)
			py-which-shell))))
    (cond
     ;; always run the code in its own asynchronous subprocess
     (async
      ;; User explicitly wants this to run in its own async subprocess
      (save-excursion
	(set-buffer buf)
	(write-region (point-min) (point-max) file nil 'nomsg))
      (let* ((buf (generate-new-buffer-name py-output-buffer))
	     ;; TBD: a horrible hack, but why create new Custom variables?
	     (arg (if (string-equal py-which-bufname "Python")
		      "-u" "")))
	(start-process py-which-bufname buf shell arg file)
	(pop-to-buffer buf)
	(py-postprocess-output-buffer buf)
	;; TBD: clean up the temporary file!
	))
     ;; if the Python interpreter shell is running, queue it up for
     ;; execution there.
     (proc
      ;; use the existing python shell
      (save-excursion
	(set-buffer buf)
	(write-region (point-min) (point-max) file nil 'nomsg))
      (if (not py-file-queue)
	  (py-execute-file proc file)
	(message "File %s queued for execution" file))
      (setq py-file-queue (append py-file-queue (list file)))
      (setq py-exception-buffer (cons file (current-buffer))))
     (t
      ;; TBD: a horrible hack, but why create new Custom variables?
      (let ((cmd (concat shell (if (string-equal py-which-bufname "JPython")
				   " -" ""))))
	;; otherwise either run it synchronously in a subprocess
	(save-excursion
	  (set-buffer buf)
	  (shell-command-on-region (point-min) (point-max)
				   cmd py-output-buffer))
	;; shell-command-on-region kills the output buffer if it never
	;; existed and there's no output from the command
	(if (not (get-buffer py-output-buffer))
	    (message "No output.")
	  (setq py-exception-buffer (current-buffer))
	  (let ((err-p (py-postprocess-output-buffer py-output-buffer)))
	    (pop-to-buffer py-output-buffer)
	    (if err-p
		(pop-to-buffer py-exception-buffer)))
	  ))
      ))
    ;; Clean up after ourselves.
    (kill-buffer buf)))


;; Code execution commands
(defun py-execute-buffer (&optional async)
  "Send the contents of the buffer to a Python interpreter.
If the file local variable `py-master-file' is non-nil, execute the
named file instead of the buffer's file.

If there is a *Python* process buffer it is used.  If a clipping
restriction is in effect, only the accessible portion of the buffer is
sent.  A trailing newline will be supplied if needed.

See the `\\[py-execute-region]' docs for an account of some
subtleties, including the use of the optional ASYNC argument."
  (interactive "P")
  (if py-master-file
      (let* ((filename (expand-file-name py-master-file))
	     (buffer (or (get-file-buffer filename)
			 (find-file-noselect filename))))
	(set-buffer buffer)))
  (py-execute-region (point-min) (point-max) async))

(defun py-execute-import-or-reload (&optional async)
  "Import the current buffer's file in a Python interpreter.

If the file has already been imported, then do reload instead to get
the latest version.

If the file's name does not end in \".py\", then do exec instead.

If the current buffer is not visiting a file, do `py-execute-buffer'
instead.

If the file local variable `py-master-file' is non-nil, import or
reload the named file instead of the buffer's file.  The file may be
saved based on the value of `py-execute-import-or-reload-save-p'.

See the `\\[py-execute-region]' docs for an account of some
subtleties, including the use of the optional ASYNC argument.

This may be preferable to `\\[py-execute-buffer]' because:

 - Definitions stay in their module rather than appearing at top
   level, where they would clutter the global namespace and not affect
   uses of qualified names (MODULE.NAME).

 - The Python debugger gets line number information about the functions."
  (interactive "P")
  ;; Check file local variable py-master-file
  (if py-master-file
      (let* ((filename (expand-file-name py-master-file))
             (buffer (or (get-file-buffer filename)
                         (find-file-noselect filename))))
        (set-buffer buffer)))
  (let ((file (buffer-file-name (current-buffer))))
    (if file
        (progn
	  ;; Maybe save some buffers
	  (save-some-buffers (not py-ask-about-save) nil)
          (py-execute-string
           (if (string-match "\\.py$" file)
               (let ((f (file-name-sans-extension
			 (file-name-nondirectory file))))
                 (format "if globals().has_key('%s'):\n    reload(%s)\nelse:\n    import %s\n"
                         f f f))
             (format "exec(open(r'%s'))\n" file))
           async))
      ;; else
      (py-execute-buffer async))))


(defun py-execute-def-or-class (&optional async)
  "Send the current function or class definition to a Python interpreter.

If there is a *Python* process buffer it is used.

See the `\\[py-execute-region]' docs for an account of some
subtleties, including the use of the optional ASYNC argument."
  (interactive "P")
  (save-excursion
    (py-mark-def-or-class)
    ;; mark is before point
    (py-execute-region (mark) (point) async)))


(defun py-execute-string (string &optional async)
  "Send the argument STRING to a Python interpreter.

If there is a *Python* process buffer it is used.

See the `\\[py-execute-region]' docs for an account of some
subtleties, including the use of the optional ASYNC argument."
  (interactive "sExecute Python command: ")
  (save-excursion
    (set-buffer (get-buffer-create
                 (generate-new-buffer-name " *Python Command*")))
    (insert string)
    (py-execute-region (point-min) (point-max) async)))



(defun py-jump-to-exception (file line)
  "Jump to the Python code in FILE at LINE."
  (let ((buffer (cond ((string-equal file "<stdin>")
		       (if (consp py-exception-buffer)
			   (cdr py-exception-buffer)
			 py-exception-buffer))
		      ((and (consp py-exception-buffer)
			    (string-equal file (car py-exception-buffer)))
		       (cdr py-exception-buffer))
		      ((py-safe (find-file-noselect file)))
		      ;; could not figure out what file the exception
		      ;; is pointing to, so prompt for it
		      (t (find-file (read-file-name "Exception file: "
						    nil
						    file t))))))
    (pop-to-buffer buffer)
    ;; Force Python mode
    (if (not (eq major-mode 'python-mode))
	(python-mode))
    (goto-line line)
    (message "Jumping to exception in file %s on line %d" file line)))

(defun py-mouseto-exception (event)
  "Jump to the code which caused the Python exception at EVENT.
EVENT is usually a mouse click."
  (interactive "e")
  (cond
   ((fboundp 'event-point)
    ;; XEmacs
    (let* ((point (event-point event))
	   (buffer (event-buffer event))
	   (e (and point buffer (extent-at point buffer 'py-exc-info)))
	   (info (and e (extent-property e 'py-exc-info))))
      (message "Event point: %d, info: %s" point info)
      (and info
	   (py-jump-to-exception (car info) (cdr info)))
      ))
   ;; Emacs -- Please port this!
   ))

(defun py-goto-exception ()
  "Go to the line indicated by the traceback."
  (interactive)
  (let (file line)
    (save-excursion
      (beginning-of-line)
      (if (looking-at py-traceback-line-re)
	  (setq file (match-string 1)
		line (string-to-int (match-string 2)))))
    (if (not file)
	(error "Not on a traceback line"))
    (py-jump-to-exception file line)))

(defun py-find-next-exception (start buffer searchdir errwhere)
  "Find the next Python exception and jump to the code that caused it.
START is the buffer position in BUFFER from which to begin searching
for an exception.  SEARCHDIR is a function, either
`re-search-backward' or `re-search-forward' indicating the direction
to search.  ERRWHERE is used in an error message if the limit (top or
bottom) of the trackback stack is encountered."
  (let (file line)
    (save-excursion
      (set-buffer buffer)
      (goto-char (py-point start))
      (if (funcall searchdir py-traceback-line-re nil t)
	  (setq file (match-string 1)
		line (string-to-int (match-string 2)))))
    (if (and file line)
	(py-jump-to-exception file line)
      (error "%s of traceback" errwhere))))

(defun py-down-exception (&optional bottom)
  "Go to the next line down in the traceback.
With \\[univeral-argument] (programmatically, optional argument
BOTTOM), jump to the bottom (innermost) exception in the exception
stack."
  (interactive "P")
  (let* ((proc (get-process "Python"))
	 (buffer (if proc "*Python*" py-output-buffer)))
    (if bottom
	(py-find-next-exception 'eob buffer 're-search-backward "Bottom")
      (py-find-next-exception 'eol buffer 're-search-forward "Bottom"))))

(defun py-up-exception (&optional top)
  "Go to the previous line up in the traceback.
With \\[universal-argument] (programmatically, optional argument TOP)
jump to the top (outermost) exception in the exception stack."
  (interactive "P")
  (let* ((proc (get-process "Python"))
	 (buffer (if proc "*Python*" py-output-buffer)))
    (if top
	(py-find-next-exception 'bob buffer 're-search-forward "Top")
      (py-find-next-exception 'bol buffer 're-search-backward "Top"))))


;; Electric deletion
(defun py-electric-backspace (arg)
  "Delete preceding character or levels of indentation.
Deletion is performed by calling the function in `py-backspace-function'
with a single argument (the number of characters to delete).

If point is at the leftmost column, delete the preceding newline.

Otherwise, if point is at the leftmost non-whitespace character of a
line that is neither a continuation line nor a non-indenting comment
line, or if point is at the end of a blank line, this command reduces
the indentation to match that of the line that opened the current
block of code.  The line that opened the block is displayed in the
echo area to help you keep track of where you are.  With
\\[universal-argument] dedents that many blocks (but not past column
zero).

Otherwise the preceding character is deleted, converting a tab to
spaces if needed so that only a single column position is deleted.
\\[universal-argument] specifies how many characters to delete;
default is 1.

When used programmatically, argument ARG specifies the number of
blocks to dedent, or the number of characters to delete, as indicated
above."
  (interactive "*p")
  (if (or (/= (current-indentation) (current-column))
	  (bolp)
	  (py-continuation-line-p)
;	  (not py-honor-comment-indentation)
;	  (looking-at "#[^ \t\n]")	; non-indenting #
	  )
      (funcall py-backspace-function arg)
    ;; else indent the same as the colon line that opened the block
    ;; force non-blank so py-goto-block-up doesn't ignore it
    (insert-char ?* 1)
    (backward-char)
    (let ((base-indent 0)		; indentation of base line
	  (base-text "")		; and text of base line
	  (base-found-p nil))
      (save-excursion
	(while (< 0 arg)
	  (condition-case nil		; in case no enclosing block
	      (progn
		(py-goto-block-up 'no-mark)
		(setq base-indent (current-indentation)
		      base-text   (py-suck-up-leading-text)
		      base-found-p t))
	    (error nil))
	  (setq arg (1- arg))))
      (delete-char 1)			; toss the dummy character
      (delete-horizontal-space)
      (indent-to base-indent)
      (if base-found-p
	  (message "Closes block: %s" base-text)))))


(defun py-electric-delete (arg)
  "Delete preceding or following character or levels of whitespace.

The behavior of this function depends on the variable
`delete-key-deletes-forward'.  If this variable is nil (or does not
exist, as in older Emacsen and non-XEmacs versions), then this
function behaves identically to \\[c-electric-backspace].

If `delete-key-deletes-forward' is non-nil and is supported in your
Emacs, then deletion occurs in the forward direction, by calling the
function in `py-delete-function'.

\\[universal-argument] (programmatically, argument ARG) specifies the
number of characters to delete (default is 1)."
  (interactive "*p")
  (if (or (and (fboundp 'delete-forward-p) ;XEmacs 21
	       (delete-forward-p))
	  (and (boundp 'delete-key-deletes-forward) ;XEmacs 20
	       delete-key-deletes-forward))
      (funcall py-delete-function arg)
    (py-electric-backspace arg)))

;; required for pending-del and delsel modes
(put 'py-electric-colon 'delete-selection t) ;delsel
(put 'py-electric-colon 'pending-delete   t) ;pending-del
(put 'py-electric-backspace 'delete-selection 'supersede) ;delsel
(put 'py-electric-backspace 'pending-delete   'supersede) ;pending-del
(put 'py-electric-delete    'delete-selection 'supersede) ;delsel
(put 'py-electric-delete    'pending-delete   'supersede) ;pending-del



(defun py-indent-line (&optional arg)
  "Fix the indentation of the current line according to Python rules.
With \\[universal-argument] (programmatically, the optional argument
ARG non-nil), ignore dedenting rules for block closing statements
(e.g. return, raise, break, continue, pass)

This function is normally bound to `indent-line-function' so
\\[indent-for-tab-command] will call it."
  (interactive "P")
  (let* ((ci (current-indentation))
	 (move-to-indentation-p (<= (current-column) ci))
	 (need (py-compute-indentation (not arg))))
    ;; see if we need to dedent
    (if (py-outdent-p)
	(setq need (- need py-indent-offset)))
    (if (/= ci need)
	(save-excursion
	  (beginning-of-line)
	  (delete-horizontal-space)
	  (indent-to need)))
    (if move-to-indentation-p (back-to-indentation))))

(defun py-newline-and-indent ()
  "Strives to act like the Emacs `newline-and-indent'.
This is just `strives to' because correct indentation can't be computed
from scratch for Python code.  In general, deletes the whitespace before
point, inserts a newline, and takes an educated guess as to how you want
the new line indented."
  (interactive)
  (let ((ci (current-indentation)))
    (if (< ci (current-column))		; if point beyond indentation
	(newline-and-indent)
      ;; else try to act like newline-and-indent "normally" acts
      (beginning-of-line)
      (insert-char ?\n 1)
      (move-to-column ci))))

(defun py-compute-indentation (honor-block-close-p)
  "Compute Python indentation.
When HONOR-BLOCK-CLOSE-P is non-nil, statements such as `return',
`raise', `break', `continue', and `pass' force one level of
dedenting."
  (save-excursion
    (beginning-of-line)
    (let* ((bod (py-point 'bod))
	   (pps (parse-partial-sexp bod (point)))
	   (boipps (parse-partial-sexp bod (py-point 'boi)))
	   placeholder)
      (cond
       ;; are we inside a multi-line string or comment?
       ((or (and (nth 3 pps) (nth 3 boipps))
	    (and (nth 4 pps) (nth 4 boipps)))
	(save-excursion
	  (if (not py-align-multiline-strings-p) 0
	    ;; skip back over blank & non-indenting comment lines
	    ;; note: will skip a blank or non-indenting comment line
	    ;; that happens to be a continuation line too
	    (re-search-backward "^[ \t]*\\([^ \t\n#]\\|#[ \t\n]\\)" nil 'move)
	    (back-to-indentation)
	    (current-column))))
       ;; are we on a continuation line?
       ((py-continuation-line-p)
	(let ((startpos (point))
	      (open-bracket-pos (py-nesting-level))
	      endpos searching found state)
	  (if open-bracket-pos
	      (progn
		;; align with first item in list; else a normal
		;; indent beyond the line with the open bracket
		(goto-char (1+ open-bracket-pos)) ; just beyond bracket
		;; is the first list item on the same line?
		(skip-chars-forward " \t")
		(if (null (memq (following-char) '(?\n ?# ?\\)))
					; yes, so line up with it
		    (current-column)
		  ;; first list item on another line, or doesn't exist yet
		  (forward-line 1)
		  (while (and (< (point) startpos)
			      (looking-at "[ \t]*[#\n\\\\]")) ; skip noise
		    (forward-line 1))
		  (if (and (< (point) startpos)
			   (/= startpos
			       (save-excursion
				 (goto-char (1+ open-bracket-pos))
				 (forward-comment (point-max))
				 (point))))
		      ;; again mimic the first list item
		      (current-indentation)
		    ;; else they're about to enter the first item
		    (goto-char open-bracket-pos)
		    (setq placeholder (point))
		    (py-goto-initial-line)
		    (py-goto-beginning-of-tqs
		     (save-excursion (nth 3 (parse-partial-sexp
					     placeholder (point)))))
		    (+ (current-indentation) py-indent-offset))))

	    ;; else on backslash continuation line
	    (forward-line -1)
	    (if (py-continuation-line-p) ; on at least 3rd line in block
		(current-indentation)	; so just continue the pattern
	      ;; else started on 2nd line in block, so indent more.
	      ;; if base line is an assignment with a start on a RHS,
	      ;; indent to 2 beyond the leftmost "="; else skip first
	      ;; chunk of non-whitespace characters on base line, + 1 more
	      ;; column
	      (end-of-line)
	      (setq endpos (point)
		    searching t)
	      (back-to-indentation)
	      (setq startpos (point))
	      ;; look at all "=" from left to right, stopping at first
	      ;; one not nested in a list or string
	      (while searching
		(skip-chars-forward "^=" endpos)
		(if (= (point) endpos)
		    (setq searching nil)
		  (forward-char 1)
		  (setq state (parse-partial-sexp startpos (point)))
		  (if (and (zerop (car state)) ; not in a bracket
			   (null (nth 3 state))) ; & not in a string
		      (progn
			(setq searching nil) ; done searching in any case
			(setq found
			      (not (or
				    (eq (following-char) ?=)
				    (memq (char-after (- (point) 2))
					  '(?< ?> ?!)))))))))
	      (if (or (not found)	; not an assignment
		      (looking-at "[ \t]*\\\\")) ; <=><spaces><backslash>
		  (progn
		    (goto-char startpos)
		    (skip-chars-forward "^ \t\n")))
	      ;; if this is a continuation for a block opening
	      ;; statement, add some extra offset.
	      (+ (current-column) (if (py-statement-opens-block-p)
				      py-continuation-offset 0)
		 1)
	      ))))

       ;; not on a continuation line
       ((bobp) (current-indentation))

       ;; Dfn: "Indenting comment line".  A line containing only a
       ;; comment, but which is treated like a statement for
       ;; indentation calculation purposes.  Such lines are only
       ;; treated specially by the mode; they are not treated
       ;; specially by the Python interpreter.

       ;; The rules for indenting comment lines are a line where:
       ;;   - the first non-whitespace character is `#', and
       ;;   - the character following the `#' is whitespace, and
       ;;   - the line is dedented with respect to (i.e. to the left
       ;;     of) the indentation of the preceding non-blank line.

       ;; The first non-blank line following an indenting comment
       ;; line is given the same amount of indentation as the
       ;; indenting comment line.

       ;; All other comment-only lines are ignored for indentation
       ;; purposes.

       ;; Are we looking at a comment-only line which is *not* an
       ;; indenting comment line?  If so, we assume that it's been
       ;; placed at the desired indentation, so leave it alone.
       ;; Indenting comment lines are aligned as statements down
       ;; below.
       ((and (looking-at "[ \t]*#[^ \t\n]")
	     ;; NOTE: this test will not be performed in older Emacsen
	     (fboundp 'forward-comment)
	     (<= (current-indentation)
		 (save-excursion
		   (forward-comment (- (point-max)))
		   (current-indentation))))
	(current-indentation))

       ;; else indentation based on that of the statement that
       ;; precedes us; use the first line of that statement to
       ;; establish the base, in case the user forced a non-std
       ;; indentation for the continuation lines (if any)
       (t
	;; skip back over blank & non-indenting comment lines note:
	;; will skip a blank or non-indenting comment line that
	;; happens to be a continuation line too.  use fast Emacs 19
	;; function if it's there.
	(if (and (eq py-honor-comment-indentation nil)
		 (fboundp 'forward-comment))
	    (forward-comment (- (point-max)))
	  (let ((prefix-re (concat py-block-comment-prefix "[ \t]*"))
		done)
	    (while (not done)
	      (re-search-backward "^[ \t]*\\([^ \t\n#]\\|#\\)" nil 'move)
	      (setq done (or (bobp)
			     (and (eq py-honor-comment-indentation t)
				  (save-excursion
				    (back-to-indentation)
				    (not (looking-at prefix-re))
				    ))
			     (and (not (eq py-honor-comment-indentation t))
				  (save-excursion
				    (back-to-indentation)
				    (and (not (looking-at prefix-re))
					 (or (looking-at "[^#]")
					     (not (zerop (current-column)))
					     ))
				    ))
			     ))
	      )))
	;; if we landed inside a string, go to the beginning of that
	;; string. this handles triple quoted, multi-line spanning
	;; strings.
	(py-goto-beginning-of-tqs (nth 3 (parse-partial-sexp bod (point))))
	;; now skip backward over continued lines
	(setq placeholder (point))
	(py-goto-initial-line)
	;; we may *now* have landed in a TQS, so find the beginning of
	;; this string.
	(py-goto-beginning-of-tqs
	 (save-excursion (nth 3 (parse-partial-sexp
				 placeholder (point)))))
	(+ (current-indentation)
	   (if (py-statement-opens-block-p)
	       py-indent-offset
	     (if (and honor-block-close-p (py-statement-closes-block-p))
		 (- py-indent-offset)
	       0)))
	)))))

(defun py-guess-indent-offset (&optional global)
  "Guess a good value for, and change, `py-indent-offset'.

By default, make a buffer-local copy of `py-indent-offset' with the
new value, so that other Python buffers are not affected.  With
\\[universal-argument] (programmatically, optional argument GLOBAL),
change the global value of `py-indent-offset'.  This affects all
Python buffers (that don't have their own buffer-local copy), both
those currently existing and those created later in the Emacs session.

Some people use a different value for `py-indent-offset' than you use.
There's no excuse for such foolishness, but sometimes you have to deal
with their ugly code anyway.  This function examines the file and sets
`py-indent-offset' to what it thinks it was when they created the
mess.

Specifically, it searches forward from the statement containing point,
looking for a line that opens a block of code.  `py-indent-offset' is
set to the difference in indentation between that line and the Python
statement following it.  If the search doesn't succeed going forward,
it's tried again going backward."
  (interactive "P")			; raw prefix arg
  (let (new-value
	(start (point))
	(restart (point))
	(found nil)
	colon-indent)
    (py-goto-initial-line)
    (while (not (or found (eobp)))
      (when (and (re-search-forward ":[ \t]*\\($\\|[#\\]\\)" nil 'move)
		 (not (py-in-literal restart)))
	(setq restart (point))
	(py-goto-initial-line)
	(if (py-statement-opens-block-p)
	    (setq found t)
	  (goto-char restart))))
    (unless found
      (goto-char start)
      (py-goto-initial-line)
      (while (not (or found (bobp)))
	(setq found (and
		     (re-search-backward ":[ \t]*\\($\\|[#\\]\\)" nil 'move)
		     (or (py-goto-initial-line) t) ; always true -- side effect
		     (py-statement-opens-block-p)))))
    (setq colon-indent (current-indentation)
	  found (and found (zerop (py-next-statement 1)))
	  new-value (- (current-indentation) colon-indent))
    (goto-char start)
    (if (not found)
	(error "Sorry, couldn't guess a value for py-indent-offset")
      (funcall (if global 'kill-local-variable 'make-local-variable)
	       'py-indent-offset)
      (setq py-indent-offset new-value)
      (or noninteractive
	  (message "%s value of py-indent-offset set to %d"
		   (if global "Global" "Local")
		   py-indent-offset)))
    ))

(defun py-comment-indent-function ()
  "Python version of `comment-indent-function'."
  ;; This is required when filladapt is turned off.  Without it, when
  ;; filladapt is not used, comments which start in column zero
  ;; cascade one character to the right
  (save-excursion
    (beginning-of-line)
    (let ((eol (py-point 'eol)))
      (and comment-start-skip
	   (re-search-forward comment-start-skip eol t)
	   (setq eol (match-beginning 0)))
      (goto-char eol)
      (skip-chars-backward " \t")
      (max comment-column (+ (current-column) (if (bolp) 0 1)))
      )))

(defun py-narrow-to-defun (&optional class)
  "Make text outside current defun invisible.
The defun visible is the one that contains point or follows point.
Optional CLASS is passed directly to `py-beginning-of-def-or-class'."
  (interactive "P")
  (save-excursion
    (widen)
    (py-end-of-def-or-class class)
    (let ((end (point)))
      (py-beginning-of-def-or-class class)
      (narrow-to-region (point) end))))


(defun py-shift-region (start end count)
  "Indent lines from START to END by COUNT spaces."
  (save-excursion
    (goto-char end)
    (beginning-of-line)
    (setq end (point))
    (goto-char start)
    (beginning-of-line)
    (setq start (point))
    (indent-rigidly start end count)))

(defun py-shift-region-left (start end &optional count)
  "Shift region of Python code to the left.
The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
shifted to the left, by `py-indent-offset' columns.

If a prefix argument is given, the region is instead shifted by that
many columns.  With no active region, dedent only the current line.
You cannot dedent the region if any line is already at column zero."
  (interactive
   (let ((p (point))
	 (m (mark))
	 (arg current-prefix-arg))
     (if m
	 (list (min p m) (max p m) arg)
       (list p (save-excursion (forward-line 1) (point)) arg))))
  ;; if any line is at column zero, don't shift the region
  (save-excursion
    (goto-char start)
    (while (< (point) end)
      (back-to-indentation)
      (if (and (zerop (current-column))
	       (not (looking-at "\\s *$")))
	  (error "Region is at left edge"))
      (forward-line 1)))
  (py-shift-region start end (- (prefix-numeric-value
				 (or count py-indent-offset))))
  (py-keep-region-active))

(defun py-shift-region-right (start end &optional count)
  "Shift region of Python code to the right.
The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
shifted to the right, by `py-indent-offset' columns.

If a prefix argument is given, the region is instead shifted by that
many columns.  With no active region, indent only the current line."
  (interactive
   (let ((p (point))
	 (m (mark))
	 (arg current-prefix-arg))
     (if m
	 (list (min p m) (max p m) arg)
       (list p (save-excursion (forward-line 1) (point)) arg))))
  (py-shift-region start end (prefix-numeric-value
			      (or count py-indent-offset)))
  (py-keep-region-active))

(defun py-indent-region (start end &optional indent-offset)
  "Reindent a region of Python code.

The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
reindented.  If the first line of the region has a non-whitespace
character in the first column, the first line is left alone and the
rest of the region is reindented with respect to it.  Else the entire
region is reindented with respect to the (closest code or indenting
comment) statement immediately preceding the region.

This is useful when code blocks are moved or yanked, when enclosing
control structures are introduced or removed, or to reformat code
using a new value for the indentation offset.

If a numeric prefix argument is given, it will be used as the value of
the indentation offset.  Else the value of `py-indent-offset' will be
used.

Warning: The region must be consistently indented before this function
is called!  This function does not compute proper indentation from
scratch (that's impossible in Python), it merely adjusts the existing
indentation to be correct in context.

Warning: This function really has no idea what to do with
non-indenting comment lines, and shifts them as if they were indenting
comment lines.  Fixing this appears to require telepathy.

Special cases: whitespace is deleted from blank lines; continuation
lines are shifted by the same amount their initial line was shifted,
in order to preserve their relative indentation with respect to their
initial line; and comment lines beginning in column 1 are ignored."
  (interactive "*r\nP")			; region; raw prefix arg
  (save-excursion
    (goto-char end)   (beginning-of-line) (setq end (point-marker))
    (goto-char start) (beginning-of-line)
    (let ((py-indent-offset (prefix-numeric-value
			     (or indent-offset py-indent-offset)))
	  (indents '(-1))		; stack of active indent levels
	  (target-column 0)		; column to which to indent
	  (base-shifted-by 0)		; amount last base line was shifted
	  (indent-base (if (looking-at "[ \t\n]")
			   (py-compute-indentation t)
			 0))
	  ci)
      (while (< (point) end)
	(setq ci (current-indentation))
	;; figure out appropriate target column
	(cond
	 ((or (eq (following-char) ?#)	; comment in column 1
	      (looking-at "[ \t]*$"))	; entirely blank
	  (setq target-column 0))
	 ((py-continuation-line-p)	; shift relative to base line
	  (setq target-column (+ ci base-shifted-by)))
	 (t				; new base line
	  (if (> ci (car indents))	; going deeper; push it
	      (setq indents (cons ci indents))
	    ;; else we should have seen this indent before
	    (setq indents (memq ci indents)) ; pop deeper indents
	    (if (null indents)
		(error "Bad indentation in region, at line %d"
		       (save-restriction
			 (widen)
			 (1+ (count-lines 1 (point)))))))
	  (setq target-column (+ indent-base
				 (* py-indent-offset
				    (- (length indents) 2))))
	  (setq base-shifted-by (- target-column ci))))
	;; shift as needed
	(if (/= ci target-column)
	    (progn
	      (delete-horizontal-space)
	      (indent-to target-column)))
	(forward-line 1))))
  (set-marker end nil))

(defun py-comment-region (beg end &optional arg)
  "Like `comment-region' but uses double hash (`#') comment starter."
  (interactive "r\nP")
  (let ((comment-start py-block-comment-prefix))
    (comment-region beg end arg)))


;; Functions for moving point
(defun py-previous-statement (count)
  "Go to the start of the COUNTth preceding Python statement.
By default, goes to the previous statement.  If there is no such
statement, goes to the first statement.  Return count of statements
left to move.  `Statements' do not include blank, comment, or
continuation lines."
  (interactive "p")			; numeric prefix arg
  (if (< count 0) (py-next-statement (- count))
    (py-goto-initial-line)
    (let (start)
      (while (and
	      (setq start (point))	; always true -- side effect
	      (> count 0)
	      (zerop (forward-line -1))
	      (py-goto-statement-at-or-above))
	(setq count (1- count)))
      (if (> count 0) (goto-char start)))
    count))

(defun py-next-statement (count)
  "Go to the start of next Python statement.
If the statement at point is the i'th Python statement, goes to the
start of statement i+COUNT.  If there is no such statement, goes to the
last statement.  Returns count of statements left to move.  `Statements'
do not include blank, comment, or continuation lines."
  (interactive "p")			; numeric prefix arg
  (if (< count 0) (py-previous-statement (- count))
    (beginning-of-line)
    (let (start)
      (while (and
	      (setq start (point))	; always true -- side effect
	      (> count 0)
	      (py-goto-statement-below))
	(setq count (1- count)))
      (if (> count 0) (goto-char start)))
    count))

(defun py-goto-block-up (&optional nomark)
  "Move up to start of current block.
Go to the statement that starts the smallest enclosing block; roughly
speaking, this will be the closest preceding statement that ends with a
colon and is indented less than the statement you started on.  If
successful, also sets the mark to the starting point.

`\\[py-mark-block]' can be used afterward to mark the whole code
block, if desired.

If called from a program, the mark will not be set if optional argument
NOMARK is not nil."
  (interactive)
  (let ((start (point))
	(found nil)
	initial-indent)
    (py-goto-initial-line)
    ;; if on blank or non-indenting comment line, use the preceding stmt
    (if (looking-at "[ \t]*\\($\\|#[^ \t\n]\\)")
	(progn
	  (py-goto-statement-at-or-above)
	  (setq found (py-statement-opens-block-p))))
    ;; search back for colon line indented less
    (setq initial-indent (current-indentation))
    (if (zerop initial-indent)
	;; force fast exit
	(goto-char (point-min)))
    (while (not (or found (bobp)))
      (setq found
	    (and
	     (re-search-backward ":[ \t]*\\($\\|[#\\]\\)" nil 'move)
	     (or (py-goto-initial-line) t) ; always true -- side effect
	     (< (current-indentation) initial-indent)
	     (py-statement-opens-block-p))))
    (if found
	(progn
	  (or nomark (push-mark start))
	  (back-to-indentation))
      (goto-char start)
      (error "Enclosing block not found"))))

(defun py-beginning-of-def-or-class (&optional class count)
  "Move point to start of `def' or `class'.

Searches back for the closest preceding `def'.  If you supply a prefix
arg, looks for a `class' instead.  The docs below assume the `def'
case; just substitute `class' for `def' for the other case.
Programmatically, if CLASS is `either', then moves to either `class'
or `def'.

When second optional argument is given programmatically, move to the
COUNTth start of `def'.

If point is in a `def' statement already, and after the `d', simply
moves point to the start of the statement.

Otherwise (i.e. when point is not in a `def' statement, or at or
before the `d' of a `def' statement), searches for the closest
preceding `def' statement, and leaves point at its start.  If no such
statement can be found, leaves point at the start of the buffer.

Returns t iff a `def' statement is found by these rules.

Note that doing this command repeatedly will take you closer to the
start of the buffer each time.

To mark the current `def', see `\\[py-mark-def-or-class]'."
  (interactive "P")			; raw prefix arg
  (setq count (or count 1))
  (let ((at-or-before-p (<= (current-column) (current-indentation)))
	(start-of-line (goto-char (py-point 'bol)))
	(start-of-stmt (goto-char (py-point 'bos)))
	(start-re (cond ((eq class 'either) "^[ \t]*\\(class\\|def\\)\\>")
			(class "^[ \t]*class\\>")
			(t "^[ \t]*def\\>")))
	)
    ;; searching backward
    (if (and (< 0 count)
	     (or (/= start-of-stmt start-of-line)
		 (not at-or-before-p)))
	(end-of-line))
    ;; search forward
    (if (and (> 0 count)
	     (zerop (current-column))
	     (looking-at start-re))
	(end-of-line))
    (if (re-search-backward start-re nil 'move count)
	(goto-char (match-beginning 0)))))

;; Backwards compatibility
(defalias 'beginning-of-python-def-or-class 'py-beginning-of-def-or-class)

(defun py-end-of-def-or-class (&optional class count)
  "Move point beyond end of `def' or `class' body.

By default, looks for an appropriate `def'.  If you supply a prefix
arg, looks for a `class' instead.  The docs below assume the `def'
case; just substitute `class' for `def' for the other case.
Programmatically, if CLASS is `either', then moves to either `class'
or `def'.

When second optional argument is given programmatically, move to the
COUNTth end of `def'.

If point is in a `def' statement already, this is the `def' we use.

Else, if the `def' found by `\\[py-beginning-of-def-or-class]'
contains the statement you started on, that's the `def' we use.

Otherwise, we search forward for the closest following `def', and use that.

If a `def' can be found by these rules, point is moved to the start of
the line immediately following the `def' block, and the position of the
start of the `def' is returned.

Else point is moved to the end of the buffer, and nil is returned.

Note that doing this command repeatedly will take you closer to the
end of the buffer each time.

To mark the current `def', see `\\[py-mark-def-or-class]'."
  (interactive "P")			; raw prefix arg
  (if (and count (/= count 1))
      (py-beginning-of-def-or-class (- 1 count)))
  (let ((start (progn (py-goto-initial-line) (point)))
	(which (cond ((eq class 'either) "\\(class\\|def\\)")
		     (class "class")
		     (t "def")))
	(state 'not-found))
    ;; move point to start of appropriate def/class
    (if (looking-at (concat "[ \t]*" which "\\>")) ; already on one
	(setq state 'at-beginning)
      ;; else see if py-beginning-of-def-or-class hits container
      (if (and (py-beginning-of-def-or-class class)
	       (progn (py-goto-beyond-block)
		      (> (point) start)))
	  (setq state 'at-end)
	;; else search forward
	(goto-char start)
	(if (re-search-forward (concat "^[ \t]*" which "\\>") nil 'move)
	    (progn (setq state 'at-beginning)
		   (beginning-of-line)))))
    (cond
     ((eq state 'at-beginning) (py-goto-beyond-block) t)
     ((eq state 'at-end) t)
     ((eq state 'not-found) nil)
     (t (error "Internal error in `py-end-of-def-or-class'")))))

;; Backwards compabitility
(defalias 'end-of-python-def-or-class 'py-end-of-def-or-class)


;; Functions for marking regions
(defun py-mark-block (&optional extend just-move)
  "Mark following block of lines.  With prefix arg, mark structure.
Easier to use than explain.  It sets the region to an `interesting'
block of succeeding lines.  If point is on a blank line, it goes down to
the next non-blank line.  That will be the start of the region.  The end
of the region depends on the kind of line at the start:

 - If a comment, the region will include all succeeding comment lines up
   to (but not including) the next non-comment line (if any).

 - Else if a prefix arg is given, and the line begins one of these
   structures:

     if elif else try except finally for while def class

   the region will be set to the body of the structure, including
   following blocks that `belong' to it, but excluding trailing blank
   and comment lines.  E.g., if on a `try' statement, the `try' block
   and all (if any) of the following `except' and `finally' blocks
   that belong to the `try' structure will be in the region.  Ditto
   for if/elif/else, for/else and while/else structures, and (a bit
   degenerate, since they're always one-block structures) def and
   class blocks.

 - Else if no prefix argument is given, and the line begins a Python
   block (see list above), and the block is not a `one-liner' (i.e.,
   the statement ends with a colon, not with code), the region will
   include all succeeding lines up to (but not including) the next
   code statement (if any) that's indented no more than the starting
   line, except that trailing blank and comment lines are excluded.
   E.g., if the starting line begins a multi-statement `def'
   structure, the region will be set to the full function definition,
   but without any trailing `noise' lines.

 - Else the region will include all succeeding lines up to (but not
   including) the next blank line, or code or indenting-comment line
   indented strictly less than the starting line.  Trailing indenting
   comment lines are included in this case, but not trailing blank
   lines.

A msg identifying the location of the mark is displayed in the echo
area; or do `\\[exchange-point-and-mark]' to flip down to the end.

If called from a program, optional argument EXTEND plays the role of
the prefix arg, and if optional argument JUST-MOVE is not nil, just
moves to the end of the block (& does not set mark or display a msg)."
  (interactive "P")			; raw prefix arg
  (py-goto-initial-line)
  ;; skip over blank lines
  (while (and
	  (looking-at "[ \t]*$")	; while blank line
	  (not (eobp)))			; & somewhere to go
    (forward-line 1))
  (if (eobp)
      (error "Hit end of buffer without finding a non-blank stmt"))
  (let ((initial-pos (point))
	(initial-indent (current-indentation))
	last-pos			; position of last stmt in region
	(followers
	 '((if elif else) (elif elif else) (else)
	   (try except finally) (except except) (finally)
	   (for else) (while else)
	   (def) (class) ) )
	first-symbol next-symbol)

    (cond
     ;; if comment line, suck up the following comment lines
     ((looking-at "[ \t]*#")
      (re-search-forward "^[ \t]*[^ \t#]" nil 'move) ; look for non-comment
      (re-search-backward "^[ \t]*#")	; and back to last comment in block
      (setq last-pos (point)))

     ;; else if line is a block line and EXTEND given, suck up
     ;; the whole structure
     ((and extend
	   (setq first-symbol (py-suck-up-first-keyword) )
	   (assq first-symbol followers))
      (while (and
	      (or (py-goto-beyond-block) t) ; side effect
	      (forward-line -1)		; side effect
	      (setq last-pos (point))	; side effect
	      (py-goto-statement-below)
	      (= (current-indentation) initial-indent)
	      (setq next-symbol (py-suck-up-first-keyword))
	      (memq next-symbol (cdr (assq first-symbol followers))))
	(setq first-symbol next-symbol)))

     ;; else if line *opens* a block, search for next stmt indented <=
     ((py-statement-opens-block-p)
      (while (and
	      (setq last-pos (point))	; always true -- side effect
	      (py-goto-statement-below)
	      (> (current-indentation) initial-indent)
	      )))

     ;; else plain code line; stop at next blank line, or stmt or
     ;; indenting comment line indented <
     (t
      (while (and
	      (setq last-pos (point))	; always true -- side effect
	      (or (py-goto-beyond-final-line) t)
	      (not (looking-at "[ \t]*$")) ; stop at blank line
	      (or
	       (>= (current-indentation) initial-indent)
	       (looking-at "[ \t]*#[^ \t\n]"))) ; ignore non-indenting #
	nil)))

    ;; skip to end of last stmt
    (goto-char last-pos)
    (py-goto-beyond-final-line)

    ;; set mark & display
    (if just-move
	()				; just return
      (push-mark (point) 'no-msg)
      (forward-line -1)
      (message "Mark set after: %s" (py-suck-up-leading-text))
      (goto-char initial-pos))))

(defun py-mark-def-or-class (&optional class)
  "Set region to body of def (or class, with prefix arg) enclosing point.
Pushes the current mark, then point, on the mark ring (all language
modes do this, but although it's handy it's never documented ...).

In most Emacs language modes, this function bears at least a
hallucinogenic resemblance to `\\[py-end-of-def-or-class]' and
`\\[py-beginning-of-def-or-class]'.

And in earlier versions of Python mode, all 3 were tightly connected.
Turned out that was more confusing than useful: the `goto start' and
`goto end' commands are usually used to search through a file, and
people expect them to act a lot like `search backward' and `search
forward' string-search commands.  But because Python `def' and `class'
can nest to arbitrary levels, finding the smallest def containing
point cannot be done via a simple backward search: the def containing
point may not be the closest preceding def, or even the closest
preceding def that's indented less.  The fancy algorithm required is
appropriate for the usual uses of this `mark' command, but not for the
`goto' variations.

So the def marked by this command may not be the one either of the
`goto' commands find: If point is on a blank or non-indenting comment
line, moves back to start of the closest preceding code statement or
indenting comment line.  If this is a `def' statement, that's the def
we use.  Else searches for the smallest enclosing `def' block and uses
that.  Else signals an error.

When an enclosing def is found: The mark is left immediately beyond
the last line of the def block.  Point is left at the start of the
def, except that: if the def is preceded by a number of comment lines
followed by (at most) one optional blank line, point is left at the
start of the comments; else if the def is preceded by a blank line,
point is left at its start.

The intent is to mark the containing def/class and its associated
documentation, to make moving and duplicating functions and classes
pleasant."
  (interactive "P")			; raw prefix arg
  (let ((start (point))
	(which (cond ((eq class 'either) "\\(class\\|def\\)")
		     (class "class")
		     (t "def"))))
    (push-mark start)
    (if (not (py-go-up-tree-to-keyword which))
	(progn (goto-char start)
	       (error "Enclosing %s not found"
		      (if (eq class 'either)
			  "def or class"
			which)))
      ;; else enclosing def/class found
      (setq start (point))
      (py-goto-beyond-block)
      (push-mark (point))
      (goto-char start)
      (if (zerop (forward-line -1))	; if there is a preceding line
	  (progn
	    (if (looking-at "[ \t]*$")	; it's blank
		(setq start (point))	; so reset start point
	      (goto-char start))	; else try again
	    (if (zerop (forward-line -1))
		(if (looking-at "[ \t]*#") ; a comment
		    ;; look back for non-comment line
		    ;; tricky: note that the regexp matches a blank
		    ;; line, cuz \n is in the 2nd character class
		    (and
		     (re-search-backward "^[ \t]*[^ \t#]" nil 'move)
		     (forward-line 1))
		  ;; no comment, so go back
		  (goto-char start)))))))
  (exchange-point-and-mark)
  (py-keep-region-active))

;; ripped from cc-mode
(defun py-forward-into-nomenclature (&optional arg)
  "Move forward to end of a nomenclature section or word.
With \\[universal-argument] (programmatically, optional argument ARG), 
do it that many times.

A `nomenclature' is a fancy way of saying AWordWithMixedCaseNotUnderscores."
  (interactive "p")
  (let ((case-fold-search nil))
    (if (> arg 0)
	(re-search-forward
	 "\\(\\W\\|[_]\\)*\\([A-Z]*[a-z0-9]*\\)"
	 (point-max) t arg)
      (while (and (< arg 0)
		  (re-search-backward
		   "\\(\\W\\|[a-z0-9]\\)[A-Z]+\\|\\(\\W\\|[_]\\)\\w+"
		   (point-min) 0))
	(forward-char 1)
	(setq arg (1+ arg)))))
  (py-keep-region-active))

(defun py-backward-into-nomenclature (&optional arg)
  "Move backward to beginning of a nomenclature section or word.
With optional ARG, move that many times.  If ARG is negative, move
forward.

A `nomenclature' is a fancy way of saying AWordWithMixedCaseNotUnderscores."
  (interactive "p")
  (py-forward-into-nomenclature (- arg))
  (py-keep-region-active))



;; pdbtrack functions
(defun py-pdbtrack-toggle-stack-tracking (arg)
  (interactive "P")
  (if (not (get-buffer-process (current-buffer)))
      (error "No process associated with buffer '%s'" (current-buffer)))
  ;; missing or 0 is toggle, >0 turn on, <0 turn off
  (if (or (not arg)
	  (zerop (setq arg (prefix-numeric-value arg))))
      (setq py-pdbtrack-do-tracking-p (not py-pdbtrack-do-tracking-p))
    (setq py-pdbtrack-do-tracking-p (> arg 0)))
  (message "%sabled Python's pdbtrack"
           (if py-pdbtrack-do-tracking-p "En" "Dis")))

(defun turn-on-pdbtrack ()
  (interactive)
  (py-pdbtrack-toggle-stack-tracking 1))

(defun turn-off-pdbtrack ()
  (interactive)
  (py-pdbtrack-toggle-stack-tracking 0))



;; Pychecker
(defun py-pychecker-run (command)
  "*Run pychecker (default on the file currently visited)."
  (interactive
   (let ((default
           (format "%s %s %s" py-pychecker-command
		   (mapconcat 'identity py-pychecker-command-args " ")
		   (buffer-file-name)))
	 (last (when py-pychecker-history
		 (let* ((lastcmd (car py-pychecker-history))
			(cmd (cdr (reverse (split-string lastcmd))))
			(newcmd (reverse (cons (buffer-file-name) cmd))))
		   (mapconcat 'identity newcmd " ")))))

     (list
      (if (fboundp 'read-shell-command)
	  (read-shell-command "Run pychecker like this: "
			      (if last
				  last
				default)
			      'py-pychecker-history)
	(read-string "Run pychecker like this: "
		     (if last
			 last
		       default)
		     'py-pychecker-history))
	)))
  (save-some-buffers (not py-ask-about-save) nil)
  (compile-internal command "No more errors"))



;; pydoc commands. The guts of this function is stolen from XEmacs's
;; symbol-near-point, but without the useless regexp-quote call on the
;; results, nor the interactive bit.  Also, we've added the temporary
;; syntax table setting, which Skip originally had broken out into a
;; separate function.  Note that Emacs doesn't have the original
;; function.
(defun py-symbol-near-point ()
  "Return the first textual item to the nearest point."
  ;; alg stolen from etag.el
  (save-excursion
    (with-syntax-table py-dotted-expression-syntax-table
      (if (or (bobp) (not (memq (char-syntax (char-before)) '(?w ?_))))
	  (while (not (looking-at "\\sw\\|\\s_\\|\\'"))
	    (forward-char 1)))
      (while (looking-at "\\sw\\|\\s_")
	(forward-char 1))
      (if (re-search-backward "\\sw\\|\\s_" nil t)
	  (progn (forward-char 1)
		 (buffer-substring (point)
				   (progn (forward-sexp -1)
					  (while (looking-at "\\s'")
					    (forward-char 1))
					  (point))))
	nil))))

(defun py-help-at-point ()
  "Get help from Python based on the symbol nearest point."
  (interactive)
  (let* ((sym (py-symbol-near-point))
	 (base (substring sym 0 (or (search "." sym :from-end t) 0)))
	 cmd)
    (if (not (equal base ""))
        (setq cmd (concat "import " base "\n")))
    (setq cmd (concat "import pydoc\n"
                      cmd
		      "try: pydoc.help('" sym "')\n"
		      "except: print 'No help available on:', \"" sym "\""))
    (message cmd)
    (py-execute-string cmd)
    (set-buffer "*Python Output*")
    ;; BAW: Should we really be leaving the output buffer in help-mode?
    (help-mode)))



;; Documentation functions

;; dump the long form of the mode blurb; does the usual doc escapes,
;; plus lines of the form ^[vc]:name$ to suck variable & command docs
;; out of the right places, along with the keys they're on & current
;; values
(defun py-dump-help-string (str)
  (with-output-to-temp-buffer "*Help*"
    (let ((locals (buffer-local-variables))
	  funckind funcname func funcdoc
	  (start 0) mstart end
	  keys )
      (while (string-match "^%\\([vc]\\):\\(.+\\)\n" str start)
	(setq mstart (match-beginning 0)  end (match-end 0)
	      funckind (substring str (match-beginning 1) (match-end 1))
	      funcname (substring str (match-beginning 2) (match-end 2))
	      func (intern funcname))
	(princ (substitute-command-keys (substring str start mstart)))
	(cond
	 ((equal funckind "c")		; command
	  (setq funcdoc (documentation func)
		keys (concat
		      "Key(s): "
		      (mapconcat 'key-description
				 (where-is-internal func py-mode-map)
				 ", "))))
	 ((equal funckind "v")		; variable
	  (setq funcdoc (documentation-property func 'variable-documentation)
		keys (if (assq func locals)
			 (concat
			  "Local/Global values: "
			  (prin1-to-string (symbol-value func))
			  " / "
			  (prin1-to-string (default-value func)))
		       (concat
			"Value: "
			(prin1-to-string (symbol-value func))))))
	 (t				; unexpected
	  (error "Error in py-dump-help-string, tag `%s'" funckind)))
	(princ (format "\n-> %s:\t%s\t%s\n\n"
		       (if (equal funckind "c") "Command" "Variable")
		       funcname keys))
	(princ funcdoc)
	(terpri)
	(setq start end))
      (princ (substitute-command-keys (substring str start))))
    (print-help-return-message)))

(defun py-describe-mode ()
  "Dump long form of Python-mode docs."
  (interactive)
  (py-dump-help-string "Major mode for editing Python files.
Knows about Python indentation, tokens, comments and continuation lines.
Paragraphs are separated by blank lines only.

Major sections below begin with the string `@'; specific function and
variable docs begin with `->'.

@EXECUTING PYTHON CODE

\\[py-execute-import-or-reload]\timports or reloads the file in the Python interpreter
\\[py-execute-buffer]\tsends the entire buffer to the Python interpreter
\\[py-execute-region]\tsends the current region
\\[py-execute-def-or-class]\tsends the current function or class definition
\\[py-execute-string]\tsends an arbitrary string
\\[py-shell]\tstarts a Python interpreter window; this will be used by
\tsubsequent Python execution commands
%c:py-execute-import-or-reload
%c:py-execute-buffer
%c:py-execute-region
%c:py-execute-def-or-class
%c:py-execute-string
%c:py-shell

@VARIABLES

py-indent-offset\tindentation increment
py-block-comment-prefix\tcomment string used by comment-region

py-python-command\tshell command to invoke Python interpreter
py-temp-directory\tdirectory used for temp files (if needed)

py-beep-if-tab-change\tring the bell if tab-width is changed
%v:py-indent-offset
%v:py-block-comment-prefix
%v:py-python-command
%v:py-temp-directory
%v:py-beep-if-tab-change

@KINDS OF LINES

Each physical line in the file is either a `continuation line' (the
preceding line ends with a backslash that's not part of a comment, or
the paren/bracket/brace nesting level at the start of the line is
non-zero, or both) or an `initial line' (everything else).

An initial line is in turn a `blank line' (contains nothing except
possibly blanks or tabs), a `comment line' (leftmost non-blank
character is `#'), or a `code line' (everything else).

Comment Lines

Although all comment lines are treated alike by Python, Python mode
recognizes two kinds that act differently with respect to indentation.

An `indenting comment line' is a comment line with a blank, tab or
nothing after the initial `#'.  The indentation commands (see below)
treat these exactly as if they were code lines: a line following an
indenting comment line will be indented like the comment line.  All
other comment lines (those with a non-whitespace character immediately
following the initial `#') are `non-indenting comment lines', and
their indentation is ignored by the indentation commands.

Indenting comment lines are by far the usual case, and should be used
whenever possible.  Non-indenting comment lines are useful in cases
like these:

\ta = b   # a very wordy single-line comment that ends up being
\t        #... continued onto another line

\tif a == b:
##\t\tprint 'panic!' # old code we've `commented out'
\t\treturn a

Since the `#...' and `##' comment lines have a non-whitespace
character following the initial `#', Python mode ignores them when
computing the proper indentation for the next line.

Continuation Lines and Statements

The Python-mode commands generally work on statements instead of on
individual lines, where a `statement' is a comment or blank line, or a
code line and all of its following continuation lines (if any)
considered as a single logical unit.  The commands in this mode
generally (when it makes sense) automatically move to the start of the
statement containing point, even if point happens to be in the middle
of some continuation line.


@INDENTATION

Primarily for entering new code:
\t\\[indent-for-tab-command]\t indent line appropriately
\t\\[py-newline-and-indent]\t insert newline, then indent
\t\\[py-electric-backspace]\t reduce indentation, or delete single character

Primarily for reindenting existing code:
\t\\[py-guess-indent-offset]\t guess py-indent-offset from file content; change locally
\t\\[universal-argument] \\[py-guess-indent-offset]\t ditto, but change globally

\t\\[py-indent-region]\t reindent region to match its context
\t\\[py-shift-region-left]\t shift region left by py-indent-offset
\t\\[py-shift-region-right]\t shift region right by py-indent-offset

Unlike most programming languages, Python uses indentation, and only
indentation, to specify block structure.  Hence the indentation supplied
automatically by Python-mode is just an educated guess:  only you know
the block structure you intend, so only you can supply correct
indentation.

The \\[indent-for-tab-command] and \\[py-newline-and-indent] keys try to suggest plausible indentation, based on
the indentation of preceding statements.  E.g., assuming
py-indent-offset is 4, after you enter
\tif a > 0: \\[py-newline-and-indent]
the cursor will be moved to the position of the `_' (_ is not a
character in the file, it's just used here to indicate the location of
the cursor):
\tif a > 0:
\t    _
If you then enter `c = d' \\[py-newline-and-indent], the cursor will move
to
\tif a > 0:
\t    c = d
\t    _
Python-mode cannot know whether that's what you intended, or whether
\tif a > 0:
\t    c = d
\t_
was your intent.  In general, Python-mode either reproduces the
indentation of the (closest code or indenting-comment) preceding
statement, or adds an extra py-indent-offset blanks if the preceding
statement has `:' as its last significant (non-whitespace and non-
comment) character.  If the suggested indentation is too much, use
\\[py-electric-backspace] to reduce it.

Continuation lines are given extra indentation.  If you don't like the
suggested indentation, change it to something you do like, and Python-
mode will strive to indent later lines of the statement in the same way.

If a line is a continuation line by virtue of being in an unclosed
paren/bracket/brace structure (`list', for short), the suggested
indentation depends on whether the current line contains the first item
in the list.  If it does, it's indented py-indent-offset columns beyond
the indentation of the line containing the open bracket.  If you don't
like that, change it by hand.  The remaining items in the list will mimic
whatever indentation you give to the first item.

If a line is a continuation line because the line preceding it ends with
a backslash, the third and following lines of the statement inherit their
indentation from the line preceding them.  The indentation of the second
line in the statement depends on the form of the first (base) line:  if
the base line is an assignment statement with anything more interesting
than the backslash following the leftmost assigning `=', the second line
is indented two columns beyond that `='.  Else it's indented to two
columns beyond the leftmost solid chunk of non-whitespace characters on
the base line.

Warning:  indent-region should not normally be used!  It calls \\[indent-for-tab-command]
repeatedly, and as explained above, \\[indent-for-tab-command] can't guess the block
structure you intend.
%c:indent-for-tab-command
%c:py-newline-and-indent
%c:py-electric-backspace


The next function may be handy when editing code you didn't write:
%c:py-guess-indent-offset


The remaining `indent' functions apply to a region of Python code.  They
assume the block structure (equals indentation, in Python) of the region
is correct, and alter the indentation in various ways while preserving
the block structure:
%c:py-indent-region
%c:py-shift-region-left
%c:py-shift-region-right

@MARKING & MANIPULATING REGIONS OF CODE

\\[py-mark-block]\t mark block of lines
\\[py-mark-def-or-class]\t mark smallest enclosing def
\\[universal-argument] \\[py-mark-def-or-class]\t mark smallest enclosing class
\\[comment-region]\t comment out region of code
\\[universal-argument] \\[comment-region]\t uncomment region of code
%c:py-mark-block
%c:py-mark-def-or-class
%c:comment-region

@MOVING POINT

\\[py-previous-statement]\t move to statement preceding point
\\[py-next-statement]\t move to statement following point
\\[py-goto-block-up]\t move up to start of current block
\\[py-beginning-of-def-or-class]\t move to start of def
\\[universal-argument] \\[py-beginning-of-def-or-class]\t move to start of class
\\[py-end-of-def-or-class]\t move to end of def
\\[universal-argument] \\[py-end-of-def-or-class]\t move to end of class

The first two move to one statement beyond the statement that contains
point.  A numeric prefix argument tells them to move that many
statements instead.  Blank lines, comment lines, and continuation lines
do not count as `statements' for these commands.  So, e.g., you can go
to the first code statement in a file by entering
\t\\[beginning-of-buffer]\t to move to the top of the file
\t\\[py-next-statement]\t to skip over initial comments and blank lines
Or do `\\[py-previous-statement]' with a huge prefix argument.
%c:py-previous-statement
%c:py-next-statement
%c:py-goto-block-up
%c:py-beginning-of-def-or-class
%c:py-end-of-def-or-class

@LITTLE-KNOWN EMACS COMMANDS PARTICULARLY USEFUL IN PYTHON MODE

`\\[indent-new-comment-line]' is handy for entering a multi-line comment.

`\\[set-selective-display]' with a `small' prefix arg is ideally suited for viewing the
overall class and def structure of a module.

`\\[back-to-indentation]' moves point to a line's first non-blank character.

`\\[indent-relative]' is handy for creating odd indentation.

@OTHER EMACS HINTS

If you don't like the default value of a variable, change its value to
whatever you do like by putting a `setq' line in your .emacs file.
E.g., to set the indentation increment to 4, put this line in your
.emacs:
\t(setq  py-indent-offset  4)
To see the value of a variable, do `\\[describe-variable]' and enter the variable
name at the prompt.

When entering a key sequence like `C-c C-n', it is not necessary to
release the CONTROL key after doing the `C-c' part -- it suffices to
press the CONTROL key, press and release `c' (while still holding down
CONTROL), press and release `n' (while still holding down CONTROL), &
then release CONTROL.

Entering Python mode calls with no arguments the value of the variable
`python-mode-hook', if that value exists and is not nil; for backward
compatibility it also tries `py-mode-hook'; see the `Hooks' section of
the Elisp manual for details.

Obscure:  When python-mode is first loaded, it looks for all bindings
to newline-and-indent in the global keymap, and shadows them with
local bindings to py-newline-and-indent."))

(require 'info-look)
;; The info-look package does not always provide this function (it
;; appears this is the case with XEmacs 21.1)
(when (fboundp 'info-lookup-maybe-add-help)
  (info-lookup-maybe-add-help
   :mode 'python-mode
   :regexp "[a-zA-Z0-9_]+"
   :doc-spec '(("(python-lib)Module Index")
	       ("(python-lib)Class-Exception-Object Index")
	       ("(python-lib)Function-Method-Variable Index")
	       ("(python-lib)Miscellaneous Index")))
  )


;; Helper functions
(defvar py-parse-state-re
  (concat
   "^[ \t]*\\(elif\\|else\\|while\\|def\\|class\\)\\>"
   "\\|"
   "^[^ #\t\n]"))

(defun py-parse-state ()
  "Return the parse state at point (see `parse-partial-sexp' docs)."
  (save-excursion
    (let ((here (point))
	  pps done)
      (while (not done)
	;; back up to the first preceding line (if any; else start of
	;; buffer) that begins with a popular Python keyword, or a
	;; non- whitespace and non-comment character.  These are good
	;; places to start parsing to see whether where we started is
	;; at a non-zero nesting level.  It may be slow for people who
	;; write huge code blocks or huge lists ... tough beans.
	(re-search-backward py-parse-state-re nil 'move)
	(beginning-of-line)
	;; In XEmacs, we have a much better way to test for whether
	;; we're in a triple-quoted string or not.  Emacs does not
	;; have this built-in function, which is its loss because
	;; without scanning from the beginning of the buffer, there's
	;; no accurate way to determine this otherwise.
	(save-excursion (setq pps (parse-partial-sexp (point) here)))
	;; make sure we don't land inside a triple-quoted string
	(setq done (or (not (nth 3 pps))
		       (bobp)))
	;; Just go ahead and short circuit the test back to the
	;; beginning of the buffer.  This will be slow, but not
	;; nearly as slow as looping through many
	;; re-search-backwards.
	(if (not done)
	    (goto-char (point-min))))
      pps)))

(defun py-nesting-level ()
  "Return the buffer position of the last unclosed enclosing list.
If nesting level is zero, return nil."
  (let ((status (py-parse-state)))
    (if (zerop (car status))
	nil				; not in a nest
      (car (cdr status)))))		; char# of open bracket

(defun py-backslash-continuation-line-p ()
  "Return t iff preceding line ends with backslash that is not in a comment."
  (save-excursion
    (beginning-of-line)
    (and
     ;; use a cheap test first to avoid the regexp if possible
     ;; use 'eq' because char-after may return nil
     (eq (char-after (- (point) 2)) ?\\ )
     ;; make sure; since eq test passed, there is a preceding line
     (forward-line -1)			; always true -- side effect
     (looking-at py-continued-re))))

(defun py-continuation-line-p ()
  "Return t iff current line is a continuation line."
  (save-excursion
    (beginning-of-line)
    (or (py-backslash-continuation-line-p)
	(py-nesting-level))))

(defun py-goto-beginning-of-tqs (delim)
  "Go to the beginning of the triple quoted string we find ourselves in.
DELIM is the TQS string delimiter character we're searching backwards
for."
  (let ((skip (and delim (make-string 1 delim)))
	(continue t))
    (when skip
      (save-excursion
	(while continue
	  (py-safe (search-backward skip))
	  (setq continue (and (not (bobp))
			      (= (char-before) ?\\))))
	(if (and (= (char-before) delim)
		 (= (char-before (1- (point))) delim))
	    (setq skip (make-string 3 delim))))
      ;; we're looking at a triple-quoted string
      (py-safe (search-backward skip)))))

(defun py-goto-initial-line ()
  "Go to the initial line of the current statement.
Usually this is the line we're on, but if we're on the 2nd or
following lines of a continuation block, we need to go up to the first
line of the block."
  ;; Tricky: We want to avoid quadratic-time behavior for long
  ;; continued blocks, whether of the backslash or open-bracket
  ;; varieties, or a mix of the two.  The following manages to do that
  ;; in the usual cases.
  ;;
  ;; Also, if we're sitting inside a triple quoted string, this will
  ;; drop us at the line that begins the string.
  (let (open-bracket-pos)
    (while (py-continuation-line-p)
      (beginning-of-line)
      (if (py-backslash-continuation-line-p)
	  (while (py-backslash-continuation-line-p)
	    (forward-line -1))
	;; else zip out of nested brackets/braces/parens
	(while (setq open-bracket-pos (py-nesting-level))
	  (goto-char open-bracket-pos)))))
  (beginning-of-line))

(defun py-goto-beyond-final-line ()
  "Go to the point just beyond the fine line of the current statement.
Usually this is the start of the next line, but if this is a
multi-line statement we need to skip over the continuation lines."
  ;; Tricky: Again we need to be clever to avoid quadratic time
  ;; behavior.
  ;;
  ;; XXX: Not quite the right solution, but deals with multi-line doc
  ;; strings
  (if (looking-at (concat "[ \t]*\\(" py-stringlit-re "\\)"))
      (goto-char (match-end 0)))
  ;;
  (forward-line 1)
  (let (state)
    (while (and (py-continuation-line-p)
		(not (eobp)))
      ;; skip over the backslash flavor
      (while (and (py-backslash-continuation-line-p)
		  (not (eobp)))
	(forward-line 1))
      ;; if in nest, zip to the end of the nest
      (setq state (py-parse-state))
      (if (and (not (zerop (car state)))
	       (not (eobp)))
	  (progn
	    (parse-partial-sexp (point) (point-max) 0 nil state)
	    (forward-line 1))))))

(defun py-statement-opens-block-p ()
  "Return t iff the current statement opens a block.
I.e., iff it ends with a colon that is not in a comment.  Point should 
be at the start of a statement."
  (save-excursion
    (let ((start (point))
	  (finish (progn (py-goto-beyond-final-line) (1- (point))))
	  (searching t)
	  (answer nil)
	  state)
      (goto-char start)
      (while searching
	;; look for a colon with nothing after it except whitespace, and
	;; maybe a comment
	(if (re-search-forward ":\\([ \t]\\|\\\\\n\\)*\\(#.*\\)?$"
			       finish t)
	    (if (eq (point) finish)	; note: no `else' clause; just
					; keep searching if we're not at
					; the end yet
		;; sure looks like it opens a block -- but it might
		;; be in a comment
		(progn
		  (setq searching nil)	; search is done either way
		  (setq state (parse-partial-sexp start
						  (match-beginning 0)))
		  (setq answer (not (nth 4 state)))))
	  ;; search failed: couldn't find another interesting colon
	  (setq searching nil)))
      answer)))

(defun py-statement-closes-block-p ()
  "Return t iff the current statement closes a block.
I.e., if the line starts with `return', `raise', `break', `continue',
and `pass'.  This doesn't catch embedded statements."
  (let ((here (point)))
    (py-goto-initial-line)
    (back-to-indentation)
    (prog1
	(looking-at (concat py-block-closing-keywords-re "\\>"))
      (goto-char here))))

(defun py-goto-beyond-block ()
  "Go to point just beyond the final line of block begun by the current line.
This is the same as where `py-goto-beyond-final-line' goes unless
we're on colon line, in which case we go to the end of the block.
Assumes point is at the beginning of the line."
  (if (py-statement-opens-block-p)
      (py-mark-block nil 'just-move)
    (py-goto-beyond-final-line)))

(defun py-goto-statement-at-or-above ()
  "Go to the start of the first statement at or preceding point.
Return t if there is such a statement, otherwise nil.  `Statement'
does not include blank lines, comments, or continuation lines."
  (py-goto-initial-line)
  (if (looking-at py-blank-or-comment-re)
      ;; skip back over blank & comment lines
      ;; note:  will skip a blank or comment line that happens to be
      ;; a continuation line too
      (if (re-search-backward "^[ \t]*[^ \t#\n]" nil t)
	  (progn (py-goto-initial-line) t)
	nil)
    t))

(defun py-goto-statement-below ()
  "Go to start of the first statement following the statement containing point.
Return t if there is such a statement, otherwise nil.  `Statement'
does not include blank lines, comments, or continuation lines."
  (beginning-of-line)
  (let ((start (point)))
    (py-goto-beyond-final-line)
    (while (and
	    (or (looking-at py-blank-or-comment-re)
		(py-in-literal))
	    (not (eobp)))
      (forward-line 1))
    (if (eobp)
	(progn (goto-char start) nil)
      t)))

(defun py-go-up-tree-to-keyword (key)
  "Go to begining of statement starting with KEY, at or preceding point.

KEY is a regular expression describing a Python keyword.  Skip blank
lines and non-indenting comments.  If the statement found starts with
KEY, then stop, otherwise go back to first enclosing block starting
with KEY.  If successful, leave point at the start of the KEY line and 
return t.  Otherwise, leav point at an undefined place and return nil."
  ;; skip blanks and non-indenting #
  (py-goto-initial-line)
  (while (and
	  (looking-at "[ \t]*\\($\\|#[^ \t\n]\\)")
	  (zerop (forward-line -1)))	; go back
    nil)
  (py-goto-initial-line)
  (let* ((re (concat "[ \t]*" key "\\b"))
	 (case-fold-search nil)		; let* so looking-at sees this
	 (found (looking-at re))
	 (dead nil))
    (while (not (or found dead))
      (condition-case nil		; in case no enclosing block
	  (py-goto-block-up 'no-mark)
	(error (setq dead t)))
      (or dead (setq found (looking-at re))))
    (beginning-of-line)
    found))

(defun py-suck-up-leading-text ()
  "Return string in buffer from start of indentation to end of line.
Prefix with \"...\" if leading whitespace was skipped."
  (save-excursion
    (back-to-indentation)
    (concat
     (if (bolp) "" "...")
     (buffer-substring (point) (progn (end-of-line) (point))))))

(defun py-suck-up-first-keyword ()
  "Return first keyword on the line as a Lisp symbol.
`Keyword' is defined (essentially) as the regular expression
([a-z]+).  Returns nil if none was found."
  (let ((case-fold-search nil))
    (if (looking-at "[ \t]*\\([a-z]+\\)\\b")
	(intern (buffer-substring (match-beginning 1) (match-end 1)))
      nil)))

(defun py-current-defun ()
  "Python value for `add-log-current-defun-function'.
This tells add-log.el how to find the current function/method/variable."
  (save-excursion
    (if (re-search-backward py-defun-start-re nil t)
	(or (match-string 3)
	    (let ((method (match-string 2)))
	      (if (and (not (zerop (length (match-string 1))))
		       (re-search-backward py-class-start-re nil t))
		  (concat (match-string 1) "." method)
		method)))
      nil)))


(defconst py-help-address "python-mode@python.org"
  "Address accepting submission of bug reports.")

(defun py-version ()
  "Echo the current version of `python-mode' in the minibuffer."
  (interactive)
  (message "Using `python-mode' version %s" py-version)
  (py-keep-region-active))

;; only works under Emacs 19
;(eval-when-compile
;  (require 'reporter))

(defun py-submit-bug-report (enhancement-p)
  "Submit via mail a bug report on `python-mode'.
With \\[universal-argument] (programmatically, argument ENHANCEMENT-P
non-nil) just submit an enhancement request."
  (interactive
   (list (not (y-or-n-p
	       "Is this a bug report (hit `n' to send other comments)? "))))
  (let ((reporter-prompt-for-summary-p (if enhancement-p
					   "(Very) brief summary: "
					 t)))
    (require 'reporter)
    (reporter-submit-bug-report
     py-help-address			;address
     (concat "python-mode " py-version)	;pkgname
     ;; varlist
     (if enhancement-p nil
       '(py-python-command
	 py-indent-offset
	 py-block-comment-prefix
	 py-temp-directory
	 py-beep-if-tab-change))
     nil				;pre-hooks
     nil				;post-hooks
     "Dear Barry,")			;salutation
    (if enhancement-p nil
      (set-mark (point))
      (insert 
"Please replace this text with a sufficiently large code sample\n\
and an exact recipe so that I can reproduce your problem.  Failure\n\
to do so may mean a greater delay in fixing your bug.\n\n")
      (exchange-point-and-mark)
      (py-keep-region-active))))


(defun py-kill-emacs-hook ()
  "Delete files in `py-file-queue'.
These are Python temporary files awaiting execution."
  (mapcar #'(lambda (filename)
	      (py-safe (delete-file filename)))
	  py-file-queue))

;; arrange to kill temp files when Emacs exists
(add-hook 'kill-emacs-hook 'py-kill-emacs-hook)
(add-hook 'comint-output-filter-functions 'py-pdbtrack-track-stack-file)

;; Add a designator to the minor mode strings
(or (assq 'py-pdbtrack-minor-mode-string minor-mode-alist)
    (push '(py-pdbtrack-is-tracking-p py-pdbtrack-minor-mode-string)
	  minor-mode-alist))



;;; paragraph and string filling code from Bernhard Herzog
;;; see http://mail.python.org/pipermail/python-list/2002-May/103189.html

(defun py-fill-comment (&optional justify)
  "Fill the comment paragraph around point"
  (let (;; Non-nil if the current line contains a comment.
	has-comment

	;; If has-comment, the appropriate fill-prefix for the comment.
	comment-fill-prefix)

    ;; Figure out what kind of comment we are looking at.
    (save-excursion
      (beginning-of-line)
      (cond
       ;; A line with nothing but a comment on it?
       ((looking-at "[ \t]*#[# \t]*")
	(setq has-comment t
	      comment-fill-prefix (buffer-substring (match-beginning 0)
						    (match-end 0))))

       ;; A line with some code, followed by a comment? Remember that the hash
       ;; which starts the comment shouldn't be part of a string or character.
       ((progn
	  (while (not (looking-at "#\\|$"))
	    (skip-chars-forward "^#\n\"'\\")
	    (cond
	     ((eq (char-after (point)) ?\\) (forward-char 2))
	     ((memq (char-after (point)) '(?\" ?')) (forward-sexp 1))))
	  (looking-at "#+[\t ]*"))
	(setq has-comment t)
	(setq comment-fill-prefix
	      (concat (make-string (current-column) ? )
		      (buffer-substring (match-beginning 0) (match-end 0)))))))

    (if (not has-comment)
	(fill-paragraph justify)

      ;; Narrow to include only the comment, and then fill the region.
      (save-restriction
	(narrow-to-region

	 ;; Find the first line we should include in the region to fill.
	 (save-excursion
	   (while (and (zerop (forward-line -1))
		       (looking-at "^[ \t]*#")))

	   ;; We may have gone to far.  Go forward again.
	   (or (looking-at "^[ \t]*#")
	       (forward-line 1))
	   (point))

	 ;; Find the beginning of the first line past the region to fill.
	 (save-excursion
	   (while (progn (forward-line 1)
			 (looking-at "^[ \t]*#")))
	   (point)))

	;; Lines with only hashes on them can be paragraph boundaries.
	(let ((paragraph-start (concat paragraph-start "\\|[ \t#]*$"))
	      (paragraph-separate (concat paragraph-separate "\\|[ \t#]*$"))
	      (fill-prefix comment-fill-prefix))
	  ;;(message "paragraph-start %S paragraph-separate %S"
	  ;;paragraph-start paragraph-separate)
	  (fill-paragraph justify))))
    t))


(defun py-fill-string (start &optional justify)
  "Fill the paragraph around (point) in the string starting at start"
  ;; basic strategy: narrow to the string and call the default
  ;; implementation
  (let (;; the start of the string's contents
	string-start
	;; the end of the string's contents
	string-end
	;; length of the string's delimiter
	delim-length
	;; The string delimiter
	delim
	)

    (save-excursion
      (goto-char start)
      (if (looking-at "\\('''\\|\"\"\"\\|'\\|\"\\)\\\\?\n?")
	  (setq string-start (match-end 0)
		delim-length (- (match-end 1) (match-beginning 1))
		delim (buffer-substring-no-properties (match-beginning 1)
						      (match-end 1)))
	(error "The parameter start is not the beginning of a python string"))

      ;; if the string is the first token on a line and doesn't start with
      ;; a newline, fill as if the string starts at the beginning of the
      ;; line. this helps with one line docstrings
      (save-excursion
	(beginning-of-line)
	(and (/= (char-before string-start) ?\n)
	     (looking-at (concat "[ \t]*" delim))
	     (setq string-start (point))))

      (forward-sexp (if (= delim-length 3) 2 1))

      ;; with both triple quoted strings and single/double quoted strings
      ;; we're now directly behind the first char of the end delimiter
      ;; (this doesn't work correctly when the triple quoted string
      ;; contains the quote mark itself). The end of the string's contents
      ;; is one less than point
      (setq string-end (1- (point))))

    ;; Narrow to the string's contents and fill the current paragraph
    (save-restriction
      (narrow-to-region string-start string-end)
      (let ((ends-with-newline (= (char-before (point-max)) ?\n)))
	(fill-paragraph justify)
	(if (and (not ends-with-newline)
		 (= (char-before (point-max)) ?\n))
	    ;; the default fill-paragraph implementation has inserted a
	    ;; newline at the end. Remove it again.
	    (save-excursion
	      (goto-char (point-max))
	      (delete-char -1)))))

    ;; return t to indicate that we've done our work
    t))

(defun py-fill-paragraph (&optional justify)
  "Like \\[fill-paragraph], but handle Python comments and strings.
If any of the current line is a comment, fill the comment or the
paragraph of it that point is in, preserving the comment's indentation
and initial `#'s.
If point is inside a string, narrow to that string and fill.
"
  (interactive "P")
  (let* ((bod (py-point 'bod))
	 (pps (parse-partial-sexp bod (point))))
    (cond
     ;; are we inside a comment or on a line with only whitespace before
     ;; the comment start?
     ((or (nth 4 pps)
	  (save-excursion (beginning-of-line) (looking-at "[ \t]*#")))
      (py-fill-comment justify))
     ;; are we inside a string?
     ((nth 3 pps)
      (py-fill-string (nth 8 pps)))
     ;; otherwise use the default
     (t
      (fill-paragraph justify)))))



(provide 'python-mode)
;;; python-mode.el ends here
