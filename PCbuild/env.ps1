$pcbuild = $script:MyInvocation.MyCommand.Path | Split-Path -parent;
& cmd /K "$pcbuild\env.bat" $args
