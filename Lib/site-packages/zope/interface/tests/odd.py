##############################################################################
#
# Copyright (c) 2003 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
"""Odd meta class that doesn't subclass type.

This is used for testing support for ExtensionClass in new interfaces.

  >>> class A(object):
  ...     __metaclass__ = MetaClass
  ...     a = 1
  ...
  >>> A.__name__
  'A'
  >>> A.__bases__ == (object,)
  True
  >>> class B(object):
  ...     __metaclass__ = MetaClass
  ...     b = 1
  ...
  >>> class C(A, B): pass
  ...
  >>> C.__name__
  'C'
  >>> int(C.__bases__ == (A, B))
  1
  >>> a = A()
  >>> aa = A()
  >>> a.a
  1
  >>> aa.a
  1
  >>> aa.a = 2
  >>> a.a
  1
  >>> aa.a
  2
  >>> c = C()
  >>> c.a
  1
  >>> c.b
  1
  >>> c.b = 2
  >>> c.b
  2
  >>> C.c = 1
  >>> c.c
  1
  >>> import sys
  >>> if sys.version[0] == '2': # This test only makes sense under Python 2.x
  ...     from types import ClassType
  ...     assert not isinstance(C, (type, ClassType))

  >>> int(C.__class__.__class__ is C.__class__)
  1
"""

# class OddClass is an odd meta class

class MetaMetaClass(type):

    def __getattribute__(cls, name):
        if name == '__class__':
            return cls
        # Under Python 3.6, __prepare__ gets requested
        return type.__getattribute__(cls, name)


class MetaClass(object):
    """Odd classes
    """

    def __init__(self, name, bases, dict):
        self.__name__ = name
        self.__bases__ = bases
        self.__dict__.update(dict)

    def __call__(self):
        return OddInstance(self)

    def __getattr__(self, name):
        for b in self.__bases__:
            v = getattr(b, name, self)
            if v is not self:
                return v
        raise AttributeError(name)

    def __repr__(self): # pragma: no cover
        return "<odd class %s at %s>" % (self.__name__, hex(id(self)))


MetaClass = MetaMetaClass('MetaClass',
                          MetaClass.__bases__,
                          {k: v for k, v in MetaClass.__dict__.items()
                          if k not in ('__dict__',)})

class OddInstance(object):

    def __init__(self, cls):
        self.__dict__['__class__'] = cls

    def __getattribute__(self, name):
        dict = object.__getattribute__(self, '__dict__')
        if name == '__dict__':
            return dict
        v = dict.get(name, self)
        if v is not self:
            return v
        return getattr(dict['__class__'], name)

    def __setattr__(self, name, v):
        self.__dict__[name] = v

    def __delattr__(self, name):
        raise NotImplementedError()

    def __repr__(self): # pragma: no cover
        return "<odd %s instance at %s>" % (
            self.__class__.__name__, hex(id(self)))
