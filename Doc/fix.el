; load the new texinfo package (2.xx) if not installed by default
(setq load-path
      (cons "/ufs/jh/lib/emacs/texinfo" load-path))
(find-file "@out.texi")
(texinfo-all-menus-update t)
(texinfo-all-menus-update t)
