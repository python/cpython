:mod:`importlib.abc` -- Abstract base classes related to import
---------------------------------------------------------------

.. module:: importlib.abc
    :synopsis: Abstract base classes related to import

**Source code:** :source:`Lib/importlib/abc.py`

--------------


The :mod:`importlib.abc` module contains all of the core abstract base classes
used by :keyword:`import`. Some subclasses of the core abstract base classes
are also provided to help in implementing the core ABCs.

ABC hierarchy::

    object
     +-- Finder (deprecated)
     |    +-- MetaPathFinder
     |    +-- PathEntryFinder
     +-- Loader
          +-- ResourceLoader --------+
          +-- InspectLoader          |
               +-- ExecutionLoader --+
                                     +-- FileLoader
                                     +-- SourceLoader


.. class:: Finder

   An abstract base class representing a :term:`finder`.

   .. deprecated:: 3.3
      Use :class:`MetaPathFinder` or :class:`PathEntryFinder` instead.

   .. abstractmethod:: find_module(fullname, path=None)

      An abstract method for finding a :term:`loader` for the specified
      module.  Originally specified in :pep:`302`, this method was meant
      for use in :data:`sys.meta_path` and in the path-based import subsystem.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of raising
         :exc:`NotImplementedError`.

      .. deprecated:: 3.10
         Implement :meth:`MetaPathFinder.find_spec` or
         :meth:`PathEntryFinder.find_spec` instead.


.. class:: MetaPathFinder

   An abstract base class representing a :term:`meta path finder`. For
   compatibility, this is a subclass of :class:`Finder`.

   .. versionadded:: 3.3

   .. versionchanged:: 3.10
      No longer a subclass of :class:`Finder`.

   .. method:: find_spec(fullname, path, target=None)

      An abstract method for finding a :term:`spec <module spec>` for
      the specified module.  If this is a top-level import, *path* will
      be ``None``.  Otherwise, this is a search for a subpackage or
      module and *path* will be the value of :attr:`__path__` from the
      parent package. If a spec cannot be found, ``None`` is returned.
      When passed in, ``target`` is a module object that the finder may
      use to make a more educated guess about what spec to return.
      :func:`importlib.util.spec_from_loader` may be useful for implementing
      concrete ``MetaPathFinders``.

      .. versionadded:: 3.4

   .. method:: find_module(fullname, path)

      A legacy method for finding a :term:`loader` for the specified
      module.  If this is a top-level import, *path* will be ``None``.
      Otherwise, this is a search for a subpackage or module and *path*
      will be the value of :attr:`__path__` from the parent
      package. If a loader cannot be found, ``None`` is returned.

      If :meth:`find_spec` is defined, backwards-compatible functionality is
      provided.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of raising
         :exc:`NotImplementedError`. Can use :meth:`find_spec` to provide
         functionality.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. method:: invalidate_caches()

      An optional method which, when called, should invalidate any internal
      cache used by the finder. Used by :func:`importlib.invalidate_caches`
      when invalidating the caches of all finders on :data:`sys.meta_path`.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of ``NotImplemented``.


.. class:: PathEntryFinder

   An abstract base class representing a :term:`path entry finder`.  Though
   it bears some similarities to :class:`MetaPathFinder`, ``PathEntryFinder``
   is meant for use only within the path-based import subsystem provided
   by :class:`importlib.machinery.PathFinder`.

   .. versionadded:: 3.3

   .. versionchanged:: 3.10
      No longer a subclass of :class:`Finder`.

   .. method:: find_spec(fullname, target=None)

      An abstract method for finding a :term:`spec <module spec>` for
      the specified module.  The finder will search for the module only
      within the :term:`path entry` to which it is assigned.  If a spec
      cannot be found, ``None`` is returned.  When passed in, ``target``
      is a module object that the finder may use to make a more educated
      guess about what spec to return. :func:`importlib.util.spec_from_loader`
      may be useful for implementing concrete ``PathEntryFinders``.

      .. versionadded:: 3.4

   .. method:: find_loader(fullname)

      A legacy method for finding a :term:`loader` for the specified
      module.  Returns a 2-tuple of ``(loader, portion)`` where ``portion``
      is a sequence of file system locations contributing to part of a namespace
      package. The loader may be ``None`` while specifying ``portion`` to
      signify the contribution of the file system locations to a namespace
      package. An empty list can be used for ``portion`` to signify the loader
      is not part of a namespace package. If ``loader`` is ``None`` and
      ``portion`` is the empty list then no loader or location for a namespace
      package were found (i.e. failure to find anything for the module).

      If :meth:`find_spec` is defined then backwards-compatible functionality is
      provided.

      .. versionchanged:: 3.4
         Returns ``(None, [])`` instead of raising :exc:`NotImplementedError`.
         Uses :meth:`find_spec` when available to provide functionality.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. method:: find_module(fullname)

      A concrete implementation of :meth:`Finder.find_module` which is
      equivalent to ``self.find_loader(fullname)[0]``.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. method:: invalidate_caches()

      An optional method which, when called, should invalidate any internal
      cache used by the finder. Used by
      :meth:`importlib.machinery.PathFinder.invalidate_caches`
      when invalidating the caches of all cached finders.


