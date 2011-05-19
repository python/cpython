"""Convenient client for all PyPI APIs.

This module provides a ClientWrapper class which will use the "simple"
or XML-RPC API to request information or files from an index.
"""

from packaging.pypi import simple, xmlrpc

_WRAPPER_MAPPINGS = {'get_release': 'simple',
                     'get_releases': 'simple',
                     'search_projects': 'simple',
                     'get_metadata': 'xmlrpc',
                     'get_distributions': 'simple'}

_WRAPPER_INDEXES = {'xmlrpc': xmlrpc.Client,
                    'simple': simple.Crawler}


def switch_index_if_fails(func, wrapper):
    """Decorator that switch of index (for instance from xmlrpc to simple)
    if the first mirror return an empty list or raises an exception.
    """
    def decorator(*args, **kwargs):
        retry = True
        exception = None
        methods = [func]
        for f in wrapper._indexes.values():
            if f != func.__self__ and hasattr(f, func.__name__):
                methods.append(getattr(f, func.__name__))
        for method in methods:
            try:
                response = method(*args, **kwargs)
                retry = False
            except Exception as e:
                exception = e
            if not retry:
                break
        if retry and exception:
            raise exception
        else:
            return response
    return decorator


class ClientWrapper:
    """Wrapper around simple and xmlrpc clients,

    Choose the best implementation to use depending the needs, using the given
    mappings.
    If one of the indexes returns an error, tries to use others indexes.

    :param index: tell which index to rely on by default.
    :param index_classes: a dict of name:class to use as indexes.
    :param indexes: a dict of name:index already instantiated
    :param mappings: the mappings to use for this wrapper
    """

    def __init__(self, default_index='simple', index_classes=_WRAPPER_INDEXES,
                 indexes={}, mappings=_WRAPPER_MAPPINGS):
        self._projects = {}
        self._mappings = mappings
        self._indexes = indexes
        self._default_index = default_index

        # instantiate the classes and set their _project attribute to the one
        # of the wrapper.
        for name, cls in index_classes.items():
            obj = self._indexes.setdefault(name, cls())
            obj._projects = self._projects
            obj._index = self

    def __getattr__(self, method_name):
        """When asking for methods of the wrapper, return the implementation of
        the wrapped classes, depending the mapping.

        Decorate the methods to switch of implementation if an error occurs
        """
        real_method = None
        if method_name in _WRAPPER_MAPPINGS:
            obj = self._indexes[_WRAPPER_MAPPINGS[method_name]]
            real_method = getattr(obj, method_name)
        else:
            # the method is not defined in the mappings, so we try first to get
            # it via the default index, and rely on others if needed.
            try:
                real_method = getattr(self._indexes[self._default_index],
                                      method_name)
            except AttributeError:
                other_indexes = [i for i in self._indexes
                                 if i != self._default_index]
                for index in other_indexes:
                    real_method = getattr(self._indexes[index], method_name,
                                          None)
                    if real_method:
                        break
        if real_method:
            return switch_index_if_fails(real_method, self)
        else:
            raise AttributeError("No index have attribute '%s'" % method_name)
