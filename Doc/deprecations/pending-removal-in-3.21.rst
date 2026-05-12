Pending removal in Python 3.21
------------------------------

* :mod:`abc`

   * Soft-deprecated since Python 3.3 :class:`abc.abstractclassmethod`,
     :class:`abc.abstractstaticmethod`, and :class:`abc.abstractproperty`
     now raise a :exc:`DeprecationWarning`.
     These classes will be removed in Python 3.21, instead
     use :func:`abc.abstractmethod` with :func:`classmethod`,
     :func:`staticmethod`, and :class:`property` respectively.
