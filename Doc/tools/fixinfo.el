(defun fix-python-texinfo ()
  (goto-char (point-min))
  (replace-regexp "\\(@setfilename \\)\\([-a-z]*\\)$"
		  "\\1python-\\2.info")
  (replace-string "@node Front Matter\n@chapter Abstract\n"
		  "@node Abstract\n@section Abstract\n")
  (mark-whole-buffer)
  (texinfo-master-menu 'update-all-nodes)
  (save-buffer)
  )	;; fix-python-texinfo

;; now really do it:
(find-file (car command-line-args-left))
(fix-python-texinfo)
(kill-emacs)
