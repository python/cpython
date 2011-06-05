:mod:`packaging.tests.pypi_server` --- PyPI mock server
=======================================================

.. module:: packaging.tests.pypi_server
   :synopsis: Mock server used to test PyPI-related modules and commands.


When you are testing code that works with Packaging, you might find these tools
useful.


The mock server
---------------

.. class:: PyPIServer

   PyPIServer is a class that implements an HTTP server running in a separate
   thread. All it does is record the requests for further inspection. The recorded
   data is available under ``requests`` attribute. The default
   HTTP response can be overridden with the ``default_response_status``,
   ``default_response_headers`` and ``default_response_data`` attributes.

   By default, when accessing the server with urls beginning with `/simple/`,
   the server also record your requests, but will look for files under
   the `/tests/pypiserver/simple/` path.

   You can tell the sever to serve static files for other paths. This could be
   accomplished by using the `static_uri_paths` parameter, as below::

      server = PyPIServer(static_uri_paths=["first_path", "second_path"])


   You need to create the content that will be served under the
   `/tests/pypiserver/default` path. If you want to serve content from another
   place, you also can specify another filesystem path (which needs to be under
   `tests/pypiserver/`. This will replace the default behavior of the server, and
   it will not serve content from the `default` dir ::

      server = PyPIServer(static_filesystem_paths=["path/to/your/dir"])


   If you just need to add some paths to the existing ones, you can do as shown,
   keeping in mind that the server will always try to load paths in reverse order
   (e.g here, try "another/super/path" then the default one) ::

      server = PyPIServer(test_static_path="another/super/path")
      server = PyPIServer("another/super/path")
      # or
      server.static_filesystem_paths.append("another/super/path")


   As a result of what, in your tests, while you need to use the PyPIServer, in
   order to isolates the test cases, the best practice is to place the common files
   in the `default` folder, and to create a directory for each specific test case::

      server = PyPIServer(static_filesystem_paths = ["default", "test_pypi_server"],
                          static_uri_paths=["simple", "external"])


Base class and decorator for tests
----------------------------------

.. class:: PyPIServerTestCase

   ``PyPIServerTestCase`` is a test case class with setUp and tearDown methods that
   take care of a single PyPIServer instance attached as a ``pypi`` attribute on
   the test class. Use it as one of the base classes in your test case::


      class UploadTestCase(PyPIServerTestCase):

          def test_something(self):
              cmd = self.prepare_command()
              cmd.ensure_finalized()
              cmd.repository = self.pypi.full_address
              cmd.run()

              environ, request_data = self.pypi.requests[-1]
              self.assertEqual(request_data, EXPECTED_REQUEST_DATA)


.. decorator:: use_pypi_server

   You also can use a decorator for your tests, if you do not need the same server
   instance along all you test case. So, you can specify, for each test method,
   some initialisation parameters for the server.

   For this, you need to add a `server` parameter to your method, like this::

      class SampleTestCase(TestCase):

          @use_pypi_server()
          def test_something(self, server):
              ...


   The decorator will instantiate the server for you, and run and stop it just
   before and after your method call. You also can pass the server initializer,
   just like this::

      class SampleTestCase(TestCase):

          @use_pypi_server("test_case_name")
          def test_something(self, server):
              ...
