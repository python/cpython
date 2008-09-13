.. _reference:

*****************
Command Reference
*****************

.. % \section{Building modules: the \protect\command{build} command family}
.. % \label{build-cmds}
.. % \subsubsection{\protect\command{build}}
.. % \label{build-cmd}
.. % \subsubsection{\protect\command{build\_py}}
.. % \label{build-py-cmd}
.. % \subsubsection{\protect\command{build\_ext}}
.. % \label{build-ext-cmd}
.. % \subsubsection{\protect\command{build\_clib}}
.. % \label{build-clib-cmd}


.. _install-cmd:

Installing modules: the :command:`install` command family
=========================================================

The install command ensures that the build commands have been run and then runs
the subcommands :command:`install_lib`, :command:`install_data` and
:command:`install_scripts`.

.. % \subsubsection{\protect\command{install\_lib}}
.. % \label{install-lib-cmd}


.. _install-data-cmd:

:command:`install_data`
-----------------------

This command installs all data files provided with the distribution.


.. _install-scripts-cmd:

:command:`install_scripts`
--------------------------

This command installs all (Python) scripts in the distribution.

.. % \subsection{Cleaning up: the \protect\command{clean} command}
.. % \label{clean-cmd}


.. _sdist-cmd:

Creating a source distribution: the :command:`sdist` command
============================================================

**\*\*** fragment moved down from above: needs context! **\*\***

The manifest template commands are:

+-------------------------------------------+-----------------------------------------------+
| Command                                   | Description                                   |
+===========================================+===============================================+
| :command:`include pat1 pat2 ...`          | include all files matching any of the listed  |
|                                           | patterns                                      |
+-------------------------------------------+-----------------------------------------------+
| :command:`exclude pat1 pat2 ...`          | exclude all files matching any of the listed  |
|                                           | patterns                                      |
+-------------------------------------------+-----------------------------------------------+
| :command:`recursive-include dir pat1 pat2 | include all files under *dir* matching any of |
| ...`                                      | the listed patterns                           |
+-------------------------------------------+-----------------------------------------------+
| :command:`recursive-exclude dir pat1 pat2 | exclude all files under *dir* matching any of |
| ...`                                      | the listed patterns                           |
+-------------------------------------------+-----------------------------------------------+
| :command:`global-include pat1 pat2 ...`   | include all files anywhere in the source tree |
|                                           | matching --- & any of the listed patterns     |
+-------------------------------------------+-----------------------------------------------+
| :command:`global-exclude pat1 pat2 ...`   | exclude all files anywhere in the source tree |
|                                           | matching --- & any of the listed patterns     |
+-------------------------------------------+-----------------------------------------------+
| :command:`prune dir`                      | exclude all files under *dir*                 |
+-------------------------------------------+-----------------------------------------------+
| :command:`graft dir`                      | include all files under *dir*                 |
+-------------------------------------------+-----------------------------------------------+

The patterns here are Unix-style "glob" patterns: ``*`` matches any sequence of
regular filename characters, ``?`` matches any single regular filename
character, and ``[range]`` matches any of the characters in *range* (e.g.,
``a-z``, ``a-zA-Z``, ``a-f0-9_.``).  The definition of "regular filename
character" is platform-specific: on Unix it is anything except slash; on Windows
anything except backslash or colon.

**\*\*** Windows support not there yet **\*\***

.. % \section{Creating a built distribution: the
.. % \protect\command{bdist} command family}
.. % \label{bdist-cmds}

.. % \subsection{\protect\command{bdist}}
.. % \subsection{\protect\command{bdist\_dumb}}
.. % \subsection{\protect\command{bdist\_rpm}}
.. % \subsection{\protect\command{bdist\_wininst}}


