"""Record of phased-in incompatible language changes.

Each line is of the form:

    FeatureName = "_Feature(" OptionalRelease "," MandatoryRelease ")"

where, normally, OptionalRelease < MandatoryRelease, and both are 5-tuples
of the same form as sys.version_info:

    (PY_MAJOR_VERSION, # the 2 in 2.1.0a3; an int
     PY_MINOR_VERSION, # the 1; an int
     PY_MICRO_VERSION, # the 0; an int
     PY_RELEASE_LEVEL, # "alpha", "beta", "candidate" or "final"; string
     PY_RELEASE_SERIAL # the 3; an int
    )

OptionalRelease records the first release in which

    from __future__ import FeatureName

was accepted.

In the case of MandatoryReleases that have not yet occurred,
MandatoryRelease predicts the release in which the feature will become part
of the language.

Else MandatoryRelease records when the feature became part of the language;
in releases at or after that, modules no longer need

    from __future__ import FeatureName

to use the feature in question, but may continue to use such imports.

MandatoryRelease may also be None, meaning that a planned feature got
dropped.

Instances of class _Feature have two corresponding methods,
.getOptionalRelease() and .getMandatoryRelease().

No feature line is ever to be deleted from this file.
"""

class _Feature:
    def __init__(self, optionalRelease, mandatoryRelease):
        self.optional = optionalRelease
        self.mandatory = mandatoryRelease

    def getOptionalRelease(self):
        """Return first release in which this feature was recognized.

        This is a 5-tuple, of the same form as sys.version_info.
        """

        return self.optional

    def getMandatoryRelease(self):
        """Return release in which this feature will become mandatory.

        This is a 5-tuple, of the same form as sys.version_info, or, if
        the feature was dropped, is None.
        """

        return self.mandatory

    def __repr__(self):
        return "Feature(" + `self.getOptionalRelease()` + ", " + \
                            `self.getMandatoryRelease()` + ")"

nested_scopes = _Feature((2, 1, 0, "beta", 1), (2, 2, 0, "final", 0))
