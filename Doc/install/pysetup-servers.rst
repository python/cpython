.. _packaging-pysetup-servers:

===============
Package Servers
===============

Pysetup supports installing Python packages from *Package Servers* in addition
to PyPI indexes and mirrors.

Package Servers are simple directory listings of Python distributions. Directories
can be served via HTTP or a local file system. This is useful when you want to
dump source distributions in a directory and not worry about the full index structure.

Serving distributions from Apache
---------------------------------
::

   $ mkdir -p /var/www/html/python/distributions
   $ cp *.tar.gz /var/www/html/python/distributions/

   <VirtualHost python.example.org:80>
       ServerAdmin webmaster@domain.com
       DocumentRoot "/var/www/html/python"
       ServerName python.example.org
       ErrorLog logs/python.example.org-error.log
       CustomLog logs/python.example.org-access.log common
       Options Indexes FollowSymLinks MultiViews
       DirectoryIndex index.html index.htm

       <Directory "/var/www/html/python/distributions">
           Options Indexes FollowSymLinks MultiViews
           Order allow,deny
           Allow from all
       </Directory>
   </VirtualHost>

Add the Apache based distribution server to :file:`.pypirc`::

   [packaging]
   package-servers =
       apache

   [apache]
   repository: http://python.example.org/distributions/


Serving distributions from a file system
----------------------------------------
::

   $ mkdir -p /data/python/distributions
   $ cp *.tar.gz /data/python/distributions/

Add the directory to :file:`.pypirc`::

   [packaging]
   package-servers =
       local

   [local]
   repository: file:///data/python/distributions/
