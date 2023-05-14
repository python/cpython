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
        suppress_health_check=[hypothesis.HealthCheck.too_slow],
    )
    hypothesis.settings.load_profile("slow-is-ok")
