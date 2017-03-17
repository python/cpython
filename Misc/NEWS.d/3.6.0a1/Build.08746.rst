Issue #22359: Disable the rules for running _freeze_importlib and pgen when
cross-compiling.  The output of these programs is normally saved with the
source code anyway, and is still regenerated when doing a native build.
Patch by Xavier de Gaye.