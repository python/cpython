Issue #25725: Fixed a reference leak in pickle.loads() when unpickling
invalid data including tuple instructions.