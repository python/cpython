;;; ccpy-style.el  ---  cc-mode style definition for Python C code
;;
;; Author:  1996 Barry A. Warsaw
;; Created:  6-Dec-1996
;; Version: $Revision$
;; Last Modified: $Date$
;; Keywords: c python languages oop

;;; Commentary
;; This file defines the standard C coding style for Python C files
;; and modules.  It is compatible with cc-mode.el which should be a
;; standard part of your Emacs distribution (or see
;; <http://www.python.org/ftp/emacs/>).

;; To use, make sure this file is on your Emacs load-path, and simply
;; add this to your .emacs file:
;;
;; (add-hook 'c-mode-common-hook '(lambda () (require 'python-style)))

;; This file will self-install on your c-style-alist variable,
;; although you will have to install it on a per-file basis with:
;;
;; M-x c-set-style RET python RET

;;; Code:

(defconst python-cc-style
  '((indent-tabs-mode . t)
    (c-basic-offset   . 8)
    (c-offset-alist   . ((substatement-open . 0)
			 ))
    )
  "Standard Python C coding style.")

(require 'cc-mode)
(if (not (assoc "python" c-style-alist))
    (c-add-style "python" python-cc-style))

(provide 'ccpy-style)
;;; ccpy-style.el ends here
