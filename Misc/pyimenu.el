;;; PYIMENU.EL --- 

;; Copyright (C) 1995 Perry A. Stoll

;; Author: Perry A. Stoll <stoll@atr-sw.atr.co.jp>
;; Created: 12 May 1995
;; Version: 1.0
;; Keywords: tools python imenu

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; A copy of the GNU General Public License can be obtained from the
;; Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
;; USA.

;;;; COMMENTS

;; I use the imenu package a lot for looking at Lisp and C/C++
;; code. When I started using Python, I was dismayed that I couldn't
;; use it to look at Python source. So here's a rough go at it.

;;;; USAGE

;; This program is used in conjunction with the imenu package. When
;; you call imenu in python-mode in a python buffer, a list of
;; functions and classes is built. The top level menu has a list of
;; all functions and classes defined at the top indentation
;; level. Classes which have methods defined in them have a submenu
;; which contains all definitions contained in them. Selecting any
;; item will bring you to that point in the file.

;;;; INSTALLATION

;; You know the routine:
;; 1) Save this file as pyimenu.el,
;; 2) Place that file somewhere in your emacs load path (maybe ~/emacs
;;    or ~/emacs/lisp),
;; 3) Byte compile it (M-x byte-compile-file),
;; 4) Add the following (between "cut here" and "end here") to your
;;    ~/.emacs file,
;; 5) Reboot. (joke: DON'T do that, although you'll probably have to
;;    either reload your ~/.emacs file or start a new emacs)

;;--------------cut here-------------------------------------------
;;;; Load the pyimenu index function
;;(autoload 'imenu "imenu" nil t)
;;(autoload 'imenu-example--create-python-index "pyimenu")
;;;; Add the index creation function to the python-mode-hook 
;;(add-hook 'python-mode-hook
;;	  (function
;;	   (lambda ()
;;	     (setq imenu-create-index-function
;;		   (function imenu-example--create-python-index)))))
;;----------------end here--------------------------------------------
;;
;; That is all you need. Of course, the following provides a more
;; useful interface. i.e. this is how I have it set up ;-)
;;
;;----------------optionally cut here----------------------------------
;;(autoload 'imenu-add-to-menubar "imenu" nil t)
;;(defun my-imenu-install-hook ()
;;  (imenu-add-to-menubar (format "%s-%s" "IM" mode-name)))
;;(add-hook 'python-mode-hook (function my-imenu-install-hook))
;;;; Bind imenu to some convenient (?) mouse key. This really lets you
;;;; fly around the buffer. Here it is set to Meta-Shift-Mouse3Click.
;;(global-set-key [M-S-down-mouse-3] (function imenu))
;;-----------------optionaly end here-----------------------------------

;;;; CAVEATS/NOTES

;; 0) I'm not a professional elisp programmer and it shows in the code
;;    below. If anyone there has suggestions/changes, I'd love to
;;    hear them. I've tried the code out on a bunch of python files
;;    from the python-1.1.1 Demo distribution and it worked with
;;    them - your mileage may very.
;;
;; 1) You must have the imenu package to use this file. This file
;;    works with imenu version 1.11 (the version included with emacs
;;    19.28) and imenu version 1.14; if you have a later version, this
;;    may not work with it.
;;
;; 2) This setup assumes you also have python-mode.el, so that it can
;;    use the python-mode-hook. It comes with the python distribution.
;;
;; 3) I don't have the Python 1.2 distribution yet, so again, this may
;;    not work with that.
;;