.. class:: Loader

    An abstract base class for a :term:`loader`.
    See :pep:`302` for the exact definition for a loader.

    Loaders that wish to support resource reading should implement a
    :meth:`get_resource_reader` method as specified by
    :class:`importlib.abc.ResourceReader`.

    .. versionchanged:: 3.7
       Introduced the optional :meth:`get_resource_reader` method.

    .. method:: create_module(spec)

       A method that returns the module object to use when
       importing a module.  This method may return ``None``,
       indicating that default module creation semantics should take place.

       .. versionadded:: 3.4

       .. versionchanged:: 3.5
          Starting in Python 3.6, this method will not be optional when
          :meth:`exec_module` is defined.

    .. method:: exec_module(module)

       An abstract method that executes the module in its own namespace
       when a module is imported or reloaded.  The module should already
       be initialized when :meth:`exec_module` is called.  When this method exists,
       :meth:`create_module` must be defined.

       .. versionadded:: 3.4

       .. versionchanged:: 3.6
          :meth:`create_module` must also be defined.

    .. method:: load_module(fullname)

        A legacy method for loading a module.  If the module cannot be
        loaded, :exc:`ImportError` is raised, otherwise the loaded module is
        returned.

        If the requested module already exists in :data:`sys.modules`, that
        module should be used and reloaded.
        Otherwise the loader should create a new module and insert it into
        :data:`sys.modules` before any loading begins, to prevent recursion
        from the import.  If the loader inserted a module and the load fails, it
        must be removed by the loader from :data:`sys.modules`; modules already
        in :data:`sys.modules` before the loader began execution should be left
        alone (see :func:`importlib.util.module_for_loader`).

        The loader should set several attributes on the module
        (note that some of these attributes can change when a module is
        reloaded):

        - :attr:`__name__`
            The module's fully-qualified name.
            It is ``'__main__'`` for an executed module.

        - :attr:`__file__`
            The location the :term:`loader` used to load the module.
            For example, for modules loaded from a .py file this is the filename.
            It is not set on all modules (e.g. built-in modules).

        - :attr:`__cached__`
            The filename of a compiled version of the module's code.
            It is not set on all modules (e.g. built-in modules).

        - :attr:`__path__`
            The list of locations where the package's submodules will be found.
            Most of the time this is a single directory.
            The import system passes this attribute to ``__import__()`` and to finders
            in the same way as :attr:`sys.path` but just for the package.
            It is not set on non-package modules so it can be used
            as an indicator that the module is a package.

        - :attr:`__package__`
            The fully-qualified name of the package the module is in (or the
            empty string for a top-level module).
            If the module is a package then this is the same as :attr:`__name__`.

        - :attr:`__loader__`
            The :term:`loader` used to load the module.

        When :meth:`exec_module` is available then backwards-compatible
        functionality is provided.

        .. versionchanged:: 3.4
           Raise :exc:`ImportError` when called instead of
           :exc:`NotImplementedError`.  Functionality provided when
           :meth:`exec_module` is available.

        .. deprecated:: 3.4
           The recommended API for loading a module is :meth:`exec_module`
           (and :meth:`create_module`).  Loaders should implement it instead of
           :meth:`load_module`.  The import machinery takes care of all the
           other responsibilities of :meth:`load_module` when
           :meth:`exec_module` is implemented.

    .. method:: module_repr(module)

        A legacy method which when implemented calculates and returns the given
        module's representation, as a string.  The module type's default
        :meth:`__repr__` will use the result of this method as appropriate.

        .. versionadded:: 3.3

        .. versionchanged:: 3.4
           Made optional instead of an abstractmethod.

        .. deprecated:: 3.4
           The import machinery now takes care of this automatically.


.. include:: importlib.resources.abc.rst
