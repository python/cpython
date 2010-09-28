@rem Used by the buildbot "test" step.
cd PCbuild
call rt.bat -d -q -uall -rw -n -v test_ttk_guionly
call rt.bat -d -q -uall -rw -n -x test_ttk_guionly

