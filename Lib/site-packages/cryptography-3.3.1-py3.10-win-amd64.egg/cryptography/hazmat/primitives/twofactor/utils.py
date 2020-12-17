# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import base64

from six.moves.urllib.parse import quote, urlencode


def _generate_uri(hotp, type_name, account_name, issuer, extra_parameters):
    parameters = [
        ("digits", hotp._length),
        ("secret", base64.b32encode(hotp._key)),
        ("algorithm", hotp._algorithm.name.upper()),
    ]

    if issuer is not None:
        parameters.append(("issuer", issuer))

    parameters.extend(extra_parameters)

    uriparts = {
        "type": type_name,
        "label": (
            "%s:%s" % (quote(issuer), quote(account_name))
            if issuer
            else quote(account_name)
        ),
        "parameters": urlencode(parameters),
    }
    return "otpauth://{type}/{label}?{parameters}".format(**uriparts)
