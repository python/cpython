:mod:`!http` --- HTTP modules
=============================

.. module:: http
   :synopsis: HTTP status codes and messages

**Source code:** :source:`Lib/http/__init__.py`

.. index::
   pair: HTTP; protocol
   single: HTTP; http (standard module)

--------------

:mod:`!http` is a package that collects several modules for working with the
HyperText Transfer Protocol:

* :mod:`http.client` is a low-level HTTP protocol client; for high-level URL
  opening use :mod:`urllib.request`
* :mod:`http.server` contains basic HTTP server classes based on :mod:`socketserver`
* :mod:`http.cookies` has utilities for implementing state management with cookies
* :mod:`http.cookiejar` provides persistence of cookies


The :mod:`!http` module also defines the following enums that help you work with http related code:

.. class:: HTTPStatus

   .. versionadded:: 3.5

   A subclass of :class:`enum.IntEnum` that defines a set of HTTP status codes,
   reason phrases and long descriptions written in English.

   Usage::

      >>> from http import HTTPStatus
      >>> HTTPStatus.OK
      HTTPStatus.OK
      >>> HTTPStatus.OK == 200
      True
      >>> HTTPStatus.OK.value
      200
      >>> HTTPStatus.OK.phrase
      'OK'
      >>> HTTPStatus.OK.description
      'Request fulfilled, document follows'
      >>> list(HTTPStatus)
      [HTTPStatus.CONTINUE, HTTPStatus.SWITCHING_PROTOCOLS, ...]

.. _http-status-codes:

HTTP status codes
-----------------

Supported,
`IANA-registered status codes <https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml>`_
available in :class:`http.HTTPStatus` are:

