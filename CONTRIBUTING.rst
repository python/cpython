========================
Python Developer’s Guide
========================

This guide is a comprehensive resource for contributing to `Python`_ – for both new and experienced contributors. It is `maintained`_ by the same community that maintains Python. We welcome your contributions to Python!

.. _Python: https://www.python.org/
.. _maintained: https://devguide.python.org/docquality/#helping-with-the-developers-guide

Contributing
------------
We encourage everyone to contribute to Python and that’s why we have put up this developer’s guide. If you still have questions after reviewing the material in this guide, then the `Core Python Mentorship`_ group is available to help guide new contributors through the process.

.. _Core Python Mentorship: https://www.python.org/dev/core-mentorship/

A number of individuals from the Python community have contributed to a series of excellent guides at `Open Source Guides`_.

.. _Open Source Guides: https://opensource.guide/

Core developers and contributors alike will find the following guides useful:

- `How to Contribute to Open Source`_
- `Building Welcoming Communities`_

.. _How to Contribute to Open Source: https://opensource.guide/how-to-contribute/
.. _Building Welcoming Communities: https://opensource.guide/building-community/

Guide for contributing to Python:

+------------------------------------+-------------------------------+--------------------------+------------------------------------------------+
| New Contributors                   | Documentarians                | Triagers                 | Core Developers                                |
+====================================+===============================+==========================+================================================+
| `Getting Started`_                 | `Helping with Documentation`_ | `Issue Tracking`_        | `How to Become a Core Developer`_              |
+------------------------------------+-------------------------------+--------------------------+------------------------------------------------+
| `Where to Get Help`_               | `Documenting Python`_         | `Triaging an Issue`_     | `Developer Log`_                               |
+------------------------------------+-------------------------------+--------------------------+------------------------------------------------+
| `Lifecycle of a Pull Request`_     | `Style guide`_                | `Helping Triage Issues`_ | `Accepting Pull Requests`_                     |
+------------------------------------+-------------------------------+--------------------------+------------------------------------------------+
| `Running & Writing Tests`_         | `reStructuredText Primer`_    | `Experts Index`_         | `Development Cycle`_                           |
+------------------------------------+-------------------------------+                          +------------------------------------------------+
| `Fixing "easy" Issues and Beyond`_ | `Translating`_                |                          | `Core Developer Motivations and Affiliations`_ |
+------------------------------------+                               |                          +------------------------------------------------+
| `Following Python's Development`_  |                               |                          | `Core Developers Office Hours`_                |
+------------------------------------+                               |                          |                                                |
| `Git Bootcamp and Cheat Sheet`_    |                               |                          |                                                |
+------------------------------------+-------------------------------+--------------------------+------------------------------------------------+

.. _Getting Started: https://devguide.python.org/setup/
.. _Where to Get Help: https://devguide.python.org/help/
.. _Lifecycle of a Pull Request: https://devguide.python.org/pullrequest/
.. _Running & Writing Tests: https://devguide.python.org/runtests/
.. _Fixing "easy" Issues and Beyond: https://devguide.python.org/fixingissues/
.. _Following Python's Development: https://devguide.python.org/communication/
.. _Git Bootcamp and Cheat Sheet: https://devguide.python.org/gitbootcamp/
.. _Helping with Documentation: https://devguide.python.org/docquality/
.. _Documenting Python: https://devguide.python.org/documenting/
.. _Style guide: https://devguide.python.org/documenting/#style-guide
.. _reStructuredText Primer: https://devguide.python.org/documenting/#rst-primer
.. _Translating: https://devguide.python.org/documenting/#translating
.. _Issue Tracking: https://devguide.python.org/tracker/
.. _Triaging an Issue: https://devguide.python.org/triaging/
.. _Helping Triage Issues: https://devguide.python.org/tracker/#helptriage
.. _Experts Index: https://devguide.python.org/experts/
.. _How to Become a Core Developer: https://devguide.python.org/coredev/
.. _Developer Log: https://devguide.python.org/developers/
.. _Accepting Pull Requests: https://devguide.python.org/committing/
.. _Development Cycle: https://devguide.python.org/devcycle/
.. _Core Developer Motivations and Affiliations: https://devguide.python.org/motivations/
.. _Core Developers Office Hours: https://devguide.python.org/help/#office-hour

