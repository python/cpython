==============================================
 HOWTO Fetch Internet Resources Using urllib2
==============================================
------------------------------------------
  Fetching URLs With Python
------------------------------------------


.. note::

    There is an French translation of this HOWTO, available at `urllib2 - Le Manuel manquant <http://www.voidspace/python/urllib2_francais.shtml>`_.

.. contents:: urllib2 Tutorial
 

Introduction
============

.. sidebar:: Related Articles

    You may also find useful the following articles on fetching web resources with Python :
    
    * `Basic Authentication <http://www.voidspace.org.uk/python/articles/authentication.shtml>`_
    
        A tutorial on *Basic Authentication*, with exampels in Python.
    
    * `cookielib and ClientCookie <http://www.voidspace.org.uk/python/articles/cookielib.shtml>`_
    
        How to handle cookies, when fetching web pages with Python.
   
    This HOWTO is written by `Michael Foord <http://www.voidspace.org.uk/python/index.shtml>`_.

**urllib2** is a Python_ module for fetching URLs (Uniform Resource Locators). It offers a very simple interface, in the form of the *urlopen* function. This is capable of fetching URLs using a variety of different protocols. It also offers a slightly more complex interface for handling common situations - like basic authentication, cookies, proxies, and so on. These are provided by objects called handlers and openers.

