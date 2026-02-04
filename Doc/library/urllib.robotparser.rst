:mod:`!urllib.robotparser` ---  Parser for robots.txt
=====================================================

.. module:: urllib.robotparser
   :synopsis: Load a robots.txt file and answer questions about
              fetchability of other URLs.

.. sectionauthor:: Skip Montanaro <skip.montanaro@gmail.com>

**Source code:** :source:`Lib/urllib/robotparser.py`

.. index::
   single: WWW
   single: World Wide Web
   single: URL
   single: robots.txt

--------------

This module provides a single class, :class:`RobotFileParser`, which answers
questions about whether or not a particular user agent can fetch a URL on the
website that published the :file:`robots.txt` file.  For more details on the
structure of :file:`robots.txt` files, see http://www.robotstxt.org/orig.html.


.. class:: RobotFileParser(url='')

   This class provides methods to read, parse and answer questions about the
   :file:`robots.txt` file at *url*.

   .. method:: set_url(url)

      Sets the URL referring to a :file:`robots.txt` file.

   .. method:: read()

      Reads the :file:`robots.txt` URL and feeds it to the parser.

   .. method:: parse(lines)

      Parses the lines argument.

   .. method:: can_fetch(useragent, url)

      Returns ``True`` if the *useragent* is allowed to fetch the *url*
      according to the rules contained in the parsed :file:`robots.txt`
      file.

   .. method:: mtime()

      Returns the time the ``robots.txt`` file was last fetched.  This is
      useful for long-running web spiders that need to check for new
      ``robots.txt`` files periodically.

   .. method:: modified()

      Sets the time the ``robots.txt`` file was last fetched to the current
      time.

   .. method:: crawl_delay(useragent)

      Returns the value of the ``Crawl-delay`` parameter from ``robots.txt``
      for the *useragent* in question.  If there is no such parameter or it
      doesn't apply to the *useragent* specified or the ``robots.txt`` entry
      for this parameter has invalid syntax, return ``None``.

      .. versionadded:: 3.6

   .. method:: request_rate(useragent)

      Returns the contents of the ``Request-rate`` parameter from
      ``robots.txt`` as a :term:`named tuple` ``RequestRate(requests, seconds)``.
      If there is no such parameter or it doesn't apply to the *useragent*
      specified or the ``robots.txt`` entry for this parameter has invalid
      syntax, return ``None``.

      .. versionadded:: 3.6

   .. method:: site_maps()

      Returns the contents of the ``Sitemap`` parameter from
      ``robots.txt`` in the form of a :func:`list`. If there is no such
      parameter or the ``robots.txt`` entry for this parameter has
      invalid syntax, return ``None``.

      .. versionadded:: 3.8


The following example demonstrates basic use of the :class:`RobotFileParser`
class::

   >>> import urllib.robotparser
   >>> rp = urllib.robotparser.RobotFileParser()
   >>> rp.set_url("http://www.pythontest.net/robots.txt")
   >>> rp.read()
   >>> rrate = rp.request_rate("*")
   >>> rrate.requests
   1
   >>> rrate.seconds
   1
   >>> rp.crawl_delay("*")
   6
   >>> rp.can_fetch("*", "http://www.pythontest.net/")
   True
   >>> rp.can_fetch("*", "http://www.pythontest.net/no-robots-here/")
   False