Advanced tasks and topics for once you are comfortable:

- `Silence Warnings From the Test Suite`_
- Fixing issues found by the `buildbots`_
- `Coverity Scan`_
- Helping out with reviewing `open pull requests`_. See `how to review a Pull Request`_.
- `Fixing "easy" Issues (and Beyond)`_

.. _Silence Warnings From the Test Suite: https://devguide.python.org/silencewarnings/
.. _buildbots: https://devguide.python.org/buildbots/
.. _Coverity Scan: https://devguide.python.org/coverity/
.. _open pull requests: https://github.com/python/cpython/pulls?utf8=%E2%9C%93&q=is%3Apr%20is%3Aopen%20label%3A%22awaiting%20review%22
.. _how to review a Pull Request: https://devguide.python.org/pullrequest/#how-to-review-a-pull-request
.. _Fixing "easy" Issues (and Beyond): https://devguide.python.org/fixingissues/

It is **recommended** that the above documents be read as needed. New contributors will build understanding of the CPython workflow by reading the sections mentioned in this table. You can stop where you feel comfortable and begin contributing immediately without reading and understanding these documents all at once. If you do choose to skip around within the documentation, be aware that it is written assuming preceding documentation has been read so you may find it necessary to backtrack to fill in missing concepts and terminology.

Proposing changes to Python itself
----------------------------------

Improving Python’s code, documentation and tests are ongoing tasks that are never going to be “finished”, as Python operates as part of an ever-evolving system of technology. An even more challenging ongoing task than these necessary maintenance activities is finding ways to make Python, in the form of the standard library and the language definition, an even better tool in a developer’s toolkit.

While these kinds of change are much rarer than those described above, they do happen and that process is also described as part of this guide:

- `Adding to the Stdlib`_
- `Changing the Python Language`_

.. _Adding to the Stdlib: https://devguide.python.org/stdlibchanges/
.. _Changing the Python Language: https://devguide.python.org/langchanges/

Other Interpreter Implementations
---------------------------------

This guide is specifically for contributing to the Python reference interpreter, also known as CPython (while most of the standard library is written in Python, the interpreter core is written in C and integrates most easily with the C and C++ ecosystems).

There are other Python implementations, each with a different focus. Like CPython, they always have more things they would like to do than they have developers to work on them. Some major examples that may be of interest are:

- `PyPy`_: A Python interpreter focused on high speed (JIT-compiled) operation on major platforms
- `Jython`_: A Python interpreter focused on good integration with the Java Virtual Machine (JVM) environment
- `IronPython`_: A Python interpreter focused on good integration with the Common Language Runtime (CLR) provided by .NET and Mono
- `Stackless`_: A Python interpreter focused on providing lightweight microthreads while remaining largely compatible with CPython specific extension modules

.. _PyPy: https://www.pypy.org/
.. _Jython: https://www.jython.org/
.. _IronPython: https://ironpython.net/
.. _Stackless: https://www.stackless.com/

Quick Reference
---------------

Here are the basic steps needed to get `set up`_ and contribute a patch. This is meant as a checklist, once you know the basics. For complete instructions please see the `setup guide`_.

.. _set up: https://devguide.python.org/setup/#setup
.. _setup guide: https://devguide.python.org/setup/#setup

1. Install and set up `Git`_ and other dependencies (see the `Get Setup`_ page for detailed information).

.. _Git: https://devguide.python.org/setup/#vcsetup
.. _Get Setup: https://devguide.python.org/setup/#setup

2. Fork the CPython repository to your GitHub account and get the source code using::

    git clone https://github.com/<your_username>/cpython
    cd cpython

3. Build Python, on UNIX and macOS use::

    ./configure --with-pydebug && make -j

   and on Windows use::

    PCbuild\build.bat -e -d

   See also `more detailed instructions`_, `how to install and build dependencies`_, and the platform-specific pages for `UNIX`_, `macOS`_, and `Windows`_.

