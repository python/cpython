;;; py2texi.el -- Conversion of Python LaTeX documentation to Texinfo

;; Copyright (C) 1998, 1999, 2001, 2002 Milan Zamazal

;; Author: Milan Zamazal <pdm@zamazal.org>
;; Version: $Id$
;; Keywords: python

;; COPYRIGHT NOTICE
;;
;; This program is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the Free
;; Software Foundation; either version 2, or (at your option) any later
;; version.
;;
;; This program is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.
;;
;; You can find the GNU General Public License at
;; http://www.gnu.org/copyleft/gpl.html
;; or you can write to the Free Software Foundation, Inc., 59 Temple Place,
;; Suite 330, Boston, MA 02111-1307, USA.

;;; Commentary:

;; This is a Q&D hack for conversion of Python manuals to on-line help format.
;; I desperately needed usable online documenta for Python, so I wrote this.
;; The result code is ugly and need not contain complete information from
;; Python manuals.  I apologize for my ignorance, especially ignorance to
;; python.sty.  Improvements of this convertor are welcomed.

;; How to use it:
;; Load this file and apply `M-x py2texi'.  You will be asked for name of a
;; file to be converted.

;; Where to find it:
;; New versions of this code might be found at
;; http://www.zamazal.org/software/python/py2texi/ .

;;; Code:


(require 'texinfo)
(eval-when-compile
  (require 'cl))


(defvar py2texi-python-version "2.2"
  "What to substitute for the \\version macro.")

(defvar py2texi-python-short-version
  (progn
    (string-match "[0-9]+\\.[0-9]+" py2texi-python-version)
    (match-string 0 py2texi-python-version))
  "Short version number, usually set by the LaTeX commands.")

(defvar py2texi-stop-on-problems nil
  "*If non-nil, stop when you encouter soft problem.")

(defconst py2texi-environments
  '(("abstract" 0 "@quotation" "@end quotation\n")
    ("center" 0 "" "")
    ("cfuncdesc" 3
     (progn (setq findex t)
	    "\n@table @code\n@item \\1 \\2(\\3)\n@findex \\2\n")
     "@end table")
    ("classdesc" 2
     (progn (setq obindex t)
	    "\n@table @code\n@item \\1(\\2)\n@obindex \\1\n")
     "@end table")
    ("classdesc*" 1
     (progn (setq obindex t)
	    "\n@table @code\n@item \\1\n@obindex \\1\n")
     "@end table")
    ("cmemberdesc" 3
     (progn (setq findex t)
	    "\n@table @code\n@item \\1 \\2 \\3\n@findex \\3\n")
     "@end table")
    ("csimplemacrodesc" 1
     (progn (setq cindex t)
	    "\n@table @code\n@item \\1\n@cindex \\1\n")
     "@end table")
    ("ctypedesc" 1
     (progn (setq cindex t)
	    "\n@table @code\n@item \\1\n@cindex \\1\n")
     "@end table")
    ("cvardesc" 2
     (progn (setq findex t)
	    "\n@table @code\n@item \\1 \\2\n@findex \\2\n")
     "@end table")
    ("datadesc" 1
     (progn (setq findex t)
	    "\n@table @code\n@item \\1\n@findex \\1\n")
     "@end table")
    ("datadescni" 1 "\n@table @code\n@item \\1\n" "@end table")
    ("definitions" 0 "@table @dfn" "@end table\n")
    ("description" 0 "@table @samp" "@end table\n")
    ("displaymath" 0 "" "")
    ("document" 0
     (concat "@defcodeindex mo\n"
	     "@defcodeindex ob\n"
	     "@titlepage\n"
	     (format "@title " title "\n")
	     (format "@author " author "\n")
	     "@page\n"
	     author-address
	     "@end titlepage\n"
	     "@node Top, , , (dir)\n")
     (concat "@indices\n"
	     "@contents\n"
	     "@bye\n"))
    ("enumerate" 0 "@enumerate" "@end enumerate")
    ("excdesc" 1
     (progn (setq obindex t)
	    "\n@table @code\n@item \\1\n@obindex \\1\n")
     "@end table")
    ("excclassdesc" 2
     (progn (setq obindex t)
	    "\n@table @code\n@item \\1(\\2)\n@obindex \\1\n")
     "@end table")
    ("flushleft" 0 "" "")
    ("fulllineitems" 0 "\n@table @code\n" "@end table")
    ("funcdesc" 2
     (progn (setq findex t)
	    "\n@table @code\n@item \\1(\\2)\n@findex \\1\n")
     "@end table")
    ("funcdescni" 2 "\n@table @code\n@item \\1(\\2)\n" "@end table")
    ("itemize" 0 "@itemize @bullet" "@end itemize\n")
    ("list" 2 "\n@table @code\n" "@end table")
    ("longtableii" 4 (concat "@multitable @columnfractions .5 .5\n"
			     "@item \\3 @tab \\4\n"
			     "@item ------- @tab ------ \n")
     "@end multitable\n")
    ("longtableiii" 5 (concat "@multitable @columnfractions .33 .33 .33\n"
			      "@item \\3 @tab \\4 @tab \\5\n"
			      "@item ------- @tab ------ @tab ------\n")
     "@end multitable\n")
    ("memberdesc" 1
     (progn (setq findex t)
	    "\n@table @code\n@item \\1\n@findex \\1\n")
     "@end table")
    ("memberdescni" 1 "\n@table @code\n@item \\1\n" "@end table")
    ("methoddesc" 2
     (progn (setq findex t)
	    "\n@table @code\n@item \\1(\\2)\n@findex \\1\n")
     "@end table")
    ("methoddescni" 2 "\n@table @code\n@item \\1(\\2)\n" "@end table")
    ("notice" 0 "@emph{Notice:} " "")
    ("opcodedesc" 2
     (progn (setq findex t)
	    "\n@table @code\n@item \\1 \\2\n@findex \\1\n")
     "@end table")
    ("productionlist" 0 "\n@table @code\n" "@end table")
    ("quotation" 0 "@quotation" "@end quotation")
    ("seealso" 0 "See also:\n@table @emph\n" "@end table")
    ("seealso*" 0 "@table @emph\n" "@end table")
    ("sloppypar" 0 "" "")
    ("small" 0 "" "")
    ("tableii" 4 (concat "@multitable @columnfractions .5 .5\n"
			 "@item \\3 @tab \\4\n"
			 "@item ------- @tab ------ \n")
     "@end multitable\n")
    ("tableiii" 5 (concat "@multitable @columnfractions .33 .33 .33\n"
			 "@item \\3 @tab \\4 @tab \\5\n"
			 "@item ------- @tab ------ @tab ------\n")
     "@end multitable\n")
    ("tableiv" 6 (concat
		  "@multitable @columnfractions .25 .25 .25 .25\n"
		  "@item \\3 @tab \\4 @tab \\5 @tab \\6\n"
		  "@item ------- @tab ------- @tab ------- @tab -------\n")
     "@end multitable\n")
    ("tablev" 7 (concat
		 "@multitable @columnfractions .20 .20 .20 .20 .20\n"
		 "@item \\3 @tab \\4 @tab \\5 @tab \\6 @tab \\7\n"
		 "@item ------- @tab ------- @tab ------- @tab ------- @tab -------\n")
     "@end multitable\n"))
  "Associative list defining substitutions for environments.
Each list item is of the form (ENVIRONMENT ARGNUM BEGIN END) where:
- ENVIRONMENT is LaTeX environment name
- ARGNUM is number of (required) macro arguments
- BEGIN is substitution for \begin{ENVIRONMENT}
- END is substitution for \end{ENVIRONMENT}
Both BEGIN and END are evaled.  Moreover, you can reference arguments through
\N regular expression notation in strings of BEGIN.")

(defconst py2texi-commands
  '(("ABC" 0 "ABC")
    ("appendix" 0 (progn (setq appendix t) ""))
    ("ASCII" 0 "ASCII")
    ("author" 1 (progn (setq author (match-string 1 string)) ""))
    ("authoraddress" 1
     (progn (setq author-address (match-string 1 string)) ""))
    ("b" 1 "@w{\\1}")
    ("bf" 0 "@destroy")
    ("bifuncindex" 1 (progn (setq findex t) "@findex{\\1}"))
    ("C" 0 "C")
    ("c" 0 "@,")
    ("catcode" 0 "")
    ("cdata" 1 "@code{\\1}")
    ("centerline" 1 "@center \\1")
    ("cfuncline" 3
     (progn (setq findex t)
	    "\n@item \\1 \\2(\\3)\n@findex \\2\n"))
    ("cfunction" 1 "@code{\\1}")
    ("chapter" 1 (format "@node \\1\n@%s \\1\n"
			 (if appendix "appendix" "chapter")))
    ("chapter*" 1 "@node \\1\n@unnumbered \\1\n")
    ("character" 1 "@samp{\\1}")
    ("citetitle" 1 "@ref{Top,,,\\1}")
    ("class" 1 "@code{\\1}")
    ("cmemberline" 3
     (progn (setq findex t)
	    "\n@item \\1 \\2 \\3\n@findex \\3\n"))
    ("code" 1 "@code{\\1}")
    ("command" 1 "@command{\\1}")
    ("constant" 1 "@code{\\1}")
    ("copyright" 1 "@copyright{}")
    ("Cpp" 0 "C++")
    ("csimplemacro" 1 "@code{\\1}")
    ("ctype" 1 "@code{\\1}")
    ("dataline" 1 (progn (setq findex t) "@item \\1\n@findex \\1\n"))
    ("date" 1 "\\1")
    ("declaremodule" 2 (progn (setq cindex t) "@label{\\2}@cindex{\\2}"))
    ("deprecated" 2 "@emph{This is deprecated in Python \\1.  \\2}")
    ("dfn" 1 "@dfn{\\1}")
    ("documentclass" 1 py2texi-magic)
    ("e" 0 "@backslash{}")
    ("else" 0 (concat "@end ifinfo\n@" (setq last-if "iftex")))
    ("EOF" 0 "@code{EOF}")
    ("email" 1 "@email{\\1}")
    ("emph" 1 "@emph{\\1}")
    ("envvar" 1 "@samp{\\1}")
    ("exception" 1 "@code{\\1}")
    ("exindex" 1 (progn (setq obindex t) "@obindex{\\1}"))
    ("fi" 0 (concat "@end " last-if))
    ("file" 1 "@file{\\1}")
    ("filevar" 1 "@file{@var{\\1}}")
    ("footnote" 1 "@footnote{\\1}")
    ("frac" 0 "")
    ("funcline" 2 (progn (setq findex t) "@item \\1 \\2\n@findex \\1"))
    ("funclineni" 2 "@item \\1 \\2")
    ("function" 1 "@code{\\1}")
    ("grammartoken" 1 "@code{\\1}")
    ("hline" 0 "")
    ("ifhtml" 0 (concat "@" (setq last-if "ifinfo")))
    ("iftexi" 0 (concat "@" (setq last-if "ifinfo")))
    ("index" 1 (progn (setq cindex t) "@cindex{\\1}"))
    ("indexii" 2 (progn (setq cindex t) "@cindex{\\1 \\2}"))
    ("indexiii" 3 (progn (setq cindex t) "@cindex{\\1 \\2 \\3}"))
    ("indexiv" 3 (progn (setq cindex t) "@cindex{\\1 \\2 \\3 \\4}"))
    ("infinity" 0 "@emph{infinity}")
    ("it" 0 "@destroy")
    ("kbd" 1 "@key{\\1}")
    ("keyword" 1 "@code{\\1}")
    ("kwindex" 1 (progn (setq cindex t) "@cindex{\\1}"))
    ("label" 1 "@label{\\1}")
    ("Large" 0 "")
    ("LaTeX" 0 "La@TeX{}")
    ("large" 0 "")
    ("ldots" 0 "@dots{}")
    ("leftline" 1 "\\1")
    ("lineii" 2 "@item \\1 @tab \\2")
    ("lineiii" 3 "@item \\1 @tab \\2 @tab \\3")
    ("lineiv" 4 "@item \\1 @tab \\2 @tab \\3 @tab \\4")
    ("linev" 5 "@item \\1 @tab \\2 @tab \\3 @tab \\4 @tab \\5")
    ("localmoduletable" 0 "")
    ("longprogramopt" 1 "@option{--\\1}")
    ("mailheader" 1 "@code{\\1}")
    ("makeindex" 0 "")
    ("makemodindex" 0 "")
    ("maketitle" 0 (concat "@top " title "\n"))
    ("makevar" 1 "@code{\\1}")
    ("manpage" 2 "@samp{\\1(\\2)}")
    ("mbox" 1 "@w{\\1}")
    ("member" 1 "@code{\\1}")
    ("memberline" 1 "@item \\1\n@findex \\1\n")
    ("menuselection" 1 "@samp{\\1}")
    ("method" 1 "@code{\\1}")
    ("methodline" 2 (progn (setq moindex t) "@item \\1(\\2)\n@moindex \\1\n"))
    ("methodlineni" 2 "@item \\1(\\2)\n")
    ("mimetype" 1 "@samp{\\1}")
    ("module" 1 "@samp{\\1}")
    ("moduleauthor" 2 "This module was written by \\1 @email{\\2}.@*")
    ("modulesynopsis" 1 "\\1")
    ("moreargs" 0 "@dots{}")
    ("n" 0 "@backslash{}n")
    ("newcommand" 2 "")
    ("newsgroup" 1 "@samp{\\1}")
    ("nodename" 1
     (save-excursion
       (save-match-data
	 (re-search-backward "^@node "))
       (delete-region (point) (save-excursion (end-of-line) (point)))
       (insert "@node " (match-string 1 string))
       ""))
    ("noindent" 0 "@noindent ")
    ("note" 1 "@emph{Note:} \\1")
    ("NULL" 0 "@code{NULL}")
    ("obindex" 1 (progn (setq obindex t) "@obindex{\\1}"))
    ("opindex" 1 (progn (setq cindex t) "@cindex{\\1}"))
    ("option" 1 "@option{\\1}")
    ("optional" 1 "[\\1]")
    ("pep" 1 (progn (setq cindex t) "PEP@ \\1@cindex PEP \\1\n"))
    ("pi" 0 "pi")
    ("platform" 1 "")
    ("plusminus" 0 "+-")
    ("POSIX" 0 "POSIX")
    ("production" 2 "@item \\1 \\2")
    ("productioncont" 1 "@item \\1")
    ("program" 1 "@command{\\1}")
    ("programopt" 1 "@option{\\1}")
    ("protect" 0 "")
    ("pytype" 1 "@code{\\1}")
    ("ref" 1 "@ref{\\1}")
    ("refbimodindex" 1 (progn (setq moindex t) "@moindex{\\1}"))
    ("refmodindex" 1 (progn (setq moindex t) "@moindex{\\1}"))
    ("refmodule" 1 "@samp{\\1}")
    ("refstmodindex" 1 (progn (setq moindex t) "@moindex{\\1}"))
    ("regexp" 1 "\"\\1\"")
    ("release" 1
     (progn (setq py2texi-python-version (match-string 1 string)) ""))
    ("renewcommand" 2 "")
    ("rfc" 1 (progn (setq cindex t) "RFC@ \\1@cindex RFC \\1\n"))
    ("rm" 0 "@destroy")
    ("samp" 1 "@samp{\\1}")
    ("section" 1 (let ((str (match-string 1 string)))
		   (save-match-data
		     (if (string-match "\\(.*\\)[ \t\n]*---[ \t\n]*\\(.*\\)"
				       str)
			 (format
			  "@node %s\n@section %s\n"
			  (py2texi-backslash-quote (match-string 1 str))
			  (py2texi-backslash-quote (match-string 2 str)))
		       "@node \\1\n@section \\1\n"))))
    ("sectionauthor" 2
     "\nThis manual section was written by \\1 @email{\\2}.@*")
    ("seemodule" 2 "@ref{\\1} \\2")
    ("seepep" 3 "\n@table @strong\n@item PEP\\1 \\2\n\\3\n@end table\n")
    ("seerfc" 3 "\n@table @strong\n@item RFC\\1 \\2\n\\3\n@end table\n")
    ("seetext" 1 "\\1")
    ("seetitle" 1 "@cite{\\1}")
    ("seeurl" 2 "\n@table @url\n@item \\1\n\\2\n@end table\n")
    ("setindexsubitem" 1 (progn (setq cindex t) "@cindex \\1"))
    ("setreleaseinfo" 1 (progn (setq py2texi-releaseinfo "")))
    ("setshortversion" 1
     (progn (setq py2texi-python-short-version (match-string 1 string)) ""))
    ("shortversion" 0 py2texi-python-short-version)
    ("sqrt" 0 "")
    ("stindex" 1 (progn (setq cindex t) "@cindex{\\1}"))
    ("stmodindex" 1 (progn (setq moindex t) "@moindex{\\1}"))
    ("strong" 1 "@strong{\\1}")
    ("sub" 0 "/")
    ("subsection" 1 "@node \\1\n@subsection \\1\n")
    ("subsubsection" 1 "@node \\1\n@subsubsection \\1\n")
    ("sum" 0 "")
    ("tableofcontents" 0 "")
    ("term" 1 "@item \\1")
    ("textasciitilde" 0 "~")
    ("textasciicircum" 0 "^")
    ("textbackslash" 0 "@backslash{}")
    ("textgreater" 0 ">")
    ("textless" 0 "<")
    ("textrm" 1 "\\1")
    ("texttt" 1 "@code{\\1}")
    ("textunderscore" 0 "_")
    ("title" 1 (progn (setq title (match-string 1 string)) "@settitle \\1"))
    ("today" 0 "@today{}")
    ("token" 1 "@code{\\1}")
    ("tt" 0 "@destroy")
    ("ttindex" 1 (progn (setq cindex t) "@cindex{\\1}"))
    ("u" 0 "@backslash{}u")
    ("ulink" 2 "\\1")
    ("UNIX" 0 "UNIX")
    ("unspecified" 0 "@dots{}")
    ("url" 1 "@url{\\1}")
    ("usepackage" 1 "")
    ("var" 1 "@var{\\1}")
    ("verbatiminput" 1 "@code{\\1}")
    ("version" 0 py2texi-python-version)
    ("versionadded" 1 "@emph{Added in Python version \\1}")
    ("versionchanged" 1 "@emph{Changed in Python version \\1}")
    ("vskip" 1 "")
    ("vspace" 1 "")
    ("warning" 1 "@emph{\\1}")
    ("withsubitem" 2 "\\2")
    ("XXX" 1 "@strong{\\1}"))
  "Associative list of command substitutions.
Each list item is of the form (COMMAND ARGNUM SUBSTITUTION) where:
- COMMAND is LaTeX command name
- ARGNUM is number of (required) command arguments
- SUBSTITUTION substitution for the command.  It is evaled and you can
  reference command arguments through the \\N regexp notation in strings.")

(defvar py2texi-magic "@documentclass\n"
  "\"Magic\" string for auxiliary insertion at the beginning of document.")

(defvar py2texi-dirs '("./" "../texinputs/")
  "Where to search LaTeX input files.")

(defvar py2texi-buffer "*py2texi*"
  "The name of a buffer where Texinfo is generated.")

(defconst py2texi-xemacs (string-match "^XEmacs" (emacs-version))
  "Running under XEmacs?")


(defmacro py2texi-search (regexp &rest body)
  `(progn
     (goto-char (point-min))
     (while (re-search-forward ,regexp nil t)
       ,@body)))

