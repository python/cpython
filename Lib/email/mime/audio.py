# Copyright (C) 2001-2007 Python Software Foundation
# Author: Anthony Baxter
# Contact: email-sig@python.org

"""Class representing audio/* type MIME documents."""

__all__ = ['MIMEAudio']

from email import encoders
from email.mime.nonmultipart import MIMENonMultipart


class MIMEAudio(MIMENonMultipart):
    """Class for generating audio/* MIME documents."""

    def __init__(self, _audiodata, _subtype=None,
                 _encoder=encoders.encode_base64, *, policy=None, **_params):
        """Create an audio/* type MIME document.

        _audiodata contains the bytes for the raw audio data.  If this data
        can be decoded as au, wav, aiff, or aifc, then the
        subtype will be automatically included in the Content-Type header.
        Otherwise, you can specify  the specific audio subtype via the
        _subtype parameter.  If _subtype is not given, and no subtype can be
        guessed, a TypeError is raised.

        _encoder is a function which will perform the actual encoding for
        transport of the image data.  It takes one argument, which is this
        Image instance.  It should use get_payload() and set_payload() to
        change the payload to the encoded form.  It should also add any
        Content-Transfer-Encoding or other headers to the message as
        necessary.  The default encoding is Base64.

        Any additional keyword arguments are passed to the base class
        constructor, which turns them into parameters on the Content-Type
        header.
        """
        _subtype = _subtype or _infer_subtype(_audiodata)
        MIMENonMultipart.__init__(self, 'audio', _subtype, policy=policy,
                                  **_params)
        self.set_payload(_audiodata)
        _encoder(self)


def _infer_subtype(h: bytes) -> str:
    """Infer the audio format based on the data's header"""
    if   h.startswith(b'FORM') and h[8:12] in (b'AIFC', b'AIFF'):
        return 'x-aiff'
    elif h.startswith(b'.snd'):
        return 'basic'
    elif h.startswith(b'RIFF') and h[8:16] == b'WAVEfmt ':
        return 'x-wav'

    raise TypeError('Could not find audio MIME subtype')
