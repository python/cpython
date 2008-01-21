:mod:`bundlebuilder` --- Assemble MacOS X (application) bundles
===============================================================

.. module:: bundlebuilder
   :synopsis: Tools to assemble MacOS X (application) bundles.
   :platform: Mac
.. moduleauthor:: Just van Rossum

.. index::
   pair: creating; application bundles

This module contains two classes to build so called "bundles" for MacOS X.
:class:`BundleBuilder` is a general tool, :class:`AppBuilder` is a subclass
specialized in building application bundles.

These Builder objects are instantiated with a bunch of keyword arguments, and
have a :meth:`build` method that will do all the work.

The module also contains a main program that can be used in two ways::

   % python bundlebuilder.py [options] build
   % python buildapp.py [options] build

where :file:`buildapp.py` is a user-supplied setup.py-like script following this
model::

   from bundlebuilder import buildapp
   buildapp(<lots-of-keyword-args>)
