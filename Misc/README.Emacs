=============
Emacs support
=============

If you want to edit Python code in Emacs, you should download python-mode.el
and install it somewhere on your load-path.  See the project page to download:

    https://launchpad.net/python-mode

While Emacs comes with a python.el file, it is not recommended.
python-mode.el is maintained by core Python developers and is generally
considered more Python programmer friendly.  For example, python-mode.el
includes a killer feature called `pdbtrack` which allows you to set a pdb
breakpoint in your code, run your program in an Emacs shell buffer, and do gud
style debugging when the breakpoint is hit.

python-mode.el is compatible with both GNU Emacs from the FSF, and XEmacs.

For more information and bug reporting, see the above project page.  For help,
development, or discussions, see the python-mode mailing list:

    http://mail.python.org/mailman/listinfo/python-mode


..
   Local Variables:
   mode: indented-text
   indent-tabs-mode: nil
   sentence-end-double-space: t
   fill-column: 78
   coding: utf-8
   End:
