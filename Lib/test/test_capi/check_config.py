# This script is used by test_misc.

import _imp
import _testinternalcapi
import json
import os
import sys


def import_singlephase():
    assert '_singlephase' not in sys.modules
    try:
        import _singlephase
    except ImportError:
        return True
    else:
        del sys.modules['_singlephase']
        return False


def check(override):
    settings_before = _testinternalcapi.get_interp_settings()
    enabled_initial = import_singlephase()

    override_initial = _imp._override_multi_interp_extensions_check(override)
    settings_after = _testinternalcapi.get_interp_settings()
    enabled_after = import_singlephase()

    override_actual = _imp._override_multi_interp_extensions_check(override_initial)
    settings_final = _testinternalcapi.get_interp_settings()
    override_noop = _imp._override_multi_interp_extensions_check(override_initial)
    settings_noop = _testinternalcapi.get_interp_settings()
    enabled_restored = import_singlephase()
    return {
        'override_requested': override,
        'settings_before': settings_before,
        'enabled_initial': enabled_initial,
        'override_initial': override_initial,
        'settings_after': settings_after,
        'enabled_after': enabled_after,
        'override_actual': override_actual,
        'settings_final': settings_final,
        'override_noop': override_noop,
        'settings_noop': settings_noop,
        'enabled_restored': enabled_restored,
    }


def run_check(override, outfd):
    with os.fdopen(outfd, 'w') as outfile:
        sys.stdout = outfile
        sys.stderr = outfile
        results = check(override)
        json.dump(results, outfile)
