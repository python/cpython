:mod:`http.cookiejar` --- Cookie handling for HTTP clients
==========================================================

.. module:: http.cookiejar
   :synopsis: Classes for automatic handling of HTTP cookies.

.. moduleauthor:: John J. Lee <jjl@pobox.com>
.. sectionauthor:: John J. Lee <jjl@pobox.com>

**Source code:** :source:`Lib/http/cookiejar.py`

--------------

The :mod:`http.cookiejar` module defines classes for automatic handling of HTTP
cookies.  It is useful for accessing web sites that require small pieces of data
-- :dfn:`cookies` -- to be set on the client machine by an HTTP response from a
web server, and then returned to the server in later HTTP requests.

Both the regular Netscape cookie protocol and the protocol defined by
:rfc:`2965` are handled.  RFC 2965 handling is switched off by default.
:rfc:`2109` cookies are parsed as Netscape cookies and subsequently treated
either as Netscape or RFC 2965 cookies according to the 'policy' in effect.
Note that the great majority of cookies on the internet are Netscape cookies.
:mod:`http.cookiejar` attempts to follow the de-facto Netscape cookie protocol (which
differs substantially from that set out in the original Netscape specification),
including taking note of the ``max-age`` and ``port`` cookie-attributes
introduced with RFC 2965.

.. note::

   The various named parameters found in :mailheader:`Set-Cookie` and
   :mailheader:`Set-Cookie2` headers (eg. ``domain`` and ``expires``) are
   conventionally referred to as :dfn:`attributes`.  To distinguish them from
   Python attributes, the documentation for this module uses the term
   :dfn:`cookie-attribute` instead.


The module defines the following exception:


.. exception:: LoadError

   Instances of :class:`FileCookieJar` raise this exception on failure to load
   cookies from a file.  :exc:`LoadError` is a subclass of :exc:`OSError`.

   .. versionchanged:: 3.3
      :exc:`LoadError` used to be a subtype of :exc:`IOError`, which is now an
      alias of :exc:`OSError`.


The following classes are provided:


.. class:: CookieJar(policy=None)

   *policy* is an object implementing the :class:`CookiePolicy` interface.

   The :class:`CookieJar` class stores HTTP cookies.  It extracts cookies from HTTP
   requests, and returns them in HTTP responses. :class:`CookieJar` instances
   automatically expire contained cookies when necessary.  Subclasses are also
   responsible for storing and retrieving cookies from a file or database.


.. class:: FileCookieJar(filename=None, delayload=None, policy=None)

   *policy* is an object implementing the :class:`CookiePolicy` interface.  For the
   other arguments, see the documentation for the corresponding attributes.

   A :class:`CookieJar` which can load cookies from, and perhaps save cookies to, a
   file on disk.  Cookies are **NOT** loaded from the named file until either the
   :meth:`load` or :meth:`revert` method is called.  Subclasses of this class are
   documented in section :ref:`file-cookie-jar-classes`.

   This should not be initialized directly – use its subclasses below instead.

   .. versionchanged:: 3.8

      The filename parameter supports a :term:`path-like object`.


.. class:: CookiePolicy()

   This class is responsible for deciding whether each cookie should be accepted
   from / returned to the server.


