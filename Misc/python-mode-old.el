;;; python-mode.el --- Major mode for editing Python programs

;; Copyright (C) 1992,1993,1994  Tim Peters

;; Author: 1995 Barry A. Warsaw
;;         1992-1994 Tim Peters
;; Maintainer:    python-mode@python.org
;; Created:       Feb 1992
;; Version:       2.26
;; Last Modified: 1995/07/05 23:26:15
;; Keywords: python editing language major-mode

;; This software is provided as-is, without express or implied
;; warranty.  Permission to use, copy, modify, distribute or sell this
;; software, without fee, for any purpose and by any individual or
;; organization, is hereby granted, provided that the above copyright
;; notice and this paragraph appear in all copies.

;;; Commentary:
;;
;; This is a major mode for editing Python programs.  It was developed
;; by Tim Peters <tim@ksr.com> after an original idea by Michael
;; A. Guravage.  Tim doesn't appear to be on the 'net any longer so I
;; have undertaken maintenance of the mode.

;; At some point this mode will undergo a rewrite to bring it more in
;; line with GNU Emacs Lisp coding standards.  But all in all, the
;; mode works exceedingly well.

;; The following statements, placed in your .emacs file or
;; site-init.el, will cause this file to be autoloaded, and
;; python-mode invoked, when visiting .py files (assuming this file is
;; in your load-path):
;;
;;	(autoload 'python-mode "python-mode" "Python editing mode." t)
;;	(setq auto-mode-alist
;;	      (cons '("\\.py$" . python-mode) auto-mode-alist))

;; Here's a brief list of recent additions/improvements:
;;
;; - Wrapping and indentation within triple quote strings should work
;;   properly now.
;; - `Standard' bug reporting mechanism (use C-c C-b)
;; - py-mark-block was moved to C-c C-m
;; - C-c C-v shows you the python-mode version
;; - a basic python-font-lock-keywords has been added for Emacs 19
;;   font-lock colorizations.
;; - proper interaction with pending-del and del-sel modes.
;; - New py-electric-colon (:) command for improved outdenting.  Also
;;   py-indent-line (TAB) should handle outdented lines better.
;; - New commands py-outdent-left (C-c C-l) and py-indent-right (C-c C-r)

;; Here's a brief to do list:
;;
;; - Better integration with gud-mode for debugging.
;; - Rewrite according to GNU Emacs Lisp standards.
;; - py-delete-char should obey numeric arguments.
;; - even better support for outdenting.  Guido suggests outdents of
;;   at least one level after a return, raise, break, or continue
;;   statement.
;; - de-electrify colon inside literals (e.g. comments and strings)

;; If you can think of more things you'd like to see, drop me a line.
;; If you want to report bugs, use py-submit-bug-report (C-c C-b).
;;
;; Note that I only test things on XEmacs (currently 19.11).  If you
;; port stuff to FSF Emacs 19, or Emacs 18, please send me your
;; patches.

;; LCD Archive Entry:
;; python-mode|Barry A. Warsaw|python-mode@python.org
;; |Major mode for editing Python programs
;; |1995/07/05 23:26:15|2.26|

;;; Code:


;; user definable variables
;; vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

(defvar py-python-command "python"
  "*Shell command used to start Python interpreter.")

(defvar py-indent-offset 8		; argue with Guido <grin>
  "*Indentation increment.
Note that `\\[py-guess-indent-offset]' can usually guess a good value
when you're editing someone else's Python code.")

(defvar py-block-comment-prefix "##"
  "*String used by `py-comment-region' to comment out a block of code.
This should follow the convention for non-indenting comment lines so
that the indentation commands won't get confused (i.e., the string
should be of the form `#x...' where `x' is not a blank or a tab, and
`...' is arbitrary).")

(defvar py-scroll-process-buffer t
  "*Scroll Python process buffer as output arrives.
If nil, the Python process buffer acts, with respect to scrolling, like
Shell-mode buffers normally act.  This is surprisingly complicated and
so won't be explained here; in fact, you can't get the whole story
without studying the Emacs C code.

If non-nil, the behavior is different in two respects (which are
slightly inaccurate in the interest of brevity):

  - If the buffer is in a window, and you left point at its end, the
    window will scroll as new output arrives, and point will move to the
    buffer's end, even if the window is not the selected window (that
    being the one the cursor is in).  The usual behavior for shell-mode
    windows is not to scroll, and to leave point where it was, if the
    buffer is in a window other than the selected window.

  - If the buffer is not visible in any window, and you left point at
    its end, the buffer will be popped into a window as soon as more
    output arrives.  This is handy if you have a long-running
    computation and don't want to tie up screen area waiting for the
    output.  The usual behavior for a shell-mode buffer is to stay
    invisible until you explicitly visit it.

Note the `and if you left point at its end' clauses in both of the
above:  you can `turn off' the special behaviors while output is in
progress, by visiting the Python buffer and moving point to anywhere
besides the end.  Then the buffer won't scroll, point will remain where
you leave it, and if you hide the buffer it will stay hidden until you
visit it again.  You can enable and disable the special behaviors as
often as you like, while output is in progress, by (respectively) moving
point to, or away from, the end of the buffer.

Warning:  If you expect a large amount of output, you'll probably be
happier setting this option to nil.

Obscure:  `End of buffer' above should really say `at or beyond the
process mark', but if you know what that means you didn't need to be
told <grin>.")

(defvar py-temp-directory
  (let ((ok '(lambda (x)
	       (and x
		    (setq x (expand-file-name x)) ; always true
		    (file-directory-p x)
		    (file-writable-p x)
		    x))))
    (or (funcall ok (getenv "TMPDIR"))
	(funcall ok "/usr/tmp")
	(funcall ok "/tmp")
	(funcall ok  ".")
	(error
	 "Couldn't find a usable temp directory -- set py-temp-directory")))
  "*Directory used for temp files created by a *Python* process.
By default, the first directory from this list that exists and that you
can write into:  the value (if any) of the environment variable TMPDIR,
/usr/tmp, /tmp, or the current directory.")

(defvar py-beep-if-tab-change t
  "*Ring the bell if tab-width is changed.
If a comment of the form

  \t# vi:set tabsize=<number>:

is found before the first code line when the file is entered, and the
current value of (the general Emacs variable) `tab-width' does not
equal <number>, `tab-width' is set to <number>, a message saying so is
displayed in the echo area, and if `py-beep-if-tab-change' is non-nil
the Emacs bell is also rung as a warning.")

;; These were the previous font-lock keywords, but I think I now
;; prefer the ones from XEmacs 19.12's font-lock.el.  I've merged the
;; two into the new definition below.
;;
;;(defvar python-font-lock-keywords
;;  (list
;;   (cons
;;    (concat
;;     "\\<\\("
;;     (mapconcat
;;      'identity
;;      '("access"  "and"      "break"  "continue"
;;	"del"     "elif"     "else"   "except"
;;	"exec"    "finally"  "for"    "from"
;;	"global"  "if"       "import" "in"
;;	"is"      "lambda"   "not"    "or"
;;	"pass"    "print"    "raise"  "return"
;;	"try"     "while"    "def"    "class"
;;	)
;;      "\\|")
;;     "\\)\\>")
;;    1)
;;   ;; functions
;;   '("\\bdef\\s +\\(\\sw+\\)(" 1 font-lock-function-name-face)
;;   ;; classes
;;   '("\\bclass\\s +\\(\\sw+\\)[(:]" 1 font-lock-function-name-face)
;;   )
;;  "*Additional keywords to highlight `python-mode' buffers.")

;; These are taken from XEmacs 19.12's font-lock.el file, but have the
;; more complete list of keywords from the previous definition in
;; python-mode.el.  There are a few other minor stylistic changes as
;; well.
;; 
(defvar python-font-lock-keywords
   (list
    (cons (concat
	   "\\b\\("
	   (mapconcat
	    'identity
	    '("access"     "and"      "break"    "continue"
	      "del"        "elif"     "else:"    "except"
	      "except:"    "exec"     "finally:" "for"
	      "from"       "global"   "if"       "import"
	      "in"         "is"       "lambda"   "not"
	      "or"         "pass"     "print"    "raise"
	      "return"     "try:"     "while"
	      )
	    "\\|")
	   "\\)[ \n\t(]")
          1)
    ;; classes
    '("\\bclass[ \t]+\\([a-zA-Z_]+[a-zA-Z0-9_]*\\)"
      1 font-lock-type-face)
    ;; functions
    '("\\bdef[ \t]+\\([a-zA-Z_]+[a-zA-Z0-9_]*\\)"
      1 font-lock-function-name-face)
    )
  "*Additional expressions to highlight in Python mode.")

;; R Lindsay Todd <toddr@rpi.edu> suggests these changes to the
;; original keywords, which wouldn't be necessary if we go with the
;; XEmacs defaults, but which I agree makes sense without them.
;;
;; functions
;; '("\\bdef\\s +\\(\\sw+\\)\\s *(" 1 font-lock-function-name-face)
;; classes
;; '("\\bclass\\s +\\(\\sw+\\)\\s *[(:]" 1 font-lock-type-face)



;; ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
;; NO USER DEFINABLE VARIABLES BEYOND THIS POINT

;; Differentiate between Emacs 18, Lucid Emacs, and Emacs 19.  This
;; seems to be the standard way of checking this.
;; BAW - This is *not* the right solution.  When at all possible,
;; instead of testing for the version of Emacs, use feature tests.

(setq py-this-is-lucid-emacs-p (string-match "Lucid\\|XEmacs" emacs-version))
(setq py-this-is-emacs-19-p
      (and
       (not py-this-is-lucid-emacs-p)
       (string-match "^19\\." emacs-version)))

;; have to bind py-file-queue before installing the kill-emacs hook
(defvar py-file-queue nil
  "Queue of Python temp files awaiting execution.
Currently-active file is at the head of the list.")

;; define a mode-specific abbrev table for those who use such things
(defvar python-mode-abbrev-table nil
  "Abbrev table in use in `python-mode' buffers.")
(define-abbrev-table 'python-mode-abbrev-table nil)

(defvar python-mode-hook nil
  "*Hook called by `python-mode'.")

;; in previous version of python-mode.el, the hook was incorrectly
;; called py-mode-hook, and was not defvar'd.  deprecate its use.
(and (fboundp 'make-obsolete-variable)
     (make-obsolete-variable 'py-mode-hook 'python-mode-hook))

(defvar py-mode-map ()
  "Keymap used in `python-mode' buffers.")

(if py-mode-map
    ()
  (setq py-mode-map (make-sparse-keymap))

  ;; shadow global bindings for newline-and-indent w/ the py- version.
  ;; BAW - this is extremely bad form, but I'm not going to change it
  ;; for now.
  (mapcar (function (lambda (key)
		      (define-key
			py-mode-map key 'py-newline-and-indent)))
   (where-is-internal 'newline-and-indent))

  ;; BAW - you could do it this way, but its not considered proper
  ;; major-mode form.
  (mapcar (function
	   (lambda (x)
	     (define-key py-mode-map (car x) (cdr x))))
	  '((":"         . py-electric-colon)
	    ("\C-c\C-c"  . py-execute-buffer)
	    ("\C-c|"	 . py-execute-region)
	    ("\C-c!"	 . py-shell)
	    ("\177"	 . py-delete-char)
	    ("\n"	 . py-newline-and-indent)
	    ("\C-c:"	 . py-guess-indent-offset)
	    ("\C-c\t"	 . py-indent-region)
	    ("\C-c\C-l"  . py-outdent-left)
	    ("\C-c\C-r"  . py-indent-right)
	    ("\C-c<"	 . py-shift-region-left)
	    ("\C-c>"	 . py-shift-region-right)
	    ("\C-c\C-n"  . py-next-statement)
	    ("\C-c\C-p"  . py-previous-statement)
	    ("\C-c\C-u"  . py-goto-block-up)
	    ("\C-c\C-m"  . py-mark-block)
	    ("\C-c#"	 . py-comment-region)
	    ("\C-c?"	 . py-describe-mode)
	    ("\C-c\C-hm" . py-describe-mode)
	    ("\e\C-a"	 . beginning-of-python-def-or-class)
	    ("\e\C-e"	 . end-of-python-def-or-class)
	    ( "\e\C-h"	 . mark-python-def-or-class)))
  ;; should do all keybindings this way
  (define-key py-mode-map "\C-c\C-b" 'py-submit-bug-report)
  (define-key py-mode-map "\C-c\C-v" 'py-version)
  )

(defvar py-mode-syntax-table nil
  "Syntax table used in `python-mode' buffers.")

(if py-mode-syntax-table
    ()
  (setq py-mode-syntax-table (make-syntax-table))
  ;; BAW - again, blech.
  (mapcar (function
	   (lambda (x) (modify-syntax-entry
			(car x) (cdr x) py-mode-syntax-table)))
	  '(( ?\( . "()" ) ( ?\) . ")(" )
	    ( ?\[ . "(]" ) ( ?\] . ")[" )
	    ( ?\{ . "(}" ) ( ?\} . "){" )
	    ;; fix operator symbols misassigned in the std table
	    ( ?\$ . "." ) ( ?\% . "." ) ( ?\& . "." )
	    ( ?\* . "." ) ( ?\+ . "." ) ( ?\- . "." )
	    ( ?\/ . "." ) ( ?\< . "." ) ( ?\= . "." )
	    ( ?\> . "." ) ( ?\| . "." )
	    ( ?\_ . "w" )	; underscore is legit in names
	    ( ?\' . "\"")	; single quote is string quote
	    ( ?\" . "\"" )	; double quote is string quote too
	    ( ?\` . "$")	; backquote is open and close paren
	    ( ?\# . "<")	; hash starts comment
	    ( ?\n . ">"))))	; newline ends comment

(defconst py-stringlit-re
  (concat
   "'\\([^'\n\\]\\|\\\\.\\)*'"		; single-quoted
   "\\|"				; or
   "\"\\([^\"\n\\]\\|\\\\.\\)*\"")	; double-quoted
  "Regexp matching a Python string literal.")

;; this is tricky because a trailing backslash does not mean
;; continuation if it's in a comment
(defconst py-continued-re
  (concat
   "\\(" "[^#'\"\n\\]" "\\|" py-stringlit-re "\\)*"
   "\\\\$")
  "Regexp matching Python lines that are continued via backslash.")

(defconst py-blank-or-comment-re "[ \t]*\\($\\|#\\)"
  "Regexp matching blank or comment lines.")

(defconst py-outdent-re
  (concat "\\(" (mapconcat 'identity
			   '("else:"
			     "except\\(\\s +.*\\)?:"
			     "finally:"
			     "elif\\s +.*:")
			   "\\|")
	  "\\)")
  "Regexp matching clauses to be outdented one level.")

(defconst py-no-outdent-re
  (concat "\\(" (mapconcat 'identity
			   '("try:"
			     "except\\(\\s +.*\\)?:"
			     "while\\s +.*:"
			     "for\\s +.*:"
			     "if\\s +.*:"
			     "elif\\s +.*:")
			   "\\|")
	  "\\)")
  "Regexp matching lines to not outdent after.")


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

py-indent-offset\tindentation increment
py-block-comment-prefix\tcomment string used by py-comment-region
py-python-command\tshell command to invoke Python interpreter
py-scroll-process-buffer\talways scroll Python process buffer
py-temp-directory\tdirectory used for temp files (if needed)
py-beep-if-tab-change\tring the bell if tab-width is changed"
  (interactive)
  (kill-all-local-variables)
  (set-syntax-table py-mode-syntax-table)
  (setq major-mode 'python-mode
	mode-name "Python"
	local-abbrev-table python-mode-abbrev-table)
  (use-local-map py-mode-map)
  ;; BAW -- style...
  (mapcar (function (lambda (x)
		      (make-local-variable (car x))
		      (set (car x) (cdr x))))
	  '((paragraph-separate . "^[ \t]*$")
	    (paragraph-start	 . "^[ \t]*$")
	    (require-final-newline . t)
	    (comment-start .		"# ")
	    (comment-start-skip .	"# *")
	    (comment-column . 40)
	    (indent-region-function . py-indent-region)
	    (indent-line-function . py-indent-line)))
  ;; hack to allow overriding the tabsize in the file (see tokenizer.c)
  ;;
  ;; not sure where the magic comment has to be; to save time
  ;; searching for a rarity, we give up if it's not found prior to the
  ;; first executable statement.
  ;;
  ;; BAW - on first glance, this seems like complete hackery.  Why was
  ;; this necessary, and is it still necessary?
  (let ((case-fold-search nil)
	(start (point))
	new-tab-width)
    (if (re-search-forward
	 "^[ \t]*#[ \t]*vi:set[ \t]+tabsize=\\([0-9]+\\):"
	 (prog2 (py-next-statement 1) (point) (goto-char 1))
	 t)
	(progn
	  (setq new-tab-width
		(string-to-int
		 (buffer-substring (match-beginning 1) (match-end 1))))
	  (if (= tab-width new-tab-width)
	      nil
	    (setq tab-width new-tab-width)
	    (message "Caution: tab-width changed to %d" new-tab-width)
	    (if py-beep-if-tab-change (beep)))))
    (goto-char start))

  ;; run the mode hook. py-mode-hook use is deprecated
  (if python-mode-hook
      (run-hooks 'python-mode-hook)
    (run-hooks 'py-mode-hook)))


;; electric characters
(defun py-outdent-p ()
  ;; returns non-nil if the current line should outdent one level
  (save-excursion
    (and (progn (back-to-indentation)
		(looking-at py-outdent-re))
	 (progn (backward-to-indentation 1)
		(while (or (looking-at py-blank-or-comment-re)
			   (bobp))
		  (backward-to-indentation 1))
		(not (looking-at py-no-outdent-re)))
	 )))
      

(defun py-electric-colon (arg)
  "Insert a colon.
In certain cases the line is outdented appropriately.  If a numeric
argument is provided, that many colons are inserted non-electrically."
  (interactive "P")
  (self-insert-command (prefix-numeric-value arg))
  (save-excursion
    (let ((here (point))
	  (outdent 0)
	  (indent (py-compute-indentation)))
      (if (and (not arg)
	       (py-outdent-p)
	       (= indent (save-excursion
			   (forward-line -1)
			   (py-compute-indentation)))
	       )
	  (setq outdent py-indent-offset))
      ;; Don't indent, only outdent.  This assumes that any lines that
      ;; are already outdented relative to py-compute-indentation were
      ;; put there on purpose.  Its highly annoying to have `:' indent
      ;; for you.  Use TAB, C-c C-l or C-c C-r to adjust.  TBD: Is
      ;; there a better way to determine this???
      (if (< (current-indentation) indent) nil
	(goto-char here)
	(beginning-of-line)
	(delete-horizontal-space)
	(indent-to (- indent outdent))
	))))

(defun py-indent-right (arg)
  "Indent the line by one `py-indent-offset' level.
With numeric arg, indent by that many levels.  You cannot indent
farther right than the distance the line would be indented by
\\[py-indent-line]."
  (interactive "p")
  (let ((col (current-indentation))
	(want (* arg py-indent-offset))
	(indent (py-compute-indentation))
	(pos (- (point-max) (point)))
	(bol (save-excursion (beginning-of-line) (point))))
    (if (<= (+ col want) indent)
	(progn
	  (beginning-of-line)
	  (delete-horizontal-space)
	  (indent-to (+ col want))
	  (if (> (- (point-max) pos) (point))
	      (goto-char (- (point-max) pos)))
	  ))))

(defun py-outdent-left (arg)
  "Outdent the line by one `py-indent-offset' level.
With numeric arg, outdent by that many levels.  You cannot outdent
farther left than column zero."
  (interactive "p")
  (let ((col (current-indentation))
	(want (* arg py-indent-offset))
	(pos (- (point-max) (point)))
	(bol (save-excursion (beginning-of-line) (point))))
    (if (<= 0 (- col want))
	(progn
	  (beginning-of-line)
	  (delete-horizontal-space)
	  (indent-to (- col want))
	  (if (> (- (point-max) pos) (point))
	      (goto-char (- (point-max) pos)))
	  ))))


;;; Functions that execute Python commands in a subprocess
(defun py-shell ()
  "Start an interactive Python interpreter in another window.
This is like Shell mode, except that Python is running in the window
instead of a shell.  See the `Interactive Shell' and `Shell Mode'
sections of the Emacs manual for details, especially for the key
bindings active in the `*Python*' buffer.

See the docs for variable `py-scroll-buffer' for info on scrolling
behavior in the process window.

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
  ;; BAW - should undo be disabled in the python process buffer, if
  ;; this bug still exists?
  (interactive)
  (if py-this-is-emacs-19-p
      (progn
	(require 'comint)
	(switch-to-buffer-other-window
	 (make-comint "Python" py-python-command)))
    (progn
      (require 'shell)
      (switch-to-buffer-other-window
       (apply (if (boundp 'make-shell) 'make-shell 'make-comint)
	      "Python" py-python-command nil))))
  (make-local-variable 'shell-prompt-pattern)
  (setq shell-prompt-pattern "^>>> \\|^\\.\\.\\. ")
  (set-process-filter (get-buffer-process (current-buffer))
		      'py-process-filter)
  (set-syntax-table py-mode-syntax-table))

(defun py-execute-region (start end)
  "Send the region between START and END to a Python interpreter.
If there is a *Python* process it is used.

Hint: If you want to execute part of a Python file several times
\(e.g., perhaps you're developing a function and want to flesh it out
a bit at a time), use `\\[narrow-to-region]' to restrict the buffer to
the region of interest, and send the code to a *Python* process via
`\\[py-execute-buffer]' instead.

Following are subtleties to note when using a *Python* process:

If a *Python* process is used, the region is copied into a temporary
file (in directory `py-temp-directory'), and an `execfile' command is
sent to Python naming that file.  If you send regions faster than
Python can execute them, `python-mode' will save them into distinct
temp files, and execute the next one in the queue the next time it
sees a `>>> ' prompt from Python.  Each time this happens, the process
buffer is popped into a window (if it's not already in some window) so
you can see it, and a comment of the form

  \t## working on region in file <name> ...

is inserted at the end.

Caution: No more than 26 regions can be pending at any given time.
This limit is (indirectly) inherited from libc's mktemp(3).
`python-mode' does not try to protect you from exceeding the limit.
It's extremely unlikely that you'll get anywhere close to the limit in
practice, unless you're trying to be a jerk <grin>.

See the `\\[py-shell]' docs for additional warnings."
  (interactive "r")
  (or (< start end) (error "Region is empty"))
  (let ((pyproc (get-process "Python"))
	fname)
    (if (null pyproc)
	(shell-command-on-region start end py-python-command)
      ;; else feed it thru a temp file
      (setq fname (py-make-temp-name))
      (write-region start end fname nil 'no-msg)
      (setq py-file-queue (append py-file-queue (list fname)))
      (if (cdr py-file-queue)
	  (message "File %s queued for execution" fname)
	;; else
	(py-execute-file pyproc fname)))))

(defun py-execute-file (pyproc fname)
  (py-append-to-process-buffer
   pyproc
   (format "## working on region in file %s ...\n" fname))
  (process-send-string pyproc (format "execfile('%s')\n" fname)))

(defun py-process-filter (pyproc string)
  (let ((curbuf (current-buffer))
	(pbuf (process-buffer pyproc))
	(pmark (process-mark pyproc))
	file-finished)

    ;; make sure we switch to a different buffer at least once.  if we
    ;; *don't* do this, then if the process buffer is in the selected
    ;; window, and point is before the end, and lots of output is
    ;; coming at a fast pace, then (a) simple cursor-movement commands
    ;; like C-p, C-n, C-f, C-b, C-a, C-e take an incredibly long time
    ;; to have a visible effect (the window just doesn't get updated,
    ;; sometimes for minutes(!)), and (b) it takes about 5x longer to
    ;; get all the process output (until the next python prompt).
    ;;
    ;; #b makes no sense to me at all.  #a almost makes sense: unless
    ;; we actually change buffers, set_buffer_internal in buffer.c
    ;; doesn't set windows_or_buffers_changed to 1, & that in turn
    ;; seems to make the Emacs command loop reluctant to update the
    ;; display.  Perhaps the default process filter in process.c's
    ;; read_process_output has update_mode_lines++ for a similar
    ;; reason?  beats me ...

    ;; BAW - we want to check to see if this still applies
    (if (eq curbuf pbuf)		; mysterious ugly hack
	(set-buffer (get-buffer-create "*scratch*")))

    (set-buffer pbuf)
    (let* ((start (point))
	   (goback (< start pmark))
	   (goend (and (not goback) (= start (point-max))))
	   (buffer-read-only nil))
      (goto-char pmark)
      (insert string)
      (move-marker pmark (point))
      (setq file-finished
	    (and py-file-queue
		 (equal ">>> "
			(buffer-substring
			 (prog2 (beginning-of-line) (point)
				(goto-char pmark))
			 (point)))))
      (if goback (goto-char start)
	;; else
	(if py-scroll-process-buffer
	    (let* ((pop-up-windows t)
		   (pwin (display-buffer pbuf)))
	      (set-window-point pwin (point)))))
      (set-buffer curbuf)
      (if file-finished
	  (progn
	    (py-delete-file-silently (car py-file-queue))
	    (setq py-file-queue (cdr py-file-queue))
	    (if py-file-queue
		(py-execute-file pyproc (car py-file-queue)))))
      (and goend
	   (progn (set-buffer pbuf)
		  (goto-char (point-max))))
      )))

(defun py-execute-buffer ()
  "Send the contents of the buffer to a Python interpreter.
If there is a *Python* process buffer it is used.  If a clipping
restriction is in effect, only the accessible portion of the buffer is
sent.  A trailing newline will be supplied if needed.

See the `\\[py-execute-region]' docs for an account of some subtleties."
  (interactive)
  (py-execute-region (point-min) (point-max)))



;; Functions for Python style indentation
(defun py-delete-char ()
  "Reduce indentation or delete character.
If point is at the leftmost column, deletes the preceding newline.

Else if point is at the leftmost non-blank character of a line that is
neither a continuation line nor a non-indenting comment line, or if
point is at the end of a blank line, reduces the indentation to match
that of the line that opened the current block of code.  The line that
opened the block is displayed in the echo area to help you keep track
of where you are.

Else the preceding character is deleted, converting a tab to spaces if
needed so that only a single column position is deleted."
  (interactive "*")
  (if (or (/= (current-indentation) (current-column))
	  (bolp)
	  (py-continuation-line-p)
	  (looking-at "#[^ \t\n]"))	; non-indenting #
      (backward-delete-char-untabify 1)
    ;; else indent the same as the colon line that opened the block

    ;; force non-blank so py-goto-block-up doesn't ignore it
    (insert-char ?* 1)
    (backward-char)
    (let ((base-indent 0)		; indentation of base line
	  (base-text "")		; and text of base line
	  (base-found-p nil))
      (condition-case nil		; in case no enclosing block
	  (save-excursion
	    (py-goto-block-up 'no-mark)
	    (setq base-indent (current-indentation)
		  base-text   (py-suck-up-leading-text)
		  base-found-p t))
	(error nil))
      (delete-char 1)			; toss the dummy character
      (delete-horizontal-space)
      (indent-to base-indent)
      (if base-found-p
	  (message "Closes block: %s" base-text)))))

;; required for pending-del and delsel modes
(put 'py-delete-char 'delete-selection 'supersede)
(put 'py-delete-char 'pending-delete   'supersede)

(defun py-indent-line ()
  "Fix the indentation of the current line according to Python rules."
  (interactive)
  (let* ((ci (current-indentation))
	 (move-to-indentation-p (<= (current-column) ci))
	 (need (py-compute-indentation)))
    ;; see if we need to outdent
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

(defun py-compute-indentation ()
  (save-excursion
    (beginning-of-line)
    (cond
     ;; are we on a continuation line?
     ((py-continuation-line-p)
      (let ((startpos (point))
	    (open-bracket-pos (py-nesting-level))
	    endpos searching found)
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
		(if (< (point) startpos)
		    ;; again mimic the first list item
		    (current-indentation)
		  ;; else they're about to enter the first item
		  (goto-char open-bracket-pos)
		  (+ (current-indentation) py-indent-offset))))

	  ;; else on backslash continuation line
	  (forward-line -1)
	  (if (py-continuation-line-p)	; on at least 3rd line in block
	      (current-indentation)	; so just continue the pattern
	    ;; else started on 2nd line in block, so indent more.
	    ;; if base line is an assignment with a start on a RHS,
	    ;; indent to 2 beyond the leftmost "="; else skip first
	    ;; chunk of non-whitespace characters on base line, + 1 more
	    ;; column
	    (end-of-line)
	    (setq endpos (point)  searching t)
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
	    (if (or (not found)		; not an assignment
		    (looking-at "[ \t]*\\\\")) ; <=><spaces><backslash>
		(progn
		  (goto-char startpos)
		  (skip-chars-forward "^ \t\n")))
	    (1+ (current-column))))))

     ;; not on a continuation line

     ;; if at start of restriction, or on a non-indenting comment line,
     ;; assume they intended whatever's there
     ((or (bobp) (looking-at "[ \t]*#[^ \t\n]"))
      (current-indentation))

     ;; else indentation based on that of the statement that precedes
     ;; us; use the first line of that statement to establish the base,
     ;; in case the user forced a non-std indentation for the
     ;; continuation lines (if any)
     (t
      ;; skip back over blank & non-indenting comment lines
      ;; note:  will skip a blank or non-indenting comment line that
      ;; happens to be a continuation line too
      (re-search-backward "^[ \t]*\\([^ \t\n#]\\|#[ \t\n]\\)"
			  nil 'move)
      ;; if we landed inside a string, go to the beginning of that
      ;; string. this handles triple quoted, multi-line spanning
      ;; strings.
      (let ((state (parse-partial-sexp
		    (save-excursion (beginning-of-python-def-or-class)
				    (point))
		    (point))))
	(if (nth 3 state)
	    (goto-char (nth 2 state))))
      (py-goto-initial-line)
      (if (py-statement-opens-block-p)
	  (+ (current-indentation) py-indent-offset)
	(current-indentation))))))

(defun py-guess-indent-offset (&optional global)
  "Guess a good value for, and change, `py-indent-offset'.
By default (without a prefix arg), makes a buffer-local copy of
`py-indent-offset' with the new value.  This will not affect any other
Python buffers.  With a prefix arg, changes the global value of
`py-indent-offset'.  This affects all Python buffers (that don't have
their own buffer-local copy), both those currently existing and those
created later in the Emacs session.

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
	restart
	(found nil)
	colon-indent)
    (py-goto-initial-line)
    (while (not (or found (eobp)))
      (if (re-search-forward ":[ \t]*\\($\\|[#\\]\\)" nil 'move)
	  (progn
	    (setq restart (point))
	    (py-goto-initial-line)
	    (if (py-statement-opens-block-p)
		(setq found t)
	      (goto-char restart)))))
    (if found
	()
      (goto-char start)
      (py-goto-initial-line)
      (while (not (or found (bobp)))
	(setq found
	      (and
	       (re-search-backward ":[ \t]*\\($\\|[#\\]\\)" nil 'move)
	       (or (py-goto-initial-line) t) ; always true -- side effect
	       (py-statement-opens-block-p)))))
    (setq colon-indent (current-indentation)
	  found (and found (zerop (py-next-statement 1)))
	  new-value (- (current-indentation) colon-indent))
    (goto-char start)
    (if found
	(progn
	  (funcall (if global 'kill-local-variable 'make-local-variable)
		   'py-indent-offset)
	  (setq py-indent-offset new-value)
	  (message "%s value of py-indent-offset set to %d"
		   (if global "Global" "Local")
		   py-indent-offset))
      (error "Sorry, couldn't guess a value for py-indent-offset"))))

(defun py-shift-region (start end count)
  (save-excursion
    (goto-char end)   (beginning-of-line) (setq end (point))
    (goto-char start) (beginning-of-line) (setq start (point))
    (indent-rigidly start end count)))

(defun py-shift-region-left (start end &optional count)
  "Shift region of Python code to the left.
The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
shifted to the left, by `py-indent-offset' columns.

If a prefix argument is given, the region is instead shifted by that
many columns."
  (interactive "*r\nP")   ; region; raw prefix arg
  (py-shift-region start end
		   (- (prefix-numeric-value
		       (or count py-indent-offset)))))

(defun py-shift-region-right (start end &optional count)
  "Shift region of Python code to the right.
The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
shifted to the right, by `py-indent-offset' columns.

If a prefix argument is given, the region is instead shifted by that
many columns."
  (interactive "*r\nP")   ; region; raw prefix arg
  (py-shift-region start end (prefix-numeric-value
			      (or count py-indent-offset))))

(defun py-indent-region (start end &optional indent-offset)
  "Reindent a region of Python code.
The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
reindented.  If the first line of the region has a non-whitespace
character in the first column, the first line is left alone and the
rest of the region is reindented with respect to it.  Else the entire
region is reindented with respect to the (closest code or
indenting-comment) statement immediately preceding the region.

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
			   (py-compute-indentation)
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


;; Functions for moving point
(defun py-previous-statement (count)
  "Go to the start of previous Python statement.
If the statement at point is the i'th Python statement, goes to the
start of statement i-COUNT.  If there is no such statement, goes to the
first statement.  Returns count of statements left to move.
`Statements' do not include blank, comment, or continuation lines."
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

(defun beginning-of-python-def-or-class (&optional class)
  "Move point to start of def (or class, with prefix arg).

Searches back for the closest preceding `def'.  If you supply a prefix
arg, looks for a `class' instead.  The docs assume the `def' case;
just substitute `class' for `def' for the other case.

If point is in a def statement already, and after the `d', simply
moves point to the start of the statement.

Else (point is not in a def statement, or at or before the `d' of a
def statement), searches for the closest preceding def statement, and
leaves point at its start.  If no such statement can be found, leaves
point at the start of the buffer.

Returns t iff a def statement is found by these rules.

Note that doing this command repeatedly will take you closer to the
start of the buffer each time.

If you want to mark the current def/class, see
`\\[mark-python-def-or-class]'."
  (interactive "P")			; raw prefix arg
  (let ((at-or-before-p (<= (current-column) (current-indentation)))
	(start-of-line (progn (beginning-of-line) (point)))
	(start-of-stmt (progn (py-goto-initial-line) (point))))
    (if (or (/= start-of-stmt start-of-line)
	    (not at-or-before-p))
	(end-of-line))			; OK to match on this line
    (re-search-backward (if class "^[ \t]*class\\>" "^[ \t]*def\\>")
			nil 'move)))

(defun end-of-python-def-or-class (&optional class)
  "Move point beyond end of def (or class, with prefix arg) body.

By default, looks for an appropriate `def'.  If you supply a prefix arg,
looks for a `class' instead.  The docs assume the `def' case; just
substitute `class' for `def' for the other case.

If point is in a def statement already, this is the def we use.

Else if the def found by `\\[beginning-of-python-def-or-class]'
contains the statement you started on, that's the def we use.

Else we search forward for the closest following def, and use that.

If a def can be found by these rules, point is moved to the start of
the line immediately following the def block, and the position of the
start of the def is returned.

Else point is moved to the end of the buffer, and nil is returned.

Note that doing this command repeatedly will take you closer to the
end of the buffer each time.

If you want to mark the current def/class, see
`\\[mark-python-def-or-class]'."
  (interactive "P")			; raw prefix arg
  (let ((start (progn (py-goto-initial-line) (point)))
	(which (if class "class" "def"))
	(state 'not-found))
    ;; move point to start of appropriate def/class
    (if (looking-at (concat "[ \t]*" which "\\>")) ; already on one
	(setq state 'at-beginning)
      ;; else see if beginning-of-python-def-or-class hits container
      (if (and (beginning-of-python-def-or-class class)
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
     (t (error "internal error in end-of-python-def-or-class")))))


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
	      (> (current-indentation) initial-indent))
	nil))

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

(defun mark-python-def-or-class (&optional class)
  "Set region to body of def (or class, with prefix arg) enclosing point.
Pushes the current mark, then point, on the mark ring (all language
modes do this, but although it's handy it's never documented ...).

In most Emacs language modes, this function bears at least a
hallucinogenic resemblance to `\\[end-of-python-def-or-class]' and
`\\[beginning-of-python-def-or-class]'.

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
	(which (if class "class" "def")))
    (push-mark start)
    (if (not (py-go-up-tree-to-keyword which))
	(progn (goto-char start)
	       (error "Enclosing %s not found" which))
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
		  (goto-char start))))))))

(defun py-comment-region (start end &optional uncomment-p)
  "Comment out region of code; with prefix arg, uncomment region.
The lines from the line containing the start of the current region up
to (but not including) the line containing the end of the region are
commented out, by inserting the string `py-block-comment-prefix' at
the start of each line.  With a prefix arg, removes
`py-block-comment-prefix' from the start of each line instead."
  (interactive "*r\nP")   ; region; raw prefix arg
  (goto-char end)   (beginning-of-line) (setq end (point))
  (goto-char start) (beginning-of-line) (setq start (point))
  (let ((prefix-len (length py-block-comment-prefix)) )
    (save-excursion
      (save-restriction
	(narrow-to-region start end)
	(while (not (eobp))
	  (if uncomment-p
	      (and (string= py-block-comment-prefix
			    (buffer-substring
			     (point) (+ (point) prefix-len)))
		   (delete-char prefix-len))
	    (insert py-block-comment-prefix))
	  (forward-line 1))))))


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
	  (setq funcdoc (substitute-command-keys
			 (get func 'variable-documentation))
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

\\[py-execute-buffer]\tsends the entire buffer to the Python interpreter
\\[py-execute-region]\tsends the current region
\\[py-shell]\tstarts a Python interpreter window; this will be used by
\tsubsequent \\[py-execute-buffer] or \\[py-execute-region] commands
%c:py-execute-buffer
%c:py-execute-region
%c:py-shell

@VARIABLES

py-indent-offset\tindentation increment
py-block-comment-prefix\tcomment string used by py-comment-region

py-python-command\tshell command to invoke Python interpreter
py-scroll-process-buffer\talways scroll Python process buffer
py-temp-directory\tdirectory used for temp files (if needed)

py-beep-if-tab-change\tring the bell if tab-width is changed
%v:py-indent-offset
%v:py-block-comment-prefix
%v:py-python-command
%v:py-scroll-process-buffer
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
\t\\[py-delete-char]\t reduce indentation, or delete single character

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
\\[py-delete-char] to reduce it.

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
%c:py-delete-char


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
\\[mark-python-def-or-class]\t mark smallest enclosing def
\\[universal-argument] \\[mark-python-def-or-class]\t mark smallest enclosing class
\\[py-comment-region]\t comment out region of code
\\[universal-argument] \\[py-comment-region]\t uncomment region of code
%c:py-mark-block
%c:mark-python-def-or-class
%c:py-comment-region

@MOVING POINT

\\[py-previous-statement]\t move to statement preceding point
\\[py-next-statement]\t move to statement following point
\\[py-goto-block-up]\t move up to start of current block
\\[beginning-of-python-def-or-class]\t move to start of def
\\[universal-argument] \\[beginning-of-python-def-or-class]\t move to start of class
\\[end-of-python-def-or-class]\t move to end of def
\\[universal-argument] \\[end-of-python-def-or-class]\t move to end of class

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
%c:beginning-of-python-def-or-class
%c:end-of-python-def-or-class

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


;; Helper functions
(defvar py-parse-state-re
  (concat
   "^[ \t]*\\(if\\|elif\\|else\\|while\\|def\\|class\\)\\>"
   "\\|"
   "^[^ #\t\n]"))

;; returns the parse state at point (see parse-partial-sexp docs)
(defun py-parse-state ()
  (save-excursion
    (let ((here (point)) )
      ;; back up to the first preceding line (if any; else start of
      ;; buffer) that begins with a popular Python keyword, or a non-
      ;; whitespace and non-comment character.  These are good places
      ;; to start parsing to see whether where we started is at a
      ;; non-zero nesting level.  It may be slow for people who write
      ;; huge code blocks or huge lists ... tough beans.
      (re-search-backward py-parse-state-re nil 'move)
      (beginning-of-line)
      (parse-partial-sexp (point) here))))

;; if point is at a non-zero nesting level, returns the number of the
;; character that opens the smallest enclosing unclosed list; else
;; returns nil.
(defun py-nesting-level ()
  (let ((status (py-parse-state)) )
    (if (zerop (car status))
	nil				; not in a nest
      (car (cdr status)))))		; char# of open bracket

;; t iff preceding line ends with backslash that's not in a comment
(defun py-backslash-continuation-line-p ()
  (save-excursion
    (beginning-of-line)
    (and
     ;; use a cheap test first to avoid the regexp if possible
     ;; use 'eq' because char-after may return nil
     (eq (char-after (- (point) 2)) ?\\ )
     ;; make sure; since eq test passed, there is a preceding line
     (forward-line -1)			; always true -- side effect
     (looking-at py-continued-re))))

;; t iff current line is a continuation line
(defun py-continuation-line-p ()
  (save-excursion
    (beginning-of-line)
    (or (py-backslash-continuation-line-p)
	(py-nesting-level))))

;; go to initial line of current statement; usually this is the line
;; we're on, but if we're on the 2nd or following lines of a
;; continuation block, we need to go up to the first line of the
;; block.
;;
;; Tricky: We want to avoid quadratic-time behavior for long continued
;; blocks, whether of the backslash or open-bracket varieties, or a
;; mix of the two.  The following manages to do that in the usual
;; cases.
(defun py-goto-initial-line ()
  (let ( open-bracket-pos )
    (while (py-continuation-line-p)
      (beginning-of-line)
      (if (py-backslash-continuation-line-p)
	  (while (py-backslash-continuation-line-p)
	    (forward-line -1))
	;; else zip out of nested brackets/braces/parens
	(while (setq open-bracket-pos (py-nesting-level))
	  (goto-char open-bracket-pos)))))
  (beginning-of-line))

;; go to point right beyond final line of current statement; usually
;; this is the start of the next line, but if this is a multi-line
;; statement we need to skip over the continuation lines.  Tricky:
;; Again we need to be clever to avoid quadratic time behavior.
(defun py-goto-beyond-final-line ()
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
	    ;; BUG ALERT: I could swear, from reading the docs, that
	    ;; the 3rd argument should be plain 0
	    (parse-partial-sexp (point) (point-max) (- 0 (car state))
				nil state)
	    (forward-line 1))))))

;; t iff statement opens a block == iff it ends with a colon that's
;; not in a comment.  point should be at the start of a statement
(defun py-statement-opens-block-p ()
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

;; go to point right beyond final line of block begun by the current
;; line.  This is the same as where py-goto-beyond-final-line goes
;; unless we're on colon line, in which case we go to the end of the
;; block.  assumes point is at bolp
(defun py-goto-beyond-block ()
  (if (py-statement-opens-block-p)
      (py-mark-block nil 'just-move)
    (py-goto-beyond-final-line)))

;; go to start of first statement (not blank or comment or
;; continuation line) at or preceding point.  returns t if there is
;; one, else nil
(defun py-goto-statement-at-or-above ()
  (py-goto-initial-line)
  (if (looking-at py-blank-or-comment-re)
      ;; skip back over blank & comment lines
      ;; note:  will skip a blank or comment line that happens to be
      ;; a continuation line too
      (if (re-search-backward "^[ \t]*[^ \t#\n]" nil t)
	  (progn (py-goto-initial-line) t)
	nil)
    t))

;; go to start of first statement (not blank or comment or
;; continuation line) following the statement containing point returns
;; t if there is one, else nil
(defun py-goto-statement-below ()
  (beginning-of-line)
  (let ((start (point)))
    (py-goto-beyond-final-line)
    (while (and
	    (looking-at py-blank-or-comment-re)
	    (not (eobp)))
      (forward-line 1))
    (if (eobp)
	(progn (goto-char start) nil)
      t)))

;; go to start of statement, at or preceding point, starting with
;; keyword KEY.  Skips blank lines and non-indenting comments upward
;; first.  If that statement starts with KEY, done, else go back to
;; first enclosing block starting with KEY.  If successful, leaves
;; point at the start of the KEY line & returns t.  Else leaves point
;; at an undefined place & returns nil.
(defun py-go-up-tree-to-keyword (key)
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

;; return string in buffer from start of indentation to end of line;
;; prefix "..." if leading whitespace was skipped
(defun py-suck-up-leading-text ()
  (save-excursion
    (back-to-indentation)
    (concat
     (if (bolp) "" "...")
     (buffer-substring (point) (progn (end-of-line) (point))))))

;; assuming point at bolp, return first keyword ([a-z]+) on the line,
;; as a Lisp symbol; return nil if none
(defun py-suck-up-first-keyword ()
  (let ((case-fold-search nil))
    (if (looking-at "[ \t]*\\([a-z]+\\)\\b")
	(intern (buffer-substring (match-beginning 1) (match-end 1)))
      nil)))

(defun py-make-temp-name ()
  (make-temp-name
   (concat (file-name-as-directory py-temp-directory) "python")))

(defun py-delete-file-silently (fname)
  (condition-case nil
      (delete-file fname)
    (error nil)))

(defun py-kill-emacs-hook ()
  ;; delete our temp files
  (while py-file-queue
    (py-delete-file-silently (car py-file-queue))
    (setq py-file-queue (cdr py-file-queue)))
  (if (not (or py-this-is-lucid-emacs-p py-this-is-emacs-19-p))
      ;; run the hook we inherited, if any
      (and py-inherited-kill-emacs-hook
	   (funcall py-inherited-kill-emacs-hook))))

;; make PROCESS's buffer visible, append STRING to it, and force
;; display; also make shell-mode believe the user typed this string,
;; so that kill-output-from-shell and show-output-from-shell work
;; "right"
(defun py-append-to-process-buffer (process string)
  (let ((cbuf (current-buffer))
	(pbuf (process-buffer process))
	(py-scroll-process-buffer t))
    (set-buffer pbuf)
    (goto-char (point-max))
    (move-marker (process-mark process) (point))
    (if (not (or py-this-is-emacs-19-p
		 py-this-is-lucid-emacs-p))
	(move-marker last-input-start (point))) ; muck w/ shell-mode
    (funcall (process-filter process) process string)
    (if (not (or py-this-is-emacs-19-p
		 py-this-is-lucid-emacs-p))
	(move-marker last-input-end (point))) ; muck w/ shell-mode
    (set-buffer cbuf))
  (sit-for 0))

(defun py-keep-region-active ()
  ;; do whatever is necessary to keep the region active in XEmacs.
  ;; Ignore byte-compiler warnings you might see.  Also note that
  ;; FSF's Emacs 19 does it differently and doesn't its policy doesn't
  ;; require us to take explicit action.
  (and (boundp 'zmacs-region-stays)
       (setq zmacs-region-stays t)))


(defconst py-version "2.26"
  "`python-mode' version number.")
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
With \\[universal-argument] just submit an enhancement request."
  (interactive
   (list (not (y-or-n-p
	       "Is this a bug report? (hit `n' to send other comments) "))))
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
	 py-scroll-process-buffer
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


;; arrange to kill temp files when Emacs exists
(if (or py-this-is-emacs-19-p py-this-is-lucid-emacs-p)
    (add-hook 'kill-emacs-hook 'py-kill-emacs-hook)
  ;; have to trust that other people are as respectful of our hook
  ;; fiddling as we are of theirs
  (if (boundp 'py-inherited-kill-emacs-hook)
      ;; we were loaded before -- trust others not to have screwed us
      ;; in the meantime (no choice, really)
      nil
    ;; else arrange for our hook to run theirs
    (setq py-inherited-kill-emacs-hook kill-emacs-hook)
    (setq kill-emacs-hook 'py-kill-emacs-hook)))



(provide 'python-mode)
;;; python-mode.el ends here
