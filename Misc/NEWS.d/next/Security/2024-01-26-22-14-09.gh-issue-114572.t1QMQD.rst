:meth:`ssl.SSLContext.cert_store_stats` and
:meth:`ssl.SSLContext.get_ca_certs` now correctly lock access to the
certificate store, when the :class:`ssl.SSLContext` is shared across
multiple threads.