.. class:: DefaultCookiePolicy( blocked_domains=None, allowed_domains=None, netscape=True, rfc2965=False, rfc2109_as_netscape=None, hide_cookie2=False, strict_domain=False, strict_rfc2965_unverifiable=True, strict_ns_unverifiable=False, strict_ns_domain=DefaultCookiePolicy.DomainLiberal, strict_ns_set_initial_dollar=False, strict_ns_set_path=False, secure_protocols=("https", "wss") )

   Constructor arguments should be passed as keyword arguments only.
   *blocked_domains* is a sequence of domain names that we never accept cookies
   from, nor return cookies to. *allowed_domains* if not :const:`None`, this is a
   sequence of the only domains for which we accept and return cookies.
   *secure_protocols* is a sequence of protocols for which secure cookies can be
   added to. By default *https* and *wss* (secure websocket) are considered
   secure protocols. For all other arguments, see the documentation for
   :class:`CookiePolicy` and :class:`DefaultCookiePolicy` objects.

   :class:`DefaultCookiePolicy` implements the standard accept / reject rules for
   Netscape and :rfc:`2965` cookies.  By default, :rfc:`2109` cookies (ie. cookies
   received in a :mailheader:`Set-Cookie` header with a version cookie-attribute of
   1) are treated according to the RFC 2965 rules.  However, if RFC 2965 handling
   is turned off or :attr:`rfc2109_as_netscape` is ``True``, RFC 2109 cookies are
   'downgraded' by the :class:`CookieJar` instance to Netscape cookies, by
   setting the :attr:`version` attribute of the :class:`Cookie` instance to 0.
   :class:`DefaultCookiePolicy` also provides some parameters to allow some
   fine-tuning of policy.


.. class:: Cookie()

   This class represents Netscape, :rfc:`2109` and :rfc:`2965` cookies.  It is not
   expected that users of :mod:`http.cookiejar` construct their own :class:`Cookie`
   instances.  Instead, if necessary, call :meth:`make_cookies` on a
   :class:`CookieJar` instance.


.. seealso::

   Module :mod:`urllib.request`
      URL opening with automatic cookie handling.

   Module :mod:`http.cookies`
      HTTP cookie classes, principally useful for server-side code.  The
      :mod:`http.cookiejar` and :mod:`http.cookies` modules do not depend on each
      other.

   https://curl.se/rfc/cookie_spec.html
      The specification of the original Netscape cookie protocol.  Though this is
      still the dominant protocol, the 'Netscape cookie protocol' implemented by all
      the major browsers (and :mod:`http.cookiejar`) only bears a passing resemblance to
      the one sketched out in ``cookie_spec.html``.

   :rfc:`2109` - HTTP State Management Mechanism
      Obsoleted by :rfc:`2965`. Uses :mailheader:`Set-Cookie` with version=1.

   :rfc:`2965` - HTTP State Management Mechanism
      The Netscape protocol with the bugs fixed.  Uses :mailheader:`Set-Cookie2` in
      place of :mailheader:`Set-Cookie`.  Not widely used.

   http://kristol.org/cookie/errata.html
      Unfinished errata to :rfc:`2965`.

   :rfc:`2964` - Use of HTTP State Management

.. _cookie-jar-objects:

CookieJar and FileCookieJar Objects
-----------------------------------

:class:`CookieJar` objects support the :term:`iterator` protocol for iterating over
contained :class:`Cookie` objects.

:class:`CookieJar` has the following methods:


.. method:: CookieJar.add_cookie_header(request)

   Add correct :mailheader:`Cookie` header to *request*.

   If policy allows (ie. the :attr:`rfc2965` and :attr:`hide_cookie2` attributes of
   the :class:`CookieJar`'s :class:`CookiePolicy` instance are true and false
   respectively), the :mailheader:`Cookie2` header is also added when appropriate.

   The *request* object (usually a :class:`urllib.request.Request` instance)
   must support the methods :meth:`get_full_url`, :meth:`has_header`,
   :meth:`get_header`, :meth:`header_items`, :meth:`add_unredirected_header`
   and the attributes :attr:`host`, :attr:`!type`, :attr:`unverifiable`
   and :attr:`origin_req_host` as documented by :mod:`urllib.request`.

   .. versionchanged:: 3.3

    *request* object needs :attr:`origin_req_host` attribute. Dependency on a
    deprecated method :meth:`get_origin_req_host` has been removed.


