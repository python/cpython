@rem Used by the buildbot "test" step.
@setlocal

@set here=%~dp0
@set rt_opts=-q -d

:CheckOpts
@if '%1'=='-x64' (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
@if '%1'=='-d' (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
@if '%1'=='-O' (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
@if '%1'=='-q' (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
@if '%1'=='+d' (set rt_opts=%rt_opts:-d=%) & shift & goto CheckOpts
@if '%1'=='+q' (set rt_opts=%rt_opts:-q=%) & shift & goto CheckOpts

call "%here%..\..\PCbuild\rt.bat" %rt_opts% -uall -rwW -n --timeout=3600 %1 %2 %3 %4 %5 %6 %7 %8 %9
