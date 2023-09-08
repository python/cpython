Bundling ensurepip wheels
=========================

``ensurepip`` expects wheels for ``pip`` to be located in this directory.
These need to be 'bundled' by each distributor of Python,
ordinarily by running the ``Tools/build/bundle_ensurepip_wheels.py`` script.

This will download the version of ``pip`` specified by the
``ensurepip._PIP_VERSION`` variable to this directory,
and verify it against the stored SHA-256 checksum.
