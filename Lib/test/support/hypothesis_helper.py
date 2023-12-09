import os

try:
    import hypothesis
except ImportError:
    from . import _hypothesis_stubs as hypothesis
else:
    # When using the real Hypothesis, we'll configure it to ignore occasional
    # slow tests (avoiding flakiness from random VM slowness in CI).
    hypothesis.settings.register_profile(
        "slow-is-ok",
        deadline=None,
        suppress_health_check=[
            hypothesis.HealthCheck.too_slow,
            hypothesis.HealthCheck.differing_executors,
        ],
    )
    hypothesis.settings.load_profile("slow-is-ok")

    # For local development, we'll write to the default on-local-disk database
    # of failing examples, and also use a pull-through cache to automatically
    # replay any failing examples discovered in CI.  For details on how this
    # works, see https://hypothesis.readthedocs.io/en/latest/database.html
    if "CI" not in os.environ:
        from hypothesis.database import (
            GitHubArtifactDatabase,
            MultiplexedDatabase,
            ReadOnlyDatabase,
        )

        hypothesis.settings.register_profile(
            "cpython-local-dev",
            database=MultiplexedDatabase(
                hypothesis.settings.default.database,
                ReadOnlyDatabase(GitHubArtifactDatabase("python", "cpython")),
            ),
        )
        hypothesis.settings.load_profile("cpython-local-dev")