(defmacro py2texi-search-safe (regexp &rest body)
  `(py2texi-search ,regexp
    (unless (py2texi-protected)
      ,@body)))


(defun py2texi-message (message)
  "Report message and stop if `py2texi-stop-on-problems' is non-nil."
  (if py2texi-stop-on-problems
      (error message)
    (message message)))


(defun py2texi-backslash-quote (string)
  "Double backslahes in STRING."
  (let ((i 0))
    (save-match-data
      (while (setq i (string-match "\\\\" string i))
	(setq string (replace-match "\\\\\\\\" t nil string))
	(setq i (+ i 2))))
    string))


(defun py2texi (file)
  "Convert Python LaTeX documentation FILE to Texinfo."
  (interactive "fFile to convert: ")
  (switch-to-buffer (get-buffer-create py2texi-buffer))
  (erase-buffer)
  (insert-file file)
  (let ((case-fold-search nil)
	(title "")
	(author "")
	(author-address "")
	(appendix nil)
	(findex nil)
	(obindex nil)
	(cindex nil)
	(moindex nil)
	last-if)
    (py2texi-process-verbatims)
    (py2texi-process-comments)
    (py2texi-process-includes)
    (py2texi-process-funnyas)
    (py2texi-process-environments)
    (py2texi-process-commands)
    (py2texi-fix-indentation)
    (py2texi-fix-nodes)
    (py2texi-fix-references)
    (py2texi-fix-indices)
    (py2texi-process-simple-commands)
    (py2texi-fix-fonts)
    (py2texi-fix-braces)
    (py2texi-fix-backslashes)
    (py2texi-destroy-empties)
    (py2texi-fix-newlines)
    (py2texi-adjust-level))
  (let* ((filename (concat "./"
			   (file-name-nondirectory file)
			   (if (string-match "\\.tex$" file) "i" ".texi")))
	 (infofilename (py2texi-info-file-name filename)))
    (goto-char (point-min))
    (when (looking-at py2texi-magic)
      (delete-region (point) (progn (beginning-of-line 2) (point)))
      (insert "\\input texinfo @c -*-texinfo-*-\n")
      (insert "@setfilename " (file-name-nondirectory infofilename)))
    (when (re-search-forward "@chapter" nil t)
      (texinfo-all-menus-update t))
    (goto-char (point-min))
    (write-file filename)
    (message (format "You can apply `makeinfo %s' now." filename))))


