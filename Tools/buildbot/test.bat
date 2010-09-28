@rem Used by the buildbot "test" step.
cd PCbuild
python_d -m test.test_ttk_guionly
call rt.bat -d -q -uall -rw -x test.test_ttk_guionly