.. method:: CookieJar.extract_cookies(response, request)

   Extract cookies from HTTP *response* and store them in the :class:`CookieJar`,
   where allowed by policy.

   The :class:`CookieJar` will look for allowable :mailheader:`Set-Cookie` and
   :mailheader:`Set-Cookie2` headers in the *response* argument, and store cookies
   as appropriate (subject to the :meth:`CookiePolicy.set_ok` method's approval).

   The *response* object (usually the result of a call to
   :meth:`urllib.request.urlopen`, or similar) should support an :meth:`info`
   method, which returns an :class:`email.message.Message` instance.

   The *request* object (usually a :class:`urllib.request.Request` instance)
   must support the method :meth:`get_full_url` and the attributes
   :attr:`host`, :attr:`unverifiable` and :attr:`origin_req_host`,
   as documented by :mod:`urllib.request`.  The request is used to set
   default values for cookie-attributes as well as for checking that the
   cookie is allowed to be set.

   .. versionchanged:: 3.3

    *request* object needs :attr:`origin_req_host` attribute. Dependency on a
    deprecated method :meth:`get_origin_req_host` has been removed.

.. method:: CookieJar.set_policy(policy)

   Set the :class:`CookiePolicy` instance to be used.


.. method:: CookieJar.make_cookies(response, request)

   Return sequence of :class:`Cookie` objects extracted from *response* object.

   See the documentation for :meth:`extract_cookies` for the interfaces required of
   the *response* and *request* arguments.


.. method:: CookieJar.set_cookie_if_ok(cookie, request)

   Set a :class:`Cookie` if policy says it's OK to do so.


.. method:: CookieJar.set_cookie(cookie)

   Set a :class:`Cookie`, without checking with policy to see whether or not it
   should be set.


.. method:: CookieJar.clear([domain[, path[, name]]])

   Clear some cookies.

   If invoked without arguments, clear all cookies.  If given a single argument,
   only cookies belonging to that *domain* will be removed. If given two arguments,
   cookies belonging to the specified *domain* and URL *path* are removed.  If
   given three arguments, then the cookie with the specified *domain*, *path* and
   *name* is removed.

   Raises :exc:`KeyError` if no matching cookie exists.


.. method:: CookieJar.clear_session_cookies()

   Discard all session cookies.

   Discards all contained cookies that have a true :attr:`discard` attribute
   (usually because they had either no ``max-age`` or ``expires`` cookie-attribute,
   or an explicit ``discard`` cookie-attribute).  For interactive browsers, the end
   of a session usually corresponds to closing the browser window.

   Note that the :meth:`save` method won't save session cookies anyway, unless you
   ask otherwise by passing a true *ignore_discard* argument.

:class:`FileCookieJar` implements the following additional methods:


.. method:: FileCookieJar.save(filename=None, ignore_discard=False, ignore_expires=False)

   Save cookies to a file.

   This base class raises :exc:`NotImplementedError`.  Subclasses may leave this
   method unimplemented.

   *filename* is the name of file in which to save cookies.  If *filename* is not
   specified, :attr:`self.filename` is used (whose default is the value passed to
   the constructor, if any); if :attr:`self.filename` is :const:`None`,
   :exc:`ValueError` is raised.

   *ignore_discard*: save even cookies set to be discarded. *ignore_expires*: save
   even cookies that have expired

   The file is overwritten if it already exists, thus wiping all the cookies it
   contains.  Saved cookies can be restored later using the :meth:`load` or
   :meth:`revert` methods.


.. method:: FileCookieJar.load(filename=None, ignore_discard=False, ignore_expires=False)

   Load cookies from a file.

   Old cookies are kept unless overwritten by newly loaded ones.

   Arguments are as for :meth:`save`.

   The named file must be in the format understood by the class, or
   :exc:`LoadError` will be raised.  Also, :exc:`OSError` may be raised, for
   example if the file does not exist.

   .. versionchanged:: 3.3
      :exc:`IOError` used to be raised, it is now an alias of :exc:`OSError`.


.. method:: FileCookieJar.revert(filename=None, ignore_discard=False, ignore_expires=False)

   Clear all cookies and reload cookies from a saved file.

   :meth:`revert` can raise the same exceptions as :meth:`load`. If there is a
   failure, the object's state will not be altered.

:class:`FileCookieJar` instances have the following public attributes:


.. attribute:: FileCookieJar.filename

   Filename of default file in which to keep cookies.  This attribute may be
   assigned to.


.. attribute:: FileCookieJar.delayload

   If true, load cookies lazily from disk.  This attribute should not be assigned
   to.  This is only a hint, since this only affects performance, not behaviour
   (unless the cookies on disk are changing). A :class:`CookieJar` object may
   ignore it.  None of the :class:`FileCookieJar` classes included in the standard
   library lazily loads cookies.


.. _file-cookie-jar-classes:

FileCookieJar subclasses and co-operation with web browsers
-----------------------------------------------------------

The following :class:`CookieJar` subclasses are provided for reading and
writing.

.. class:: MozillaCookieJar(filename=None, delayload=None, policy=None)

   A :class:`FileCookieJar` that can load from and save cookies to disk in the
   Mozilla ``cookies.txt`` file format (which is also used by curl and the Lynx
   and Netscape browsers).

   .. note::

      This loses information about :rfc:`2965` cookies, and also about newer or
      non-standard cookie-attributes such as ``port``.

   .. warning::

      Back up your cookies before saving if you have cookies whose loss / corruption
      would be inconvenient (there are some subtleties which may lead to slight
      changes in the file over a load / save round-trip).

   Also note that cookies saved while Mozilla is running will get clobbered by
   Mozilla.


.. class:: LWPCookieJar(filename=None, delayload=None, policy=None)

   A :class:`FileCookieJar` that can load from and save cookies to disk in format
   compatible with the libwww-perl library's ``Set-Cookie3`` file format.  This is
   convenient if you want to store cookies in a human-readable file.

   .. versionchanged:: 3.8

      The filename parameter supports a :term:`path-like object`.

.. _cookie-policy-objects:

CookiePolicy Objects
--------------------

Objects implementing the :class:`CookiePolicy` interface have the following
methods:


.. method:: CookiePolicy.set_ok(cookie, request)

   Return boolean value indicating whether cookie should be accepted from server.

   *cookie* is a :class:`Cookie` instance.  *request* is an object
   implementing the interface defined by the documentation for
   :meth:`CookieJar.extract_cookies`.


.. method:: CookiePolicy.return_ok(cookie, request)

   Return boolean value indicating whether cookie should be returned to server.

   *cookie* is a :class:`Cookie` instance.  *request* is an object
   implementing the interface defined by the documentation for
   :meth:`CookieJar.add_cookie_header`.


.. method:: CookiePolicy.domain_return_ok(domain, request)

   Return ``False`` if cookies should not be returned, given cookie domain.

   This method is an optimization.  It removes the need for checking every cookie
   with a particular domain (which might involve reading many files).  Returning
   true from :meth:`domain_return_ok` and :meth:`path_return_ok` leaves all the
   work to :meth:`return_ok`.

   If :meth:`domain_return_ok` returns true for the cookie domain,
   :meth:`path_return_ok` is called for the cookie path.  Otherwise,
   :meth:`path_return_ok` and :meth:`return_ok` are never called for that cookie
   domain.  If :meth:`path_return_ok` returns true, :meth:`return_ok` is called
   with the :class:`Cookie` object itself for a full check.  Otherwise,
   :meth:`return_ok` is never called for that cookie path.

   Note that :meth:`domain_return_ok` is called for every *cookie* domain, not just
   for the *request* domain.  For example, the function might be called with both
   ``".example.com"`` and ``"www.example.com"`` if the request domain is
   ``"www.example.com"``.  The same goes for :meth:`path_return_ok`.

   The *request* argument is as documented for :meth:`return_ok`.


.. method:: CookiePolicy.path_return_ok(path, request)

   Return ``False`` if cookies should not be returned, given cookie path.

   See the documentation for :meth:`domain_return_ok`.

In addition to implementing the methods above, implementations of the
:class:`CookiePolicy` interface must also supply the following attributes,
indicating which protocols should be used, and how.  All of these attributes may
be assigned to.


.. attribute:: CookiePolicy.netscape

   Implement Netscape protocol.


.. attribute:: CookiePolicy.rfc2965

   Implement :rfc:`2965` protocol.


.. attribute:: CookiePolicy.hide_cookie2

   Don't add :mailheader:`Cookie2` header to requests (the presence of this header
   indicates to the server that we understand :rfc:`2965` cookies).

