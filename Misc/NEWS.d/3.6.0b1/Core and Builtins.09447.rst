Issue #27587: Fix another issue found by PVS-Studio: Null pointer check
after use of 'def' in _PyState_AddModule().
Initial patch by Christian Heimes.