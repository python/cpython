.. bpo: 30730
.. date: 9992
.. nonce: rJsyTH
.. original section: Library
.. release date: 2017-07-07
.. section: Security

Prevent environment variables injection in subprocess on Windows.  Prevent
passing other environment variables and command arguments.

..

.. bpo: 30694
.. date: 9991
.. nonce: WkMWM_
.. original section: Library
.. section: Security

Upgrade expat copy from 2.2.0 to 2.2.1 to get fixes of multiple security
vulnerabilities including: CVE-2017-9233 (External entity infinite loop
DoS), CVE-2016-9063 (Integer overflow, re-fix), CVE-2016-0718 (Fix
regression bugs from 2.2.0's fix to CVE-2016-0718) and CVE-2012-0876
(Counter hash flooding with SipHash). Note: the CVE-2016-5300 (Use
os-specific entropy sources like getrandom) doesn't impact Python, since
Python already gets entropy from the OS to set the expat secret using
``XML_SetHashSalt()``.

..

.. bpo: 30500
.. date: 9990
.. nonce: 1VG7R-
.. original section: Library
.. section: Security

Fix urllib.parse.splithost() to correctly parse fragments. For example,
``splithost('//127.0.0.1#@evil.com/')`` now correctly returns the
``127.0.0.1`` host, instead of treating ``@evil.com`` as the host in an
authentication (``login@host``).
