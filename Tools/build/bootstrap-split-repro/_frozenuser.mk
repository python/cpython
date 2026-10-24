# Rule included from Modules/Setup.local: freeze the module's Python source
# with the build tree's own freeze tool.
Modules/_frozenuser_data.h: $(srcdir)/Modules/_frozenuser.py $(FREEZE_MODULE_DEPS)
	$(FREEZE_MODULE) _frozenuser $(srcdir)/Modules/_frozenuser.py $@
.DEFAULT_GOAL := all
