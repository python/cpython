Issue #25604: Fix a minor bug in integer true division; this bug could
potentially have caused off-by-one-ulp results on platforms with
unreliable ldexp implementations.