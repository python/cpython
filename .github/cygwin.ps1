function Cygwin {
    $cygbin = "C:\\cygwin64\\bin"
    $project_path = & "$cygbin\\cygpath" -u -a $env:APPVEYOR_BUILD_FOLDER
    $cmd = ("C:\\cygwin64\\bin\\bash.exe -lc " +`
        "'cd $project_path; $($args -join " ")'")
    Invoke-Expression $cmd
    if ($LastExitCode -ne 0) {
        $host.SetShouldExit($LastExitCode)
    }
}
