.. _email-examples:

:mod:`email`: Examples
----------------------

Here are a few examples of how to use the :mod:`email` package to read, write,
and send simple email messages, as well as more complex MIME messages.

First, let's see how to create and send a simple text message (both the
text content and the addresses may contain unicode characters):

.. literalinclude:: ../includes/email-simple.py


Parsing RFC822 headers can easily be done by the using the classes
from the :mod:`~email.parser` module:

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
text version.  To make things a bit more interesting, we include a related
image in the html part, and we save a copy of what we are going to send to
disk, as well as sending it.

.. literalinclude:: ../includes/email-alternative.py


If we were sent the message from the last example, here is one way we could
process it:

.. literalinclude:: ../includes/email-read-alternative.py

Up to the prompt, the output from the above is:

.. code-block:: none

    To: Penelope Pussycat <penelope@example.com>, Fabrette Pussycat <fabrette@example.com>
    From: Pepé Le Pew <pepe@example.com>
    Subject: Ayons asperges pour le déjeuner

    Salut!

    Cela ressemble à un excellent recipie[1] déjeuner.


.. rubric:: Footnotes

.. [1] Thanks to Matthew Dixon Cowles for the original inspiration and examples.