(require 'imenu)

;;;
;;; VARIABLES: customizable in your .emacs file.
;;;

(defvar imenu-example--python-show-method-args-p nil 
  "*When using imenu package with python-mode, whether the arguments of
the function/methods should be printed in the imenu buffer in addition
to the function/method name. If non-nil, args are printed.")

;;;
;;; VARIABLES: internal use.
;;;
(defvar imenu-example--python-class-regexp
  (concat                              ; <<classes>>
   "\\("                               ;
   "^[ \t]*"                           ; newline and maybe whitespace
   "\\(class[ \t]+[a-zA-Z0-9_]+\\)"    ; class name
                                       ; possibly multiple superclasses
   "\\([ \t]*\\((\\([a-zA-Z0-9_, \t\n]\\)*)\\)?\\)"
   "[ \t]*:"                           ; and the final :
   "\\)"                               ; >>classes<<
   )
  "Regexp for Python classes for use with the imenu package."
)

(defvar imenu-example--python-method-regexp
  (concat                               ; <<methods and functions>>
   "\\("                                ; 
   "^[ \t]*"                            ; new line and maybe whitespace
   "\\(def[ \t]+"                       ; function definitions start with def
   "\\([a-zA-Z0-9_]+\\)"                ;   name is here
					;   function arguments...
   "[ \t]*(\\([a-zA-Z0-9_=,\* \t\n]*\\))"
   "\\)"                                ; end of def
   "[ \t]*:"                            ; and then the :
   "\\)"                                ; >>methods and functions<<
   )
  "Regexp for Python methods/functions for use with the imenu package."
  )

(defvar imenu-example--python-method-no-arg-parens '(2 8)
  "Indicies into the parenthesis list of the regular expression for
python for use with imenu. Using these values will result in smaller
imenu lists, as arguments to functions are not listed.

See the variable imenu-example--python-show-method-args-p to for
information")

(defvar imenu-example--python-method-arg-parens '(2 7)
  "Indicies into the parenthesis list of the regular expression for
python for use with imenu. Using these values will result in large
imenu lists, as arguments to functions are listed.

See the variable imenu-example--python-show-method-args-p to for
information")

;; Note that in this format, this variable can still be used with the
;; imenu--generic-function. Otherwise, there is no real reason to have
;; it.
(defvar imenu-example--generic-python-expression
  (cons
   (concat 
    imenu-example--python-class-regexp
    "\\|"  ; or...
    imenu-example--python-method-regexp
    )
   imenu-example--python-method-no-arg-parens)
  "Generic Python expression which may be used directly with imenu by
setting the variable imenu-generic-expression to this value. Also, see
the function \\[imenu-example--create-python-index] for an alternate
way of finding the index.")

;; These next two variables are used when searching for the python
;; class/definitions. Just saving some time in accessing the
;; generic-python-expression, really.
(defvar imenu-example--python-generic-regexp)
(defvar imenu-example--python-generic-parens)

;;;
;;; CODE:
;;;

;; Note:
;; At first, I tried using some of the functions supplied by
;; python-mode to navigate through functions and classes, but after a
;; while, I decided dump it. This file is relatively self contained
;; and I liked it that.

;;;###autoload
(defun imenu-example--create-python-index ()
  "Interface function for imenu package to find all python classes and
functions/methods. Calls function
\\[imenu-example--create-python-index-engine]. See that function for
the details of how this works."
  (setq imenu-example--python-generic-regexp
	(car imenu-example--generic-python-expression))
  (setq imenu-example--python-generic-parens
	(if imenu-example--python-show-method-args-p
	    imenu-example--python-method-arg-parens
	  imenu-example--python-method-no-arg-parens))
  (goto-char (point-min))
  (imenu-example--create-python-index-engine nil))

(defun imenu-example--create-python-index-engine (&optional start-indent)
"Function for finding all definitions (classes, methods, or functions)
in a python file for the imenu package. 

Retuns a possibly nested alist of the form \(INDEX-NAME
 INDEX-POSITION). The second element of the alist may be an alist,
producing a nested list as in \(INDEX-NAME . INDEX-ALIST).

This function should not be called directly, as it calls itself
recursively and requires some setup. Rather this is the engine for the
function \\[imenu-example--create-python-index].

It works recursively by looking for all definitions at the current
indention level. When it finds one, it adds it to the alist. If it
finds a definition at a greater indentation level, it removes the
previous definition from the alist. In it's place it adds all
definitions found at the next indentation level. When it finds a
definition that is less indented then the current level, it retuns the
alist it has created thus far.

 The optional argument START-INDENT indicates the starting indentation
at which to continue looking for python classes, methods, or
functions. If this is not supplied, the function uses the indentation
of the first definition found. "
  (let ((index-alist '())
	(sub-method-alist '())
	looking-p
	def-name prev-name
	cur-indent def-pos
	(class-paren (first  imenu-example--python-generic-parens)) 
	(def-paren   (second imenu-example--python-generic-parens)))
    (setq looking-p
	  (re-search-forward imenu-example--python-generic-regexp
			     (point-max) t))
    (while looking-p
      (save-excursion
	;; used to set def-name to this value but generic-extract-name is
	;; new to imenu-1.14. this way it still works with imenu-1.11
	;;(imenu--generic-extract-name imenu-example--python-generic-parens))
	(let ((cur-paren (if (match-beginning class-paren)
			     class-paren def-paren)))
	  (setq def-name
		(buffer-substring (match-beginning cur-paren)
				  (match-end  cur-paren))))
	(beginning-of-line)
	(setq cur-indent (current-indentation)))

      ;; HACK: want to go to the correct definition location. Assuming
      ;; here that there are only two..which is true for python.
      (setq def-pos
	    (or  (match-beginning class-paren)
		 (match-beginning def-paren)))

      ; if we don't have a starting indent level, take this one
      (or start-indent
	  (setq start-indent cur-indent))

      ; if we don't have class name yet, take this one
      (or prev-name
	  (setq prev-name def-name))

      ;; what level is the next definition on? 
      ;; must be same, deeper or shallower indentation
      (cond

       ;; at the same indent level, add it to the list...
       ((= start-indent cur-indent)
	(push (cons def-name def-pos) index-alist))

       ;; deeper indented expression, recur...
       ((< start-indent cur-indent)

	;; the point is currently on the expression we're supposed to
	;; start on, so go back to the last expression. The recursive
	;; call will find this place again and add it to the correct
	;; list
	(re-search-backward imenu-example--python-generic-regexp
			    (point-min) 'move)
	(setq sub-method-alist (imenu-example--create-python-index-engine
				cur-indent))

	(if sub-method-alist
	    ;; we put the last element on the index-alist on the start
	    ;; of the submethod alist so the user can still get to it.
	    (let ((save-elmt (pop index-alist)))
	      (push (cons (imenu-create-submenu-name prev-name)
			  (cons save-elmt sub-method-alist))
		    index-alist))))

       ;; found less indented expression, we're done.
       (t 
	(setq looking-p nil)
	(re-search-backward imenu-example--python-generic-regexp 
			    (point-min) t)))
      (setq prev-name def-name)
      (and looking-p
	   (setq looking-p
		 (re-search-forward imenu-example--python-generic-regexp
				    (point-max) 'move))))
    (nreverse index-alist)))

(provide 'pyimenu)

;;; PyImenu.EL ends here
