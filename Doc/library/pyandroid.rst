:mod:`pyandroid` --- Python Android Development Library
=======================================================

.. module:: pyandroid
   :synopsis: Python library for Android application development

.. moduleauthor:: Subhobhai <subhobhai943@example.com>

**Source code:** `PyAndroid GitHub Repository <https://github.com/subhobhai943/pyandroid-dev>`_

--------------

The :mod:`pyandroid` module provides a comprehensive Python library for Android application
development with cross-platform capabilities. It offers a Pythonic interface for building
Android applications using familiar Python patterns and paradigms.

Overview
--------

PyAndroid enables Python developers to create Android applications without needing to learn
Java or Kotlin. The library provides:

* Core Android components (Activities, Intents, Application lifecycle)
* Complete UI framework (Views, Layouts, Widgets)
* Utility classes (Logger, FileManager, NetworkManager)
* Cross-platform compatibility
* Extensible architecture

Installation
------------

The PyAndroid library can be installed via pip::

    pip install pyandroid-dev

Basic Usage
-----------

Creating a Simple Android Application
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    from pyandroid import AndroidApp, Activity
    from pyandroid.ui import LinearLayout, Widget

    class MainActivity(Activity):
        def __init__(self):
            super().__init__("MainActivity")
            
        def on_start(self):
            layout = LinearLayout("main_layout")
            
            text_view = Widget.create_text_view(
                "hello_text", 
                "Hello from PyAndroid!"
            )
            
            button = Widget.create_button(
                "click_btn", 
                "Click Me",
                on_click=lambda v: print("Button clicked!")
            )
            
            layout.add_view(text_view)
            layout.add_view(button)
            self.add_view("main_layout", layout)

    # Create and run the application
    app = AndroidApp("HelloApp", "com.example.hello")
    app.register_activity("main", MainActivity)
    app.start_activity("main")
    app.run()

Core Classes
------------

.. class:: AndroidApp(app_name, package_name)

   Main Android Application class that manages the application lifecycle
   and global state.

   .. method:: register_activity(activity_name, activity_class)

      Register an activity with the application.

   .. method:: start_activity(activity_name, **kwargs)

      Start a specific activity by name.

   .. method:: run()

      Run the Android application.

.. class:: Activity(name)

   Base class representing a single screen in an Android application.

   .. method:: on_start()

      Called when the activity starts. Override in subclasses.

   .. method:: on_resume()

      Called when the activity resumes. Override in subclasses.

   .. method:: on_pause()

      Called when the activity pauses. Override in subclasses.

   .. method:: add_view(view_id, view)

      Add a view to this activity.

   .. method:: get_view(view_id)

      Get a view by its ID.

UI Framework
------------

The PyAndroid UI framework provides a complete set of components for building
user interfaces:

.. class:: View(view_id, width=0, height=0)

   Base class for all UI views.

   .. method:: set_position(x, y)

      Set the view's position on screen.

   .. method:: set_size(width, height)

      Set the view's dimensions.

   .. method:: set_on_click_listener(callback)

      Set a function to call when the view is clicked.

.. class:: TextView(view_id, text="", **kwargs)

   A view for displaying text.

   .. method:: set_text(text)

      Set the text content.

   .. method:: set_text_color(color)

      Set the text color using hex color codes.

.. class:: Button(view_id, text="Button", **kwargs)

   A button widget that extends TextView.

.. class:: EditText(view_id, hint="", **kwargs)

   A text input field.

   .. method:: get_text()

      Get the current input text.

Layouts
-------

.. class:: LinearLayout(layout_id, orientation="vertical")

   A layout that arranges child views in a single direction.

   .. method:: add_view(view)

      Add a child view to the layout.

   .. method:: find_view_by_id(view_id)

      Find a child view by its ID.

Utility Classes
---------------

.. class:: Logger(name, level="INFO")

   Enhanced logging utility for Android applications.

   .. method:: info(message, **kwargs)

      Log an informational message.

   .. method:: error(message, **kwargs)

      Log an error message.

.. class:: FileManager(app_name)

   File and storage management utility.

   .. method:: write_file(filename, content, subdir="")

      Write content to a file.

   .. method:: save_json(filename, data, subdir="")

      Save data as a JSON file.

   .. method:: load_json(filename, subdir="")

      Load data from a JSON file.

.. class:: NetworkManager(app_name, timeout=30)

   Network and HTTP utility for Android applications.

   .. method:: get(url, headers=None)

      Make an HTTP GET request.

   .. method:: post_json(url, data, headers=None)

      Make an HTTP POST request with JSON data.

   .. method:: is_connected(test_url="https://www.google.com")

      Check if internet connection is available.

Examples
--------

For more comprehensive examples, see the `PyAndroid Examples Directory 
<https://github.com/subhobhai943/pyandroid-dev/tree/main/examples>`_.

See Also
--------

* :mod:`tkinter` --- Python's de-facto standard GUI package
* :mod:`logging` --- Logging facility for Python
* :mod:`json` --- JSON encoder and decoder
* :mod:`urllib` --- URL handling modules
