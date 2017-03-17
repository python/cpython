- Issue #27053: Updates make_zip.py to correctly generate library ZIP file.

- Issue #26268: Update the prepare_ssl.py script to handle OpenSSL releases
  that don't include the contents of the include directory (that is, 1.0.2e
  and later).

- Issue #26071: bdist_wininst created binaries fail to start and find
  32bit Python

- Issue #26073: Update the list of magic numbers in launcher

- Issue #26065: Excludes venv from library when generating embeddable
  distro.

- Issue #17500, and https://github.com/python/pythondotorg/issues/945: Remove
  unused and outdated icons.

