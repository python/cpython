.. _using-android:

=======================
Using Python on Android
=======================

Python on Android is unlike Python on desktop platforms. On a desktop platform,
Python is generally installed as a system resource that can be used by any user
of that computer. Users then interact with Python by running a :program:`python`
executable and entering commands at an interactive prompt, or by running a
Python script.

On Android, there is no concept of installing as a system resource. The only unit
of software distribution is an "app". There is also no console where you could
run a :program:`python` executable, or interact with a Python REPL.

As a result, the only way you can use Python on Android is in embedded mode – that
is, by writing a native Android application, embedding a Python interpreter
using ``libpython``, and invoking Python code using the :ref:`Python embedding
API <embedding>`. The full Python interpreter, the standard library, and all
your Python code is then packaged into your app for its own private use.

The Python standard library has some notable omissions and restrictions on
Android. See the :ref:`API availability guide <mobile-availability>` for
details.

Adding Python to an Android app
-------------------------------

Most app developers should use one of the following tools, which will provide a
much easier experience:

* `Briefcase <https://briefcase.readthedocs.io>`__, from the BeeWare project
* `Buildozer <https://buildozer.readthedocs.io>`__, from the Kivy project
* `Chaquopy <https://chaquo.com/chaquopy>`__
* `pyqtdeploy <https://www.riverbankcomputing.com/static/Docs/pyqtdeploy/>`__
* `Termux <https://termux.dev/en/>`__

If you're sure you want to do all of this manually, read on. You can use the
:source:`testbed app <Android/testbed>` as a guide; each step below contains a
link to the relevant file.

* Build Python by following the instructions in :source:`Android/README.md`.
  This will create the directory ``cross-build/HOST/prefix``.

* Add code to your :source:`build.gradle <Android/testbed/app/build.gradle.kts>`
  file to copy the following items into your project. All except your own Python
  code can be copied from ``prefix/lib``:

  * In your JNI libraries:

    * ``libpython*.*.so``
    * ``lib*_python.so`` (external libraries such as OpenSSL)

  * In your assets:

    * ``python*.*`` (the Python standard library)
    * ``python*.*/site-packages`` (your own Python code)

* Add code to your app to :source:`extract the assets to the filesystem
  <Android/testbed/app/src/main/java/org/python/testbed/MainActivity.kt>`.

* Add code to your app to :source:`start Python in embedded mode
  <Android/testbed/app/src/main/c/main_activity.c>`. This will need to be C code
  called via JNI.