For straightforward situations *urlopen* is very easy to use. But as soon as you encounter errors, or non-trivial cases, you will need some understanding of the HyperText Transfer Protocol. The most comprehensive reference to HTTP is :RFC:`2616`. This is a technical document and not intended to be easy to read. This HOWTO aims to illustrate using *urllib2*, with enough detail about HTTP to help you through. It is not intended to replace the `urllib2 docs`_ [#]_, but is supplementary to them. 


Fetching URLs
=============

HTTP is based on requests and responses - the client makes requests and servers send responses. Python mirrors this by having you form a ``Request`` object which represents the request you are making. In it's simplest form you create a Request object that specifies the URL you want to fetch [#]_. Calling ``urlopen`` with this Request object returns a handle on the page requested. This handle is a file like object : ::

    import urllib2
    
    the_url = 'http://www.voidspace.org.uk'
    req = urllib2.Request(the_url)
    handle = urllib2.urlopen(req)
    the_page = handle.read()
    
There are two extra things that Request objects allow you to do. Sometimes you want to **POST** data to a CGI (Common Gateway Interface) [#]_ or other web application. This is what your browser does when you fill in a FORM on the web. You may be mimicking a FORM submission, or transmitting data to your own application. In either case the data needs to be encoded for safe transmission over HTTP, and then passed to the Request object as the ``data`` argument. The encoding is done using a function from the ``urllib`` library *not* from ``urllib2``. ::

    import urllib
    import urllib2  
    
    the_url = 'http://www.someserver.com/cgi-bin/register.cgi'
    values = {'name' : 'Michael Foord',
              'location' : 'Northampton',
              'language' : 'Python' }
    
    data = urllib.urlencode(values)
    req = urllib2.Request(the_url, data)
    handle = urllib2.urlopen(req)
    the_page = handle.read()    

Some websites [#]_ dislike being browsed by programs, or send different versions to different browsers [#]_ . By default urllib2 identifies itself as ``Python-urllib/2.4``, which may confuse the site, or just plain not work. The way a browser identifies itself is through the ``User-Agent`` header [#]_. When you create a Request object you can pass a dictionary of headers in. The following example makes the same request as above, but identifies itself as a version of Internet Explorer [#]_. ::

    import urllib
    import urllib2  
    
    the_url = 'http://www.someserver.com/cgi-bin/register.cgi'
    user_agent = 'Mozilla/4.0 (compatible; MSIE 5.5; Windows NT)' 
    values = {'name' : 'Michael Foord',
              'location' : 'Northampton',
              'language' : 'Python' }
    headers = { 'User-Agent' : user_agent }
    
    data = urllib.urlencode(values)
    req = urllib2.Request(the_url, data, headers)
    handle = urllib2.urlopen(req)
    the_page = handle.read()

The handle also has two useful methods. See the section on `info and geturl`_ which comes after we have a look at what happens when things go wrong.


Coping With Errors
==================

*urlopen* raises ``URLError`` or ``HTTPError`` in the event of an error. ``HTTPError`` is a subclass of ``URLError``, which is a subclass of ``IOError``. This means you can trap for ``IOError`` if you want. ::

    req = urllib2.Request(some_url)
    try:
        handle = urllib2.urlopen(req)
    except IOError:
        print 'Something went wrong'
    else:
        print handle.read()

URLError
--------

If the request fails to reach a server then urlopen will raise a ``URLError``. This will usually be because there is no network connection (no route to the specified server), or the specified server doesn't exist.

In this case, the exception raised will have a 'reason' attribute, which is a tuple containing an error code and a text error message. 

e.g. ::

    >>> req = urllib2.Request('http://www.pretend_server.org')
    >>> try: urllib2.urlopen(req)
    >>> except IOError, e:
    >>>    print e.reason
    >>>
    (4, 'getaddrinfo failed')


HTTPError
---------

If the request reaches a server, but the server is unable to fulfil the request, it returns an error code. The default handlers will hande some of these errors for you. For those it can't handle, urlopen will raise an ``HTTPError``. Typical errors include '404' (page not found), '403' (request forbidden), and '401' (authentication required).

See http://www.w3.org/Protocols/HTTP/HTRESP.html for a reference on all the http error codes.

The ``HTTPError`` instance raised will have an integer 'code' attribute, which corresponds to the error sent by the server.

There is a useful dictionary of response codes in ``HTTPBaseServer``, that shows all the defined response codes. Because the default handlers handle redirects (codes in the 300 range), and codes in the 100-299 range indicate success, you will usually only see error codes in the 400-599 range.

Error Codes
~~~~~~~~~~~

.. note::

    As of Python 2.5 a dictionary like this one has become part of ``urllib2``.

::

    # Table mapping response codes to messages; entries have the
    # form {code: (shortmessage, longmessage)}.
    httpresponses = {
        100: ('Continue', 'Request received, please continue'),
        101: ('Switching Protocols',
              'Switching to new protocol; obey Upgrade header'),

        200: ('OK', 'Request fulfilled, document follows'),
        201: ('Created', 'Document created, URL follows'),
        202: ('Accepted',
              'Request accepted, processing continues off-line'),
        203: ('Non-Authoritative Information', 
                'Request fulfilled from cache'),
        204: ('No response', 'Request fulfilled, nothing follows'),
        205: ('Reset Content', 'Clear input form for further input.'),
        206: ('Partial Content', 'Partial content follows.'),

        300: ('Multiple Choices',
              'Object has several resources -- see URI list'),
        301: ('Moved Permanently', 
                'Object moved permanently -- see URI list'),
        302: ('Found', 'Object moved temporarily -- see URI list'),
        303: ('See Other', 'Object moved -- see Method and URL list'),
        304: ('Not modified',
              'Document has not changed since given time'),
        305: ('Use Proxy',
                'You must use proxy specified in Location'
                ' to access this resource.'),
        307: ('Temporary Redirect',
              'Object moved temporarily -- see URI list'),
              
        400: ('Bad request',
              'Bad request syntax or unsupported method'),
        401: ('Unauthorized',
              'No permission -- see authorization schemes'),
        402: ('Payment required',
              'No payment -- see charging schemes'),
        403: ('Forbidden',
              'Request forbidden -- authorization will not help'),
        404: ('Not Found', 'Nothing matches the given URI'),
        405: ('Method Not Allowed',
              'Specified method is invalid for this server.'),
        406: ('Not Acceptable', 
                'URI not available in preferred format.'),
        407: ('Proxy Authentication Required', 
                'You must authenticate with '
                'this proxy before proceeding.'),
        408: ('Request Time-out', 
                'Request timed out; try again later.'),
        409: ('Conflict', 'Request conflict.'),
        410: ('Gone',
              'URI no longer exists and has been permanently removed.'),
        411: ('Length Required', 'Client must specify Content-Length.'),
        412: ('Precondition Failed', 
                'Precondition in headers is false.'),
        413: ('Request Entity Too Large', 'Entity is too large.'),
        414: ('Request-URI Too Long', 'URI is too long.'),
        415: ('Unsupported Media Type', 
                'Entity body in unsupported format.'),
        416: ('Requested Range Not Satisfiable',
              'Cannot satisfy request range.'),
        417: ('Expectation Failed',
              'Expect condition could not be satisfied.'),

        500: ('Internal error', 'Server got itself in trouble'),
        501: ('Not Implemented',
              'Server does not support this operation'),
        502: ('Bad Gateway', 
                'Invalid responses from another server/proxy.'),
        503: ('Service temporarily overloaded',
              'The server cannot '
              'process the request due to a high load'),
        504: ('Gateway timeout',
              'The gateway server did not receive a timely response'),
        505: ('HTTP Version not supported', 'Cannot fulfill request.'),
        }

When an error is raised the server responds by returning an http error code *and* an error page. You can use the ``HTTPError`` instance as a handle on the page returned. This means that as well as the code attribute, it also has read, geturl, and info, methods. ::

    >>> req = urllib2.Request('http://www.python.org/fish.html')
    >>> try: 
    >>>     urllib2.urlopen(req)
    >>> except IOError, e:
    >>>     print e.code
    >>>     print e.read()
    >>> 
    404
    <!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" 
        "http://www.w3.org/TR/html4/loose.dtd">
    <?xml-stylesheet href="./css/ht2html.css" 
        type="text/css"?>
    <html><head><title>Error 404: File Not Found</title> 
    ...... etc...

Wrapping it Up
--------------

So if you want to be prepared for ``HTTPError`` *or* ``URLError`` there are two
basic approaches. I prefer the second approach.

Number 1
~~~~~~~~

::


    from urllib2 import Request, urlopen, URLError, HTTPError
    req = Request(someurl)
    try:
        handle = urlopen(req)
    except HTTPError, e:
        print 'The server couldn\'t fulfill the request.'
        print 'Error code: ', e.code
    except URLError, e:
        print 'We failed to reach a server.'
        print 'Reason: ', e.reason
    else:
        # everything is fine


.. note::

    The ``except HTTPError`` *must* come first, otherwise ``except URLError`` will *also* catch an ``HTTPError``.

Number 2
~~~~~~~~

::

    from urllib2 import Request, urlopen
    req = Request(someurl)
    try:
        handle = urlopen(req)
    except IOError, e:
        if hasattr(e, 'reason'):
            print 'We failed to reach a server.'
            print 'Reason: ', e.reason
        elif hasattr(e, 'code'):
            print 'The server couldn\'t fulfill the request.'
            print 'Error code: ', e.code
    else:
        # everything is fine
        

info and geturl
===============

The handle returned by urlopen (or the ``HTTPError`` instance) has two useful methods ``info`` and ``geturl``.

**geturl** - this returns the real url of the page fetched. This is useful because ``urlopen`` (or the openener object used) may have followed a redirect. The url of the page fetched may not be the same as the url requested.

**info** - this returns a dictionary like object  that describes the page fetched, particularly the headers sent by the server. It is actually an ``httplib.HTTPMessage`` instance. In versions of Python prior to 2.3.4 it wasn't safe to iterate over the object directly, so you should iterate over the list returned  by ``msg.keys()`` instead.

Typical headers include 'content-length', 'content-type', and so on. See the `Quick Reference to HTTP Headers`_ for a useful reference on the different sort of headers.


Openers and Handlers
====================

Openers and handlers are slightly esoteric parts of **urllib2**. When you fetch a URL you use an opener. Normally we have been using the default opener - via ``urlopen`` - but you can create custom openers. Openers use handlers.

``build_opener`` is used to create ``opener`` objects - for fetching URLs with specific handlers installed. Handlers can handle cookies, authentication, and other common but slightly specialised situations. Opener objects have an ``open`` method, which can be called directly to fetch urls in the same way as the ``urlopen`` function.

``install_opener`` can be used to make an ``opener`` object the default opener. This means that calls to ``urlopen`` will use the opener you have installed.


Basic Authentication
====================

To illustrate creating and installing a handler we will use the ``HTTPBasicAuthHandler``. For a more detailed discussion of this subject - including an explanation of how Basic Authentication works - see the `Basic Authentication Tutorial`_.

When authentication is required, the server sends a header (as well as the 401 error code) requesting authentication.  This specifies the authentication scheme and a 'realm'. The header looks like : ``www-authenticate: SCHEME realm="REALM"``. 

e.g. :: 

    www-authenticate: Basic realm="cPanel"


The client should then retry the request with the appropriate name and password for the realm included as a header in the request. This is 'basic authentication'. In order to simplify this process we can create an instance of ``HTTPBasicAuthHandler`` and an opener to use this handler. 

The ``HTTPBasicAuthHandler`` uses an object called a  password manager to handle the mapping of URIs and realms to passwords and usernames. If you know what the realm is (from the authentication header sent by the server), then you can use a ``HTTPPasswordMgr``. Generally there is only one realm per URI, so it is possible to use ``HTTPPasswordMgrWithDefaultRealm``. This allows you to specify a default username and password for a URI. This will be supplied in the absence of yoou providing an alternative combination for a specific realm. We signify this by providing ``None`` as the realm argument to the ``add_password`` method.

The toplevelurl is the first url that requires authentication. This is usually a 'super-url' of any others in the same realm. ::

    password_mgr = urllib2.HTTPPasswordMgrWithDefaultRealm()                        
    # create a password manager
    
    password_mgr.add_password(None, 
        top_level_url, username, password)              
    # add the username and password
    # if we knew the realm, we could
    # use it instead of ``None``
    
    handler = urllib2.HTTPBasicAuthHandler(password_mgr)                            
    # create the handler
    
    opener = urllib2.build_opener(handler)                       
    # from handler to opener

    opener.open(a_url)      
    # use the opener to fetch a URL

    urllib2.install_opener(opener)                               
    # install the opener
    # now all calls to urllib2.urlopen use our opener

.. note::

    In the above example we only supplied our ``HHTPBasicAuthHandler`` to ``build_opener``. By default openers have the handlers for normal situations - ``ProxyHandler``, ``UnknownHandler``, ``HTTPHandler``, ``HTTPDefaultErrorHandler``, ``HTTPRedirectHandler``, ``FTPHandler``, ``FileHandler``, ``HTTPErrorProcessor``. The only reason to explicitly supply these to ``build_opener`` (which chains handlers provided as a list), would be to change the order they appear in the chain.

One thing not to get bitten by is that the ``top_level_url`` in the code above *must not* contain the protocol - the ``http://`` part. So if the URL we are trying to access is ``http://www.someserver.com/path/page.html``, then we set : ::

    top_level_url = "www.someserver.com/path/page.html"
    # *no* http:// !!

It took me a long time to track that down the first time I tried to use handlers.


Proxies
=======

**urllib2** will auto-detect your proxy settings and use those. This is through the ``ProxyHandler`` which is part of the normal handler chain. Normally that's a good thing, but there are occasions when it may not be helpful [#]_. In order to do this we need to setup our own ``ProxyHandler``, with no proxies defined. This is done using similar steps to setting up a `Basic Authentication`_ handler : ::

    >>> proxy_support = urllib2.ProxyHandler({})
    >>> opener = urllib2.build_opener(proxy_support)
    >>> urllib2.install_opener(opener)

.. caution::

    Currently ``urllib2`` *does not* support fetching of ``https`` locations through
    a proxy. This can be a problem.

Sockets and Layers
==================

The Python support for fetching resources from the web is layered. urllib2 uses the httplib library, which in turn uses the socket library.

As of Python 2.3 you can specify how long a socket should wait for a response before timing out. This can be useful in applications which have to fetch web pages. By default the socket module has *no timeout* and can hang. To set the timeout use : ::

    import socket
    import urllib2
    
    timeout = 10
    # timeout in seconds
    socket.setdefaulttimeout(timeout) 

    req = urllib2.Request('http://www.voidspace.org.uk')
    handle = urllib2.urlopen(req)
    # this call to urllib2.urlopen
    # now uses the default timeout
    # we have set in the socket module    


-------


Footnotes
===========

.. [#] Possibly some of this tutorial will make it into the standard library docs for versions of Python after 2.4.1.
.. [#] You *can* fetch URLs directly with urlopen, without using a request object. It's more explicit, and therefore more Pythonic, to use ``urllib2.Request`` though. It also makes it easier to add headers to your request.
.. [#] For an introduction to the CGI protocol see `Writing Web Applications in Python`_. 
.. [#] Like Google for example. The *proper* way to use google from a program is to use PyGoogle_ of course. See `Voidspace Google`_ for some examples of using the Google API.
.. [#] Browser sniffing is a very bad practise for website design - building sites using web standards is much more sensible. Unfortunately a lot of sites still send different versions to different browsers.
.. [#] The user agent for MSIE 6 is *'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)'*
.. [#] For details of more HTTP request headers, see `Quick Reference to HTTP Headers`_.

.. [#] In my case I have to use a proxy to access the internet at work. If you attempt to fetch *localhost* URLs through this proxy it blocks them. IE is set to use the proxy, which urllib2 picks up on. In order to test scripts with a localhost server, I have to prevent urllib2 from using the proxy.  

.. _Python: http://www.python.org
.. _urllib2 docs: http://docs.python.org/lib/module-urllib2.html
.. _Quick Reference to HTTP Headers: http://www.cs.tut.fi/~jkorpela/http.html
.. _PyGoogle: http://pygoogle.sourceforge.net
.. _Voidspace Google: http://www.voidspace.org.uk/python/recipebook.shtml#google
.. _Writing Web Applications in Python: http://www.pyzine.com/Issue008/Section_Articles/article_CGIOne.html
.. _Basic Authentication Tutorial: http://www.voidspace.org.uk/python/articles/authentication.shtml