======= =================================== ==================================================================
Code    Enum Name                           Details
======= =================================== ==================================================================
``100`` ``CONTINUE``                        HTTP Semantics :rfc:`9110`, Section 15.2.1
``101`` ``SWITCHING_PROTOCOLS``             HTTP Semantics :rfc:`9110`, Section 15.2.2
``102`` ``PROCESSING``                      WebDAV :rfc:`2518`, Section 10.1
``103`` ``EARLY_HINTS``                     An HTTP Status Code for Indicating Hints :rfc:`8297`
``200`` ``OK``                              HTTP Semantics :rfc:`9110`, Section 15.3.1
``201`` ``CREATED``                         HTTP Semantics :rfc:`9110`, Section 15.3.2
``202`` ``ACCEPTED``                        HTTP Semantics :rfc:`9110`, Section 15.3.3
``203`` ``NON_AUTHORITATIVE_INFORMATION``   HTTP Semantics :rfc:`9110`, Section 15.3.4
``204`` ``NO_CONTENT``                      HTTP Semantics :rfc:`9110`, Section 15.3.5
``205`` ``RESET_CONTENT``                   HTTP Semantics :rfc:`9110`, Section 15.3.6
``206`` ``PARTIAL_CONTENT``                 HTTP Semantics :rfc:`9110`, Section 15.3.7
``207`` ``MULTI_STATUS``                    WebDAV :rfc:`4918`, Section 11.1
``208`` ``ALREADY_REPORTED``                WebDAV Binding Extensions :rfc:`5842`, Section 7.1 (Experimental)
``226`` ``IM_USED``                         Delta Encoding in HTTP :rfc:`3229`, Section 10.4.1
``300`` ``MULTIPLE_CHOICES``                HTTP Semantics :rfc:`9110`, Section 15.4.1
``301`` ``MOVED_PERMANENTLY``               HTTP Semantics :rfc:`9110`, Section 15.4.2
``302`` ``FOUND``                           HTTP Semantics :rfc:`9110`, Section 15.4.3
``303`` ``SEE_OTHER``                       HTTP Semantics :rfc:`9110`, Section 15.4.4
``304`` ``NOT_MODIFIED``                    HTTP Semantics :rfc:`9110`, Section 15.4.5
``305`` ``USE_PROXY``                       HTTP Semantics :rfc:`9110`, Section 15.4.6
``307`` ``TEMPORARY_REDIRECT``              HTTP Semantics :rfc:`9110`, Section 15.4.8
``308`` ``PERMANENT_REDIRECT``              HTTP Semantics :rfc:`9110`, Section 15.4.9
``400`` ``BAD_REQUEST``                     HTTP Semantics :rfc:`9110`, Section 15.5.1
``401`` ``UNAUTHORIZED``                    HTTP Semantics :rfc:`9110`, Section 15.5.2
``402`` ``PAYMENT_REQUIRED``                HTTP Semantics :rfc:`9110`, Section 15.5.3
``403`` ``FORBIDDEN``                       HTTP Semantics :rfc:`9110`, Section 15.5.4
``404`` ``NOT_FOUND``                       HTTP Semantics :rfc:`9110`, Section 15.5.5
``405`` ``METHOD_NOT_ALLOWED``              HTTP Semantics :rfc:`9110`, Section 15.5.6
``406`` ``NOT_ACCEPTABLE``                  HTTP Semantics :rfc:`9110`, Section 15.5.7
``407`` ``PROXY_AUTHENTICATION_REQUIRED``   HTTP Semantics :rfc:`9110`, Section 15.5.8
``408`` ``REQUEST_TIMEOUT``                 HTTP Semantics :rfc:`9110`, Section 15.5.9
``409`` ``CONFLICT``                        HTTP Semantics :rfc:`9110`, Section 15.5.10
``410`` ``GONE``                            HTTP Semantics :rfc:`9110`, Section 15.5.11
``411`` ``LENGTH_REQUIRED``                 HTTP Semantics :rfc:`9110`, Section 15.5.12
``412`` ``PRECONDITION_FAILED``             HTTP Semantics :rfc:`9110`, Section 15.5.13
``413`` ``CONTENT_TOO_LARGE``               HTTP Semantics :rfc:`9110`, Section 15.5.14
``414`` ``URI_TOO_LONG``                    HTTP Semantics :rfc:`9110`, Section 15.5.15
``415`` ``UNSUPPORTED_MEDIA_TYPE``          HTTP Semantics :rfc:`9110`, Section 15.5.16
``416`` ``RANGE_NOT_SATISFIABLE``           HTTP Semantics :rfc:`9110`, Section 15.5.17
``417`` ``EXPECTATION_FAILED``              HTTP Semantics :rfc:`9110`, Section 15.5.18
``418`` ``IM_A_TEAPOT``                     HTCPCP/1.0 :rfc:`2324`, Section 2.3.2
``421`` ``MISDIRECTED_REQUEST``             HTTP Semantics :rfc:`9110`, Section 15.5.20
``422`` ``UNPROCESSABLE_CONTENT``           HTTP Semantics :rfc:`9110`, Section 15.5.21
``423`` ``LOCKED``                          WebDAV :rfc:`4918`, Section 11.3
``424`` ``FAILED_DEPENDENCY``               WebDAV :rfc:`4918`, Section 11.4
``425`` ``TOO_EARLY``                       Using Early Data in HTTP :rfc:`8470`
``426`` ``UPGRADE_REQUIRED``                HTTP Semantics :rfc:`9110`, Section 15.5.22
``428`` ``PRECONDITION_REQUIRED``           Additional HTTP Status Codes :rfc:`6585`
``429`` ``TOO_MANY_REQUESTS``               Additional HTTP Status Codes :rfc:`6585`
``431`` ``REQUEST_HEADER_FIELDS_TOO_LARGE`` Additional HTTP Status Codes :rfc:`6585`
``451`` ``UNAVAILABLE_FOR_LEGAL_REASONS``   An HTTP Status Code to Report Legal Obstacles :rfc:`7725`
``500`` ``INTERNAL_SERVER_ERROR``           HTTP Semantics :rfc:`9110`, Section 15.6.1
``501`` ``NOT_IMPLEMENTED``                 HTTP Semantics :rfc:`9110`, Section 15.6.2
``502`` ``BAD_GATEWAY``                     HTTP Semantics :rfc:`9110`, Section 15.6.3
``503`` ``SERVICE_UNAVAILABLE``             HTTP Semantics :rfc:`9110`, Section 15.6.4
``504`` ``GATEWAY_TIMEOUT``                 HTTP Semantics :rfc:`9110`, Section 15.6.5
``505`` ``HTTP_VERSION_NOT_SUPPORTED``      HTTP Semantics :rfc:`9110`, Section 15.6.6
``506`` ``VARIANT_ALSO_NEGOTIATES``         Transparent Content Negotiation in HTTP :rfc:`2295`, Section 8.1 (Experimental)
``507`` ``INSUFFICIENT_STORAGE``            WebDAV :rfc:`4918`, Section 11.5
``508`` ``LOOP_DETECTED``                   WebDAV Binding Extensions :rfc:`5842`, Section 7.2 (Experimental)
``510`` ``NOT_EXTENDED``                    An HTTP Extension Framework :rfc:`2774`, Section 7 (Experimental)
``511`` ``NETWORK_AUTHENTICATION_REQUIRED`` Additional HTTP Status Codes :rfc:`6585`, Section 6
======= =================================== ==================================================================