.. _more detailed instructions: https://devguide.python.org/setup/#compiling
.. _how to install and build dependencies: https://devguide.python.org/setup/#build-dependencies
.. _UNIX: https://devguide.python.org/setup/#unix-compiling
.. _macOS: https://devguide.python.org/setup/#macos
.. _Windows: https://devguide.python.org/setup/#windows-compiling

4. `Run the tests`_::

    ./python -m test -j3

   On `most`_ macOS systems, replace ``./python`` with ``./python.exe``. On Windows, use ``python.bat``. With Python 2.7, replace test with ``test.regrtest``.

.. _Run the tests: https://devguide.python.org/runtests/
.. _most: https://devguide.python.org/setup/#mac-python-exe

5. Create a new branch where your work for the issue will go, e.g.::

    git checkout -b fix-issue-12345 master

   If an issue does not already exist, please `create it`_. Trivial issues (e.g. typo fixes) do not require any issue to be created.

.. _create it: https://bugs.python.org/

6. Once you fixed the issue, run the tests, run ``make patchcheck``, and if everything is ok, commit.

7. Push the branch on your fork on GitHub and `create a pull request`_. Include the issue number using ``bpo-NNNN`` in the pull request description. For example::

    bpo-12345: Fix some bug in spam module

.. _create a pull request: https://devguide.python.org/pullrequest/

::

    **Note**: First time contributors will need to sign the Contributor Licensing Agreement (CLA) as described in the `Licensing`_ section of this guide.

.. _Licensing: https://devguide.python.org/pullrequest/#cla

Key Resources
-------------

- Coding style guides
    - `PEP 7`_ (Style Guide for C Code)
    - `PEP 8`_ (Style Guide for Python Code)

.. _PEP 7: https://www.python.org/dev/peps/pep-0007/
.. _PEP 8: https://www.python.org/dev/peps/pep-0008/

- `Issue tracker`_
    - `Meta tracker`_ (issue tracker for the issue tracker)
    - `Experts Index`_

.. _Issue tracker: https://bugs.python.org/
.. _Meta tracker: https://psf.upfronthosting.co.za/roundup/meta
.. _Experts Index: https://devguide.python.org/experts/

- `Buildbot status`_

.. _Buildbot status: https://www.python.org/dev/buildbot/

- Source code
    - `Browse online`_
    - `Snapshot of the *master* branch`_
    - `Daily macOS installer`_

.. _Browse online: https://github.com/python/cpython/
.. _Snapshot of the *master* branch: https://github.com/python/cpython/archive/master.zip
.. _Daily macOS installer: https://buildbot.python.org/daily-dmg/

- `PEPs`_ (Python Enhancement Proposals)

.. _PEPs: https://www.python.org/dev/peps/

- `Where to Get Help`_

.. _Where to Get Help: https://devguide.python.org/help/

- `Developer Log`_

.. _Developer Log: https://devguide.python.org/developers/


Additional Resources
--------------------

- Anyone can clone the sources for this guide. `See Helping with the Developer's Guide`_.

.. _See Helping with the Developer's Guide: https://devguide.python.org/docquality/#helping-with-the-developers-guide

- Help with …
    - `Exploring CPython's Internals`_
    - `Changing CPython's Grammar`_
    - `Design of CPython's Compiler`_

.. _Exploring CPython's Internals: https://devguide.python.org/exploring/
.. _Changing CPython's Grammar: https://devguide.python.org/grammar/
.. _Design of CPython's Compiler: https://devguide.python.org/compiler/

- Tool support
    - `gdb Support`_
    - `Dynamic Analysis with Clang`_
    - Various tools with configuration files as found in the `Misc directory`_
    - Information about editors and their configurations can be found in the `wiki`_

.. _gdb Support: https://devguide.python.org/gdb/
.. _Dynamic Analysis with Clang: https://devguide.python.org/clang/
.. _Misc directory: https://github.com/python/cpython/tree/master/Misc
.. _wiki: https://wiki.python.org/moin/PythonEditors

- `python.org maintenance`_
- `Search this guide`_

.. _python.org maintenance: https://pythondotorg.readthedocs.io/
.. _Search this guide: https://devguide.python.org/search/