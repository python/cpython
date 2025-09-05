.. _security-warnings:

.. index:: single: security considerations

Security Considerations
=======================

The following modules have specific security considerations:

* :mod:`base64`: :ref:`base64 security considerations <base64-security>` in
  :rfc:`4648`
* :mod:`hashlib`: :ref:`all constructors take a "usedforsecurity" keyword-only
  argument disabling known insecure and blocked algorithms
  <hashlib-usedforsecurity>`
* :mod:`http.server` is not suitable for production use, only implementing
  basic security checks. See the :ref:`security considerations <http.server-security>`.
* :mod:`logging`: :ref:`Logging configuration uses eval()
  <logging-eval-security>`
* :mod:`multiprocessing`: :ref:`Connection.recv() uses pickle
  <multiprocessing-recv-pickle-security>`
* :mod:`pickle`: :ref:`Restricting globals in pickle <pickle-restrict>`
* :mod:`random` shouldn't be used for security purposes, use :mod:`secrets`
  instead
* :mod:`shelve`: :ref:`shelve is based on pickle and thus unsuitable for
  dealing with untrusted sources <shelve-security>`
* :mod:`ssl`: :ref:`SSL/TLS security considerations <ssl-security>`
* :mod:`subprocess`: :ref:`Subprocess security considerations
  <subprocess-security>`
* :mod:`tempfile`: :ref:`mktemp is deprecated due to vulnerability to race
  conditions <tempfile-mktemp-deprecated>`
* :mod:`xml`: :ref:`XML security <xml-security>`
* :mod:`zipfile`: :ref:`maliciously prepared .zip files can cause disk volume
  exhaustion <zipfile-resources-limitations>`

The :option:`-I` command line option can be used to run Python in isolated
mode. When it cannot be used, the :option:`-P` option or the
:envvar:`PYTHONSAFEPATH` environment variable can be used to not prepend a
potentially unsafe path to :data:`sys.path` such as the current directory, the
script's directory or an empty string.