In order to preserve backwards compatibility, enum values are also present
in the :mod:`http.client` module in the form of constants. The enum name is
equal to the constant name (i.e. ``http.HTTPStatus.OK`` is also available as
``http.client.OK``).

.. versionchanged:: 3.7
   Added ``421 MISDIRECTED_REQUEST`` status code.

.. versionadded:: 3.8
   Added ``451 UNAVAILABLE_FOR_LEGAL_REASONS`` status code.

.. versionadded:: 3.9
   Added ``103 EARLY_HINTS``, ``418 IM_A_TEAPOT`` and ``425 TOO_EARLY`` status codes.

.. versionchanged:: 3.13
   Implemented RFC9110 naming for status constants. Old constant names are preserved for
   backwards compatibility: ``413 REQUEST_ENTITY_TOO_LARGE``, ``414 REQUEST_URI_TOO_LONG``,
   ``416 REQUESTED_RANGE_NOT_SATISFIABLE`` and ``422 UNPROCESSABLE_ENTITY``.

HTTP status category
--------------------

.. versionadded:: 3.12

The enum values have several properties to indicate the HTTP status category:

==================== ======================== ======================================
Property             Indicates that           Details
==================== ======================== ======================================
``is_informational`` ``100 <= status <= 199`` HTTP Semantics :rfc:`9110`, Section 15
``is_success``       ``200 <= status <= 299`` HTTP Semantics :rfc:`9110`, Section 15
``is_redirection``   ``300 <= status <= 399`` HTTP Semantics :rfc:`9110`, Section 15
``is_client_error``  ``400 <= status <= 499`` HTTP Semantics :rfc:`9110`, Section 15
``is_server_error``  ``500 <= status <= 599`` HTTP Semantics :rfc:`9110`, Section 15
==================== ======================== ======================================

   Usage::

      >>> from http import HTTPStatus
      >>> HTTPStatus.OK.is_success
      True
      >>> HTTPStatus.OK.is_client_error
      False

.. class:: HTTPMethod

   .. versionadded:: 3.11

   A subclass of :class:`enum.StrEnum` that defines a set of HTTP methods and descriptions written in English.

   Usage::

      >>> from http import HTTPMethod
      >>>
      >>> HTTPMethod.GET
      <HTTPMethod.GET>
      >>> HTTPMethod.GET == 'GET'
      True
      >>> HTTPMethod.GET.value
      'GET'
      >>> HTTPMethod.GET.description
      'Retrieve the target.'
      >>> list(HTTPMethod)
      [<HTTPMethod.CONNECT>,
       <HTTPMethod.DELETE>,
       <HTTPMethod.GET>,
       <HTTPMethod.HEAD>,
       <HTTPMethod.OPTIONS>,
       <HTTPMethod.PATCH>,
       <HTTPMethod.POST>,
       <HTTPMethod.PUT>,
       <HTTPMethod.TRACE>]

.. _http-methods:

HTTP methods
-----------------

Supported,
`IANA-registered methods <https://www.iana.org/assignments/http-methods/http-methods.xhtml>`_
available in :class:`http.HTTPMethod` are:

=========== =================================== ==================================================================
Method      Enum Name                           Details
=========== =================================== ==================================================================
``GET``     ``GET``                             HTTP Semantics :rfc:`9110`, Section 9.3.1
``HEAD``    ``HEAD``                            HTTP Semantics :rfc:`9110`, Section 9.3.2
``POST``    ``POST``                            HTTP Semantics :rfc:`9110`, Section 9.3.3
``PUT``     ``PUT``                             HTTP Semantics :rfc:`9110`, Section 9.3.4
``DELETE``  ``DELETE``                          HTTP Semantics :rfc:`9110`, Section 9.3.5
``CONNECT`` ``CONNECT``                         HTTP Semantics :rfc:`9110`, Section 9.3.6
``OPTIONS`` ``OPTIONS``                         HTTP Semantics :rfc:`9110`, Section 9.3.7
``TRACE``   ``TRACE``                           HTTP Semantics :rfc:`9110`, Section 9.3.8
``PATCH``   ``PATCH``                           HTTP/1.1 :rfc:`5789`
=========== =================================== ==================================================================
