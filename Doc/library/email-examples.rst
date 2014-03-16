.. _email-examples:

:mod:`email`: Examples
----------------------

Here are a few examples of how to use the :mod:`email` package to read, write,
and send simple email messages, as well as more complex MIME messages.

First, let's see how to create and send a simple text message:

.. literalinclude:: ../includes/email-simple.py


And parsing RFC822 headers can easily be done by the parse(filename) or
parsestr(message_as_string) methods of the Parser() class:

.. literalinclude:: ../includes/email-headers.py


Here's an example of how to send a MIME message containing a bunch of family
pictures that may be residing in a directory:

.. literalinclude:: ../includes/email-mime.py


Here's an example of how to send the entire contents of a directory as an email
message: [1]_

.. literalinclude:: ../includes/email-dir.py


Here's an example of how to unpack a MIME message like the one
above, into a directory of files:

.. literalinclude:: ../includes/email-unpack.py

Here's an example of how to create an HTML message with an alternative plain
text version: [2]_

.. literalinclude:: ../includes/email-alternative.py


.. _email-contentmanager-api-examples:

Examples using the Provisional API
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Here is a reworking of the last example using the provisional API.  To make
things a bit more interesting, we include a related image in the html part, and
we save a copy of what we are going to send to disk, as well as sending it.

This example also shows how easy it is to include non-ASCII, and simplifies the
sending of the message using the :meth:`.send_message` method of the
:mod:`smtplib` module.

.. literalinclude:: ../includes/email-alternative-new-api.py

If we were instead sent the message from the last example, here is one
way we could process it:

.. literalinclude:: ../includes/email-read-alternative-new-api.py

Up to the prompt, the output from the above is::

    To: Penelope Pussycat <"penelope@example.com">, Fabrette Pussycat <"fabrette@example.com">
    From: Pepé Le Pew <pepe@example.com>
    Subject: Ayons asperges pour le déjeuner

    Salut!

    Cela ressemble à un excellent recipie[1] déjeuner.


.. rubric:: Footnotes

.. [1] Thanks to Matthew Dixon Cowles for the original inspiration and examples.
.. [2] Contributed by Martin Matejek.