(defun py2texi-info-file-name (filename)
  "Generate name of info file from original file name FILENAME."
  (setq filename (expand-file-name filename))
  (let ((directory (file-name-directory filename))
	(basename (file-name-nondirectory filename)))
    (concat directory "python-"
	    (substring basename 0 (- (length basename) 4)) "info")))


(defun py2texi-process-verbatims ()
  "Process and protect verbatim environments."
  (let (delimiter
	beg
	end)
    (py2texi-search-safe "\\\\begin{\\(verbatim\\|displaymath\\)}"
      (replace-match "@example")
      (setq beg (copy-marker (point) nil))
      (re-search-forward "\\\\end{\\(verbatim\\|displaymath\\)}")
      (setq end (copy-marker (match-beginning 0) nil))
      (replace-match "@end example")
      (py2texi-texinfo-escape beg end)
      (put-text-property (- beg (length "@example"))
			 (+ end (length "@end example"))
			 'py2texi-protected t))
    (py2texi-search-safe "\\\\verb\\([^a-z]\\)"
      (setq delimiter (match-string 1))
      (replace-match "@code{")
      (setq beg (copy-marker (point) nil))
      (re-search-forward (regexp-quote delimiter))
      (setq end (copy-marker (match-beginning 0) nil))
      (replace-match "}")
      (put-text-property (- beg (length "@code{")) (+ end (length "}"))
			 'py2texi-protected t)
      (py2texi-texinfo-escape beg end))))


(defun py2texi-process-comments ()
  "Remove comments."
  (let (point)
    (py2texi-search-safe "%"
      (setq point (point))
      (when (save-excursion
	      (re-search-backward "\\(^\\|[^\\]\\(\\\\\\\\\\)*\\)%\\=" nil t))
	(delete-region (1- point)
		       (save-excursion (beginning-of-line 2) (point)))))))


(defun py2texi-process-includes ()
  "Include LaTeX input files.
Do not include .ind files."
  (let ((path (file-name-directory file))
	filename
	dirs
	includefile)
    (py2texi-search-safe "\\\\input{\\([^}]+\\)}"
      (setq filename (match-string 1))
      (unless (save-match-data (string-match "\\.tex$" filename))
	(setq filename (concat filename ".tex")))
      (setq includefile (save-match-data
			  (string-match "\\.ind\\.tex$" filename)))
      (setq dirs py2texi-dirs)
      (while (and (not includefile) dirs)
	(setq includefile (concat path (car dirs) filename))
	(unless (file-exists-p includefile)
	  (setq includefile nil)
	  (setq dirs (cdr dirs))))
      (if includefile
	  (save-restriction
	    (narrow-to-region (match-beginning 0) (match-end 0))
	    (delete-region (point-min) (point-max))
	    (when (stringp includefile)
	      (insert-file-contents includefile)
	      (goto-char (point-min))
	      (insert "\n")
	      (py2texi-process-verbatims)
	      (py2texi-process-comments)
	      (py2texi-process-includes)))
	(replace-match (format "\\\\emph{Included file %s}" filename))
	(py2texi-message (format "Input file %s not found" filename))))))


(defun py2texi-process-funnyas ()
  "Convert @s."
  (py2texi-search-safe "@"
    (replace-match "@@")))


(defun py2texi-process-environments ()
  "Process LaTeX environments."
  (let ((stack ())
	kind
	environment
	parameter
	arguments
	n
	string
	description)
    (py2texi-search-safe (concat "\\\\\\(begin\\|end\\|item\\)"
				 "\\({\\([^}]*\\)}\\|[[]\\([^]]*\\)[]]\\|\\)")
      (setq kind (match-string 1)
	    environment (match-string 3)
	    parameter (match-string 4))
      (replace-match "")
      (cond
       ((string= kind "begin")
	(setq description (assoc environment py2texi-environments))
	(if description
	    (progn
	      (setq n (cadr description))
	      (setq description (cddr description))
	      (setq string (py2texi-tex-arguments n))
	      (string-match (py2texi-regexp n) string)
				      ; incorrect but sufficient
	      (insert (replace-match (eval (car description))
				     t nil string))
	      (setq stack (cons (cadr description) stack)))
	  (py2texi-message (format "Unknown environment: %s" environment))
	  (setq stack (cons "" stack))))
       ((string= kind "end")
	(insert (eval (car stack)))
	(setq stack (cdr stack)))
       ((string= kind "item")
	(insert "\n@item " (or parameter "") "\n"))))
    (when stack
      (py2texi-message (format "Unclosed environment: %s" (car stack))))))


(defun py2texi-process-commands ()
  "Process LaTeX commands."
  (let (done
	command
	command-info
	string
	n)
    (while (not done)
      (setq done t)
      (py2texi-search-safe "\\\\\\([a-zA-Z*]+\\)\\(\\[[^]]*\\]\\)?"
	(setq command (match-string 1))
	(setq command-info (assoc command py2texi-commands))
	(if command-info
	    (progn
	      (setq done nil)
	      (replace-match "")
	      (setq command-info (cdr command-info))
	      (setq n (car command-info))
	      (setq string (py2texi-tex-arguments n))
	      (string-match (py2texi-regexp n) string)
				      ; incorrect but sufficient
	      (insert (replace-match (eval (cadr command-info))
				     t nil string)))
	  (py2texi-message (format "Unknown command: %s (not processed)"
				   command)))))))


(defun py2texi-argument-pattern (count)
  (let ((filler "\\(?:[^{}]\\|\\\\{\\|\\\\}\\)*"))
    (if (<= count 0)
	filler
      (concat filler "\\(?:{"
	      (py2texi-argument-pattern (1- count))
	      "}" filler "\\)*" filler))))
(defconst py2texi-tex-argument
  (concat
   "{\\("
   (py2texi-argument-pattern 10)	;really at least 10!
   "\\)}[ \t%@c\n]*")
  "Regexp describing LaTeX command argument including argument separators.")


(defun py2texi-regexp (n)
  "Make regexp matching N LaTeX command arguments."
  (if (= n 0)
      ""
    (let ((regexp "^[^{]*"))
      (while (> n 0)
	(setq regexp (concat regexp py2texi-tex-argument))
	(setq n (1- n)))
      regexp)))


(defun py2texi-tex-arguments (n)
  "Remove N LaTeX command arguments and return them as a string."
  (let ((point (point))
	(i 0)
	result
	match)
    (if (= n 0)
	(progn
	  (when (re-search-forward "\\=\\({}\\| *\\)" nil t)
	    (replace-match ""))
	  "")
      (while (> n 0)
	(unless (re-search-forward
		 "\\(\\=\\|[^\\\\]\\)\\(\\\\\\\\\\)*\\([{}]\\)" nil t)
	  (debug))
	(if (string= (match-string 3) "{")
	    (setq i (1+ i))
	  (setq i (1- i))
	  (when (<= i 0)
	    (setq n (1- n)))))
      (setq result (buffer-substring-no-properties point (point)))
      (while (string-match "\n[ \t]*" result)
	(setq result (replace-match " " t nil result)))
      (delete-region point (point))
      result)))


(defun py2texi-process-simple-commands ()
  "Replace single character LaTeX commands."
  (let (char)
    (py2texi-search-safe "\\\\\\([^a-z]\\)"
      (setq char (match-string 1))
      (replace-match (format "%s%s"
			     (if (or (string= char "{")
				     (string= char "}")
				     (string= char " "))
				 "@"
			       "")
			     (if (string= char "\\")
				 "\\\\"
			       char))))))


(defun py2texi-fix-indentation ()
  "Remove white space at the beginning of lines."
  (py2texi-search-safe "^[ \t]+"
    (replace-match "")))


(defun py2texi-fix-nodes ()
  "Remove unwanted characters from nodes and make nodes unique."
  (let ((nodes (make-hash-table :test 'equal))
	id
	counter
	string
	label)
    (py2texi-search "^@node +\\(.*\\)$"
      (setq string (match-string 1))
      (if py2texi-xemacs
	  (replace-match "@node " t)
	(replace-match "" t nil nil 1))
      (when (string-match "@label{[^}]*}" string)
	(setq label (match-string 0 string))
	(setq string (replace-match "" t nil string)))
      (while (string-match "@[a-zA-Z]+\\|[{}():]\\|``\\|''" string)
	(setq string (replace-match "" t nil string)))
      (while (string-match " -- " string)
	(setq string (replace-match " - " t nil string)))
      (when (string-match " +$" string)
	(setq string (replace-match "" t nil string)))
      (when (string-match "^\\(Built-in\\|Standard\\) Module \\|The " string)
	(setq string (replace-match "" t nil string)))
      (string-match "^[^,]+" string)
      (setq id (match-string 0 string))
      (setq counter (gethash id nodes))
      (if counter
	  (progn
	    (setq counter (1+ counter))
	    (setq string (replace-match (format "\\& %d" counter)
					t nil string)))
	(setq counter 1))
      (setf (gethash id nodes) counter)
      (insert string)
      (when label
	(beginning-of-line 3)
	(insert label "\n")))))


(defun py2texi-fix-references ()
  "Process labels and make references to point to appropriate nodes."
  (let ((labels ())
	node)
    (py2texi-search-safe "@label{\\([^}]*\\)}"
      (setq node (save-excursion
		   (save-match-data
		     (and (re-search-backward "@node +\\([^,\n]+\\)" nil t)
			  (match-string 1)))))
      (when node
	(setq labels (cons (cons (match-string 1) node) labels)))
      (replace-match ""))
    (py2texi-search-safe "@ref{\\([^}]*\\)}"
      (setq node (assoc (match-string 1) labels))
      (replace-match "")
      (when node
	(insert (format "@ref{%s}" (cdr node)))))))


(defun py2texi-fix-indices ()
  "Remove unwanted characters from @*index commands and create final indices."
  (py2texi-search-safe "@..?index\\>[^\n]*\\(\\)\n"
    (replace-match "" t nil nil 1))
  (py2texi-search-safe "@..?index\\>[^\n]*\\(\\)"
    (replace-match "\n" t nil nil 1))
  (py2texi-search-safe "@..?index\\({\\)\\([^}]+\\)\\(}+\\)"
    (replace-match " " t nil nil 1)
    (replace-match "" t nil nil 3)
    (let ((string (match-string 2)))
      (save-match-data
	(while (string-match "@[a-z]+{" string)
	  (setq string (replace-match "" nil nil string)))
	(while (string-match "{" string)
	  (setq string (replace-match "" nil nil string))))
      (replace-match string t t nil 2)))
  (py2texi-search-safe "@..?index\\>.*\\([{}]\\|@[a-z]*\\)"
    (replace-match "" t nil nil 1)
    (goto-char (match-beginning 0)))
  (py2texi-search-safe "[^\n]\\(\\)@..?index\\>"
    (replace-match "\n" t nil nil 1))
  (goto-char (point-max))
  (re-search-backward "@indices")
  (replace-match "")
  (insert (if moindex
	      (concat "@node Module Index\n"
		      "@unnumbered Module Index\n"
		      "@printindex mo\n")
	    "")
	  (if obindex
	      (concat "@node Class-Exception-Object Index\n"
		      "@unnumbered Class, Exception, and Object Index\n"
		      "@printindex ob\n")
	    "")
	  (if findex
	      (concat "@node Function-Method-Variable Index\n"
		      "@unnumbered Function, Method, and Variable Index\n"
		      "@printindex fn\n")
	    "")
	  (if cindex
	      (concat "@node Miscellaneous Index\n"
		      "@unnumbered Miscellaneous Index\n"
		      "@printindex cp\n")
	    "")))


(defun py2texi-fix-backslashes ()
  "Make backslashes from auxiliary commands."
  (py2texi-search-safe "@backslash{}"
    (replace-match "\\\\")))


(defun py2texi-fix-fonts ()
  "Remove garbage after unstructured font commands."
  (let (string)
    (py2texi-search-safe "@destroy"
      (replace-match "")
      (when (eq (preceding-char) ?{)
	(forward-char -1)
	(setq string (py2texi-tex-arguments 1))
	(insert (substring string 1 (1- (length string))))))))


(defun py2texi-fix-braces ()
  "Escape braces for Texinfo."
  (let (string)
    (py2texi-search "{"
       (unless (or (py2texi-protected)
		   (save-excursion
		     (re-search-backward
		      "@\\([a-zA-Z]*\\|multitable.*\\){\\=" nil t)))
	 (forward-char -1)
	 (setq string (py2texi-tex-arguments 1))
	 (insert "@" (substring string 0 (1- (length string))) "@}")))))


(defun py2texi-fix-newlines ()
  "Remove extra newlines."
  (py2texi-search "\n\n\n+"
    (replace-match "\n\n"))
  (py2texi-search-safe "@item.*\n\n"
    (delete-backward-char 1))
  (py2texi-search "@end example"
    (unless (looking-at "\n\n")
      (insert "\n"))))


(defun py2texi-destroy-empties ()
  "Remove all comments.
This avoids some makeinfo errors."
  (py2texi-search "@c\\>"
    (unless (eq (py2texi-protected) t)
      (delete-region (- (point) 2) (save-excursion (end-of-line) (point)))
      (cond
       ((looking-at "\n\n")
	(delete-char 1))
       ((save-excursion (re-search-backward "^[ \t]*\\=" nil t))
	(delete-region (save-excursion (beginning-of-line) (point))
		       (1+ (point))))))))


(defun py2texi-adjust-level ()
  "Increase heading level to @chapter, if needed.
This is only needed for distutils, so it has a very simple form only."
  (goto-char (point-min))
  (unless (re-search-forward "@chapter\\>" nil t)
    (py2texi-search-safe "@section\\>"
      (replace-match "@chapter" t))
    (py2texi-search-safe "@\\(sub\\)\\(sub\\)?section\\>"
      (replace-match "" nil nil nil 1))))


(defun py2texi-texinfo-escape (beg end)
  "Escape Texinfo special characters in region."
  (save-excursion
    (goto-char beg)
    (while (re-search-forward "[@{}]" end t)
      (replace-match "@\\&"))))


(defun py2texi-protected ()
  "Return protection status of the point before current point."
  (get-text-property (1- (point)) 'py2texi-protected))


;;; Announce

(provide 'py2texi)


;;; py2texi.el ends here
