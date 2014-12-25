:mod:`http` --- HTTP modules
============================

.. module:: http
   :synopsis: HTTP status codes and messages

.. index::
   pair: HTTP; protocol
   single: HTTP; http (standard module)

**Source code:** :source:`Lib/http/__init__.py`

:mod:`http` is a also package that collects several modules for working with the
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
      >>> http.HTTPStatus.OK.value
      200
      >>> HTTPStatus.OK.phrase
      'OK'
      >>> HTTPStatus.OK.description
      'Request fulfilled, document follows'
      >>> list(HTTPStatus)
      [<HTTPStatus.CONTINUE: 100>, <HTTPStatus.SWITCHING_PROTOCOLS: 101>, ...]

   The supported HTTP status codes are:

   === ==============================
   100 Continue
   101 Switching Protocols
   102 Processing
   200 OK
   201 Created
   202 Accepted
   203 Non-Authoritative Information
   204 No Content
   205 Reset Content
   206 Partial Content
   207 Multi-Status
   208 Already Reported
   226 IM Used
   300 Multiple Choices
   301 Moved Permanently
   302 Found
   303 See Other
   304 Not Modified
   305 Use Proxy
   306 Switch Proxy
   307 Temporary Redirect
   308 Permanent Redirect
   400 Bad Request
   401 Unauthorized
   402 Payment Required
   403 Forbidden
   404 Not Found
   405 Method Not Allowed
   406 Not Acceptable
   407 Proxy Authentication Required
   408 Request Timeout
   409 Conflict
   410 Gone
   411 Length Required
   412 Precondition Failed
   413 Request Entity Too Large
   414 Request URI Too Long
   415 Unsupported Media Type
   416 Request Range Not Satisfiable
   417 Expectation Failed
   418 I'm a teapot
   419 Authentication Timeout
   420 Method Failure *(Spring framework)*
   422 Unprocessable Entity
   423 Locked
   424 Failed Dependency
   426 Upgrade Required
   428 Precondition Required
   429 Too Many Requests
   431 Request Header Field Too Large
   440 Login Timeout *(Microsoft)*
   444 No Response *(Nginx)*
   449 Retry With *(Microsoft)*
   450 Blocked By Windows Parental Controls *(Microsoft)*
   494 Request Header Too Large *(Nginx)*
   495 Cert Error *(Nginx)*
   496 No Cert *(Nginx)*
   497 HTTP To HTTPS *(Nginx)*
   499 Client Closed Request *(Nginx)*
   500 Internal Server Error
   501 Not Implemented
   502 Bad Gateway
   503 Service Unavailable
   504 Gateway Timeout
   505 HTTP Version Not Supported
   506 Variant Also Negotiates
   507 Insufficient Storage
   508 Loop Detected
   509 Bandwidth Limit Exceeded
   510 Not Extended
   511 Network Authentication Required
   520 Origin Error *(CloudFlare)*
   521 Web Server Is Down *(CloudFlare)*
   522 Connection Timed Out *(CloudFlare)*
   523 Proxy Declined Request *(CloudFlare)*
   524 A Timeout Occurred *(CloudFlare)*
   598 Network Read Timeout Error
   599 Network Connect Timeout Error
   === ==============================
