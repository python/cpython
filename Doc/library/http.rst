:mod:`http` --- HTTP modules
============================

.. module:: http
   :synopsis: HTTP status codes and messages

**Source code:** :source:`Lib/http/__init__.py`

.. index::
   pair: HTTP; protocol
   single: HTTP; http (standard module)

--------------

:mod:`http` is a package that collects several modules for working with the
HyperText Transfer Protocol:

* :mod:`http.client` is a low-level HTTP protocol client; for high-level URL
  opening use :mod:`urllib.request`
* :mod:`http.server` contains basic HTTP server classes based on :mod:`socketserver`
* :mod:`http.cookies` has utilities for implementing state management with cookies
* :mod:`http.cookiejar` provides persistence of cookies

:mod:`http` is also a module that defines a number of HTTP status codes and
associated messages through the :class:`http.HTTPStatus` enum:

.. class:: HTTPStatus

   .. versionadded:: 3.5

   A subclass of :class:`enum.IntEnum` that defines a set of HTTP status codes,
   reason phrases and long descriptions written in English.

   Usage::

      >>> from http import HTTPStatus
      >>> HTTPStatus.OK
      <HTTPStatus.OK: 200>
      >>> HTTPStatus.OK == 200
      True
      >>> HTTPStatus.OK.value
      200
      >>> HTTPStatus.OK.phrase
      'OK'
      >>> HTTPStatus.OK.description
      'Request fulfilled, document follows'
      >>> list(HTTPStatus)
      [<HTTPStatus.CONTINUE: 100>, <HTTPStatus.SWITCHING_PROTOCOLS: 101>, ...]

.. _http-status-codes:

HTTP status codes
-----------------

Supported,
`IANA-registered <https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml>`_
status codes available in :class:`http.HTTPStatus` are:

