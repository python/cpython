import os
import unittest


def skip_on_buildbot(func):
    """
    #132947 revealed that after applying some otherwise stable
    changes, only on some buildbot runners, the tests will fail with
    ResourceWarnings.
    """
    # detect "not github actions" as a proxy for BUILDBOT not being present yet.
    is_buildbot = "GITHUB_ACTION" not in os.environ or "BUILDBOT" in os.environ
    skipper = unittest.skip("Causes Resource Warnings (python/cpython#132947)")
    wrapper = skipper if is_buildbot else lambda x: x
    return wrapper(func)