The most useful way to define a :class:`CookiePolicy` class is by subclassing
from :class:`DefaultCookiePolicy` and overriding some or all of the methods
above.  :class:`CookiePolicy` itself may be used as a 'null policy' to allow
setting and receiving any and all cookies (this is unlikely to be useful).


.. _default-cookie-policy-objects:

DefaultCookiePolicy Objects
---------------------------

Implements the standard rules for accepting and returning cookies.

Both :rfc:`2965` and Netscape cookies are covered.  RFC 2965 handling is switched
off by default.

The easiest way to provide your own policy is to override this class and call
its methods in your overridden implementations before adding your own additional
checks::

   import http.cookiejar
   class MyCookiePolicy(http.cookiejar.DefaultCookiePolicy):
       def set_ok(self, cookie, request):
           if not http.cookiejar.DefaultCookiePolicy.set_ok(self, cookie, request):
               return False
           if i_dont_want_to_store_this_cookie(cookie):
               return False
           return True

In addition to the features required to implement the :class:`CookiePolicy`
interface, this class allows you to block and allow domains from setting and
receiving cookies.  There are also some strictness switches that allow you to
tighten up the rather loose Netscape protocol rules a little bit (at the cost of
blocking some benign cookies).

A domain blocklist and allowlist is provided (both off by default). Only domains
not in the blocklist and present in the allowlist (if the allowlist is active)
participate in cookie setting and returning.  Use the *blocked_domains*
constructor argument, and :meth:`blocked_domains` and
:meth:`set_blocked_domains` methods (and the corresponding argument and methods
for *allowed_domains*).  If you set an allowlist, you can turn it off again by
setting it to :const:`None`.

