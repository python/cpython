.. _reporting-bugs:

*****************
Dealing with Bugs
*****************

Python is a mature programming language which has established a reputation for
stability.  In order to maintain this reputation, the developers would like to
know of any deficiencies you find in Python.

It can be sometimes faster to fix bugs yourself and contribute patches to
Python as it streamlines the process and involves less people. Learn how to
:ref:`contribute <contributing-to-python>`.

Documentation bugs
==================

If you find a bug in this documentation or would like to propose an improvement,
please submit a bug report on the :ref:`tracker <using-the-tracker>`.  If you
have a suggestion on how to fix it, include that as well.

If you're short on time, you can also email documentation bug reports to
docs@python.org (behavioral bugs can be sent to python-list@python.org).
'docs@' is a mailing list run by volunteers; your request will be noticed,
though it may take a while to be processed.

.. seealso::

   `Documentation bugs`_
      A list of documentation bugs that have been submitted to the Python issue tracker.

   `Issue Tracking <https://devguide.python.org/tracker/>`_
      Overview of the process involved in reporting an improvement on the tracker.

   `Helping with Documentation <https://devguide.python.org/docquality/#helping-with-documentation>`_
      Comprehensive guide for individuals that are interested in contributing to Python documentation.

   `Documentation Translations <https://devguide.python.org/documenting/#translating>`_
      A list of GitHub pages for documentation translation and their primary contacts.


.. _using-the-tracker:

Using the Python issue tracker
==============================

Issue reports for Python itself should be submitted via the GitHub issues
tracker (https://github.com/python/cpython/issues).
The GitHub issues tracker offers a web form which allows pertinent information
to be entered and submitted to the developers.

The first step in filing a report is to determine whether the problem has
already been reported.  The advantage in doing so, aside from saving the
developers' time, is that you learn what has been done to fix it; it may be that
the problem has already been fixed for the next release, or additional
information is needed (in which case you are welcome to provide it if you can!).
To do this, search the tracker using the search box at the top of the page.

If the problem you're reporting is not already in the list, log in to GitHub.
If you don't already have a GitHub account, create a new account using the
"Sign up" link.
It is not possible to submit a bug report anonymously.

Being now logged in, you can submit an issue.
Click on the "New issue" button in the top bar to report a new issue.

The submission form has two fields, "Title" and "Comment".

For the "Title" field, enter a *very* short description of the problem;
less than ten words is good.

In the "Comment" field, describe the problem in detail, including what you
expected to happen and what did happen.  Be sure to include whether any
extension modules were involved, and what hardware and software platform you
were using (including version information as appropriate).

Each issue report will be reviewed by a developer who will determine what needs to
be done to correct the problem. You will receive an update each time an action is
taken on the issue.


.. seealso::

   `How to Report Bugs Effectively <https://www.chiark.greenend.org.uk/~sgtatham/bugs.html>`_
      Article which goes into some detail about how to create a useful bug report.
      This describes what kind of information is useful and why it is useful.

   `Bug Writing Guidelines <https://bugzilla.mozilla.org/page.cgi?id=bug-writing.html>`_
      Information about writing a good bug report.  Some of this is specific to the
      Mozilla project, but describes general good practices.

.. _contributing-to-python:

Getting started contributing to Python yourself
===============================================

Beyond just reporting bugs that you find, you are also welcome to submit
patches to fix them.  You can find more information on how to get started
patching Python in the `Python Developer's Guide`_.  If you have questions,
the `core-mentorship mailing list`_ is a friendly place to get answers to
any and all questions pertaining to the process of fixing issues in Python.

.. _Documentation bugs: https://github.com/python/cpython/issues?q=is%3Aissue+is%3Aopen+label%3Adocs
.. _Python Developer's Guide: https://devguide.python.org/
.. _core-mentorship mailing list: https://mail.python.org/mailman3/lists/core-mentorship.python.org/
