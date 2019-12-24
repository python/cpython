__all__ = ['MIMEMultipartSigned']

from email.mime.multipart import MIMEMultipart


class MIMEMultipartSigned(MIMEMultipart):
    """Base class for MIME multipart/signed type messages."""

    def __init__(self, _subtype='signed', boundary=None, _subparts=None,
                 *, policy=None, sign_fun=None,
                 **_params):
        """Creates a multipart/signed message.

        This is very similar to the MIMEMultipart class, except that it
        accepts a callable sign_fun that can be used to sign the payload of
        the message.

        This callable will be called with the multipart container object and
        a list of flattened parts after these parts have been flattened,
        allowing for single pass signing of messages.
        """
        MIMEMultipart.__init__(self, _subtype, boundary=boundary,
                               _subparts=_subparts, policy=policy,
                               **_params)
        if sign_fun:
            self.sign_fun = sign_fun