Domains in block or allow lists that do not start with a dot must equal the
cookie domain to be matched.  For example, ``"example.com"`` matches a blocklist
entry of ``"example.com"``, but ``"www.example.com"`` does not.  Domains that do
start with a dot are matched by more specific domains too. For example, both
``"www.example.com"`` and ``"www.coyote.example.com"`` match ``".example.com"``
(but ``"example.com"`` itself does not).  IP addresses are an exception, and
must match exactly.  For example, if blocked_domains contains ``"192.168.1.2"``
and ``".168.1.2"``, 192.168.1.2 is blocked, but 193.168.1.2 is not.

:class:`DefaultCookiePolicy` implements the following additional methods:


.. method:: DefaultCookiePolicy.blocked_domains()

   Return the sequence of blocked domains (as a tuple).


.. method:: DefaultCookiePolicy.set_blocked_domains(blocked_domains)

   Set the sequence of blocked domains.


.. method:: DefaultCookiePolicy.is_blocked(domain)

   Return ``True`` if *domain* is on the blocklist for setting or receiving
   cookies.


.. method:: DefaultCookiePolicy.allowed_domains()

   Return :const:`None`, or the sequence of allowed domains (as a tuple).


.. method:: DefaultCookiePolicy.set_allowed_domains(allowed_domains)

   Set the sequence of allowed domains, or :const:`None`.


.. method:: DefaultCookiePolicy.is_not_allowed(domain)

   Return ``True`` if *domain* is not on the allowlist for setting or receiving
   cookies.

:class:`DefaultCookiePolicy` instances have the following attributes, which are
all initialised from the constructor arguments of the same name, and which may
all be assigned to.