======= =================================== ==================================================================
Code    Enum Name                           Details
======= =================================== ==================================================================
``100`` ``CONTINUE``                        HTTP/1.1 :rfc:`RFC 7231, Section 6.2.1 <7231#section-6.2.1>`
``101`` ``SWITCHING_PROTOCOLS``             HTTP/1.1 :rfc:`RFC 7231, Section 6.2.2 <7231#section-6.2.2>`
``102`` ``PROCESSING``                      WebDAV :rfc:`RFC 2518, Section 10.1 <2518#section-10.1>`
``200`` ``OK``                              HTTP/1.1 :rfc:`RFC 7231, Section 6.3.1 <7231#section-6.3.1>`
``201`` ``CREATED``                         HTTP/1.1 :rfc:`RFC 7231, Section 6.3.2 <7231#section-6.3.2>`
``202`` ``ACCEPTED``                        HTTP/1.1 :rfc:`RFC 7231, Section 6.3.3 <7231#section-6.3.3>`
``203`` ``NON_AUTHORITATIVE_INFORMATION``   HTTP/1.1 :rfc:`RFC 7231, Section 6.3.4 <7231#section-6.3.4>`
``204`` ``NO_CONTENT``                      HTTP/1.1 :rfc:`RFC 7231, Section 6.3.5 <7231#section-6.3.5>`
``205`` ``RESET_CONTENT``                   HTTP/1.1 :rfc:`RFC 7231, Section 6.3.6 <7231#section-6.3.6>`
``206`` ``PARTIAL_CONTENT``                 HTTP/1.1 :rfc:`RFC 7233, Section 4.1 <7233#section-4.1>`
``207`` ``MULTI_STATUS``                    WebDAV :rfc:`RFC 4918, Section 11.1 <4918#section-11.1>`
``208`` ``ALREADY_REPORTED``                WebDAV Binding Extensions :rfc:`RFC 5842, Section 7.1 <5842#section-7.1>` (Experimental)
``226`` ``IM_USED``                         Delta Encoding in HTTP :rfc:`RFC 3229, Section 10.4.1 <3229#section-10.4.1>`
``300`` ``MULTIPLE_CHOICES``                HTTP/1.1 :rfc:`RFC 7231, Section 6.4.1 <7231#section-6.4.1>`
``301`` ``MOVED_PERMANENTLY``               HTTP/1.1 :rfc:`RFC 7231, Section 6.4.2 <7231#section-6.4.2>`
``302`` ``FOUND``                           HTTP/1.1 :rfc:`RFC 7231, Section 6.4.3 <7231#section-6.4.3>`
``303`` ``SEE_OTHER``                       HTTP/1.1 :rfc:`RFC 7231, Section 6.4.4 <7231#section-6.4.4>`
``304`` ``NOT_MODIFIED``                    HTTP/1.1 :rfc:`RFC 7232, Section 4.1 <7232#section-4.1>`
``305`` ``USE_PROXY``                       HTTP/1.1 :rfc:`RFC 7231, Section 6.4.5 <7231#section-6.4.5>`
``307`` ``TEMPORARY_REDIRECT``              HTTP/1.1 :rfc:`RFC 7231, Section 6.4.7 <7231#section-6.4.7>`
``308`` ``PERMANENT_REDIRECT``              Permanent Redirect :rfc:`RFC 7238, Section 3 <7238#section-3>` (Experimental)
``400`` ``BAD_REQUEST``                     HTTP/1.1 :rfc:`RFC 7231, Section 6.5.1 <7231#section-6.5.1>`
``401`` ``UNAUTHORIZED``                    HTTP/1.1 Authentication :rfc:`RFC 7235, Section 3.1 <7235#section-3.1>`
``402`` ``PAYMENT_REQUIRED``                HTTP/1.1 :rfc:`RFC 7231, Section 6.5.2 <7231#section-6.5.2>`
``403`` ``FORBIDDEN``                       HTTP/1.1 :rfc:`RFC 7231, Section 6.5.3 <7231#section-6.5.3>`
``404`` ``NOT_FOUND``                       HTTP/1.1 :rfc:`RFC 7231, Section 6.5.4 <7231#section-6.5.4>`
``405`` ``METHOD_NOT_ALLOWED``              HTTP/1.1 :rfc:`RFC 7231, Section 6.5.5 <7231#section-6.5.5>`
``406`` ``NOT_ACCEPTABLE``                  HTTP/1.1 :rfc:`RFC 7231, Section 6.5.6 <7231#section-6.5.6>`
``407`` ``PROXY_AUTHENTICATION_REQUIRED``   HTTP/1.1 Authentication :rfc:`RFC 7235, Section 3.2 <7235#section-3.2>`
``408`` ``REQUEST_TIMEOUT``                 HTTP/1.1 :rfc:`RFC 7231, Section 6.5.7 <7231#section-6.5.7>`
``409`` ``CONFLICT``                        HTTP/1.1 :rfc:`RFC 7231, Section 6.5.8 <7231#section-6.5.8>`
``410`` ``GONE``                            HTTP/1.1 :rfc:`RFC 7231, Section 6.5.9 <7231#section-6.5.9>`
``411`` ``LENGTH_REQUIRED``                 HTTP/1.1 :rfc:`RFC 7231, Section 6.5.10 <7231#section-6.5.10>`
``412`` ``PRECONDITION_FAILED``             HTTP/1.1 :rfc:`RFC 7232, Section 4.2 <7232#section-4.2>`
``413`` ``REQUEST_ENTITY_TOO_LARGE``        HTTP/1.1 :rfc:`RFC 7231, Section 6.5.11 <7231#section-6.5.11>`
``414`` ``REQUEST_URI_TOO_LONG``            HTTP/1.1 :rfc:`RFC 7231, Section 6.5.12 <7231#section-6.5.12>`
``415`` ``UNSUPPORTED_MEDIA_TYPE``          HTTP/1.1 :rfc:`RFC 7231, Section 6.5.13 <7231#section-6.5.13>`
``416`` ``REQUESTED_RANGE_NOT_SATISFIABLE`` HTTP/1.1 Range Requests :rfc:`RFC 7233, Section 4.4 <7233#section-4.4>`
``417`` ``EXPECTATION_FAILED``              HTTP/1.1 :rfc:`RFC 7231, Section 6.5.14 <7231#section-6.5.14>`
``421`` ``MISDIRECTED_REQUEST``             HTTP/2 :rfc:`RFC 7540, Section 9.1.2 <7540#section-9.1.2>`
``422`` ``UNPROCESSABLE_ENTITY``            WebDAV :rfc:`RFC 4918, Section 11.2 <4918#section-11.2>`
``423`` ``LOCKED``                          WebDAV :rfc:`RFC 4918, Section 11.3 <4918#section-11.3>`
``424`` ``FAILED_DEPENDENCY``               WebDAV :rfc:`RFC 4918, Section 11.4 <4918#section-11.4>`
``426`` ``UPGRADE_REQUIRED``                HTTP/1.1 :rfc:`RFC 7231, Section 6.5.15 <7231#section-6.5.15>`
``428`` ``PRECONDITION_REQUIRED``           Additional HTTP Status Codes :rfc:`RFC 6585, Section 3 <6585#section-3>`
``429`` ``TOO_MANY_REQUESTS``               Additional HTTP Status Codes :rfc:`RFC 6585, Section 4 <6585#section-4>`
``431`` ``REQUEST_HEADER_FIELDS_TOO_LARGE`` Additional HTTP Status Codes :rfc:`RFC 6585, Section 5 <6585#section-5>`
``451`` ``UNAVAILABLE_FOR_LEGAL_REASONS``   An HTTP Status Code to Report Legal Obstacles :rfc:`RFC 7725, Section 3 <7725#section-3>`
``500`` ``INTERNAL_SERVER_ERROR``           HTTP/1.1 :rfc:`RFC 7231, Section 6.6.1 <7231#section-6.6.1>`
``501`` ``NOT_IMPLEMENTED``                 HTTP/1.1 :rfc:`RFC 7231, Section 6.6.2 <7231#section-6.6.2>`
``502`` ``BAD_GATEWAY``                     HTTP/1.1 :rfc:`RFC 7231, Section 6.6.3 <7231#section-6.6.3>`
``503`` ``SERVICE_UNAVAILABLE``             HTTP/1.1 :rfc:`RFC 7231, Section 6.6.4 <7231#section-6.6.4>`
``504`` ``GATEWAY_TIMEOUT``                 HTTP/1.1 :rfc:`RFC 7231, Section 6.6.5 <7231#section-6.6.5>`
``505`` ``HTTP_VERSION_NOT_SUPPORTED``      HTTP/1.1 :rfc:`RFC 7231, Section 6.6.6 <7231#section-6.6.6>`
``506`` ``VARIANT_ALSO_NEGOTIATES``         Transparent Content Negotiation in HTTP :rfc:`RFC 2295, Section 8.1 <2295#section-8.1>` (Experimental)
``507`` ``INSUFFICIENT_STORAGE``            WebDAV :rfc:`RFC 4918, Section 11.5 <4918#section-11.5>`
``508`` ``LOOP_DETECTED``                   WebDAV Binding Extensions :rfc:`RFC 5842, Section 7.2 <5842#section-7.2>` (Experimental)
``510`` ``NOT_EXTENDED``                    An HTTP Extension Framework :rfc:`RFC 2774, Section 7 <2774#section-7>` (Experimental)
``511`` ``NETWORK_AUTHENTICATION_REQUIRED`` Additional HTTP Status Codes :rfc:`RFC 6585, Section 6 <6585#section-6>`
======= =================================== ==================================================================

In order to preserve backwards compatibility, enum values are also present
in the :mod:`http.client` module in the form of constants. The enum name is
equal to the constant name (i.e. ``http.HTTPStatus.OK`` is also available as
``http.client.OK``).

.. versionchanged:: 3.7
   Added ``421 MISDIRECTED_REQUEST`` status code.

.. versionadded:: 3.8
   Added ``451 UNAVAILABLE_FOR_LEGAL_REASONS`` status code.
