import os
import unittest


def skip_on_buildbot(func):
    """
    #132947 revealed that after applying some otherwise stable
    changes, only on some buildbot runners, the tests will fail with
    ResourceWarnings.
    """
    is_buildbot = "buildbot" in os.getcwd() or any(
        "buildbot" in key.lower() for key in os.environ
    )
    assert is_buildbot, f"no buildbot in {os.getcwd()} or {tuple(os.environ)}"
    skipper = unittest.skip("Causes Resource Warnings (python/cpython#132947)")
    wrapper = skipper if is_buildbot else lambda x: x
    return wrapper(func)