.. attribute:: DefaultCookiePolicy.rfc2109_as_netscape

   If true, request that the :class:`CookieJar` instance downgrade :rfc:`2109` cookies
   (ie. cookies received in a :mailheader:`Set-Cookie` header with a version
   cookie-attribute of 1) to Netscape cookies by setting the version attribute of
   the :class:`Cookie` instance to 0.  The default value is :const:`None`, in which
   case RFC 2109 cookies are downgraded if and only if :rfc:`2965` handling is turned
   off.  Therefore, RFC 2109 cookies are downgraded by default.


General strictness switches:

.. attribute:: DefaultCookiePolicy.strict_domain

   Don't allow sites to set two-component domains with country-code top-level
   domains like ``.co.uk``, ``.gov.uk``, ``.co.nz``.etc.  This is far from perfect
   and isn't guaranteed to work!


:rfc:`2965` protocol strictness switches:

.. attribute:: DefaultCookiePolicy.strict_rfc2965_unverifiable

   Follow :rfc:`2965` rules on unverifiable transactions (usually, an unverifiable
   transaction is one resulting from a redirect or a request for an image hosted on
   another site).  If this is false, cookies are *never* blocked on the basis of
   verifiability


Netscape protocol strictness switches:

.. attribute:: DefaultCookiePolicy.strict_ns_unverifiable

   Apply :rfc:`2965` rules on unverifiable transactions even to Netscape cookies.


.. attribute:: DefaultCookiePolicy.strict_ns_domain

   Flags indicating how strict to be with domain-matching rules for Netscape
   cookies.  See below for acceptable values.


.. attribute:: DefaultCookiePolicy.strict_ns_set_initial_dollar

   Ignore cookies in Set-Cookie: headers that have names starting with ``'$'``.


.. attribute:: DefaultCookiePolicy.strict_ns_set_path

   Don't allow setting cookies whose path doesn't path-match request URI.

:attr:`strict_ns_domain` is a collection of flags.  Its value is constructed by
or-ing together (for example, ``DomainStrictNoDots|DomainStrictNonDomain`` means
both flags are set).


.. attribute:: DefaultCookiePolicy.DomainStrictNoDots

   When setting cookies, the 'host prefix' must not contain a dot (eg.
   ``www.foo.bar.com`` can't set a cookie for ``.bar.com``, because ``www.foo``
   contains a dot).


.. attribute:: DefaultCookiePolicy.DomainStrictNonDomain

   Cookies that did not explicitly specify a ``domain`` cookie-attribute can only
   be returned to a domain equal to the domain that set the cookie (eg.
   ``spam.example.com`` won't be returned cookies from ``example.com`` that had no
   ``domain`` cookie-attribute).


.. attribute:: DefaultCookiePolicy.DomainRFC2965Match

   When setting cookies, require a full :rfc:`2965` domain-match.

The following attributes are provided for convenience, and are the most useful
combinations of the above flags:


.. attribute:: DefaultCookiePolicy.DomainLiberal

   Equivalent to 0 (ie. all of the above Netscape domain strictness flags switched
   off).


.. attribute:: DefaultCookiePolicy.DomainStrict

   Equivalent to ``DomainStrictNoDots|DomainStrictNonDomain``.


Cookie Objects
--------------

:class:`Cookie` instances have Python attributes roughly corresponding to the
standard cookie-attributes specified in the various cookie standards.  The
correspondence is not one-to-one, because there are complicated rules for
assigning default values, because the ``max-age`` and ``expires``
cookie-attributes contain equivalent information, and because :rfc:`2109` cookies
may be 'downgraded' by :mod:`http.cookiejar` from version 1 to version 0 (Netscape)
cookies.

Assignment to these attributes should not be necessary other than in rare
circumstances in a :class:`CookiePolicy` method.  The class does not enforce
internal consistency, so you should know what you're doing if you do that.


.. attribute:: Cookie.version

   Integer or :const:`None`.  Netscape cookies have :attr:`version` 0. :rfc:`2965` and
   :rfc:`2109` cookies have a ``version`` cookie-attribute of 1.  However, note that
   :mod:`http.cookiejar` may 'downgrade' RFC 2109 cookies to Netscape cookies, in which
   case :attr:`version` is 0.


.. attribute:: Cookie.name

   Cookie name (a string).


.. attribute:: Cookie.value

   Cookie value (a string), or :const:`None`.


.. attribute:: Cookie.port

   String representing a port or a set of ports (eg. '80', or '80,8080'), or
   :const:`None`.


.. attribute:: Cookie.domain

   Cookie domain (a string).


.. attribute:: Cookie.path

   Cookie path (a string, eg. ``'/acme/rocket_launchers'``).


.. attribute:: Cookie.secure

   ``True`` if cookie should only be returned over a secure connection.


.. attribute:: Cookie.expires

   Integer expiry date in seconds since epoch, or :const:`None`.  See also the
   :meth:`is_expired` method.


.. attribute:: Cookie.discard

   ``True`` if this is a session cookie.


.. attribute:: Cookie.comment

   String comment from the server explaining the function of this cookie, or
   :const:`None`.


.. attribute:: Cookie.comment_url

   URL linking to a comment from the server explaining the function of this cookie,
   or :const:`None`.


.. attribute:: Cookie.rfc2109

   ``True`` if this cookie was received as an :rfc:`2109` cookie (ie. the cookie
   arrived in a :mailheader:`Set-Cookie` header, and the value of the Version
   cookie-attribute in that header was 1).  This attribute is provided because
   :mod:`http.cookiejar` may 'downgrade' RFC 2109 cookies to Netscape cookies, in
   which case :attr:`version` is 0.


.. attribute:: Cookie.port_specified

   ``True`` if a port or set of ports was explicitly specified by the server (in the
   :mailheader:`Set-Cookie` / :mailheader:`Set-Cookie2` header).


.. attribute:: Cookie.domain_specified

   ``True`` if a domain was explicitly specified by the server.


.. attribute:: Cookie.domain_initial_dot

   ``True`` if the domain explicitly specified by the server began with a dot
   (``'.'``).

Cookies may have additional non-standard cookie-attributes.  These may be
accessed using the following methods:


.. method:: Cookie.has_nonstandard_attr(name)

   Return ``True`` if cookie has the named cookie-attribute.


.. method:: Cookie.get_nonstandard_attr(name, default=None)

   If cookie has the named cookie-attribute, return its value. Otherwise, return
   *default*.


.. method:: Cookie.set_nonstandard_attr(name, value)

   Set the value of the named cookie-attribute.

The :class:`Cookie` class also defines the following method:


.. method:: Cookie.is_expired(now=None)

   ``True`` if cookie has passed the time at which the server requested it should
   expire.  If *now* is given (in seconds since the epoch), return whether the
   cookie has expired at the specified time.


Examples
--------

The first example shows the most common usage of :mod:`http.cookiejar`::

   import http.cookiejar, urllib.request
   cj = http.cookiejar.CookieJar()
   opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))
   r = opener.open("http://example.com/")

This example illustrates how to open a URL using your Netscape, Mozilla, or Lynx
cookies (assumes Unix/Netscape convention for location of the cookies file)::

   import os, http.cookiejar, urllib.request
   cj = http.cookiejar.MozillaCookieJar()
   cj.load(os.path.join(os.path.expanduser("~"), ".netscape", "cookies.txt"))
   opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))
   r = opener.open("http://example.com/")

The next example illustrates the use of :class:`DefaultCookiePolicy`. Turn on
:rfc:`2965` cookies, be more strict about domains when setting and returning
Netscape cookies, and block some domains from setting cookies or having them
returned::

   import urllib.request
   from http.cookiejar import CookieJar, DefaultCookiePolicy
   policy = DefaultCookiePolicy(
       rfc2965=True, strict_ns_domain=Policy.DomainStrict,
       blocked_domains=["ads.net", ".ads.net"])
   cj = CookieJar(policy)
   opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))
   r = opener.open("http://example.com/")
