<#
.Synopsis
    Uploads from a VSTS release build layout to python.org
.Description
    Given the downloaded/extracted build artifact from a release
    build run on python.visualstudio.com, this script uploads
    the files to the correct locations.
.Parameter build
    The location on disk of the extracted build artifact.
.Parameter user
    The username to use when logging into the host.
.Parameter server
    The host or PuTTY session name.
.Parameter target
    The subdirectory on the host to copy files to.
.Parameter tests
    The path to run download tests in.
.Parameter skipupload
    Skip uploading
.Parameter skippurge
    Skip purging the CDN
.Parameter skiptest
    Skip the download tests
.Parameter skiphash
    Skip displaying hashes
#>
param(
    [Parameter(Mandatory=$true)][string]$build,
    [Parameter(Mandatory=$true)][string]$user,
    [string]$server="python-downloads",
    [string]$target="/srv/www.python.org/ftp/python",
    [string]$tests=${env:TEMP},
    [switch]$skipupload,
    [switch]$skippurge,
    [switch]$skiptest,
    [switch]$skiphash
)

if (-not $build) { throw "-build option is required" }
if (-not $user) { throw "-user option is required" }

if (-not ((Test-Path "$build\win32\python-*.exe") -or (Test-Path "$build\amd64\python-*.exe"))) {
    throw "-build argument does not look like a 'build' directory"
}

function find-putty-tool {
    param ([string]$n)
    $t = gcm $n -EA 0
    if (-not $t) { $t = gcm ".\$n" -EA 0 }
    if (-not $t) { $t = gcm "${env:ProgramFiles}\PuTTY\$n" -EA 0 }
    if (-not $t) { $t = gcm "${env:ProgramFiles(x86)}\PuTTY\$n" -EA 0 }
    if (-not $t) { throw "Unable to locate $n.exe. Please put it on $PATH" }
    return gi $t.Path
}

$p = gci -r "$build\python-*.exe" | `
    ?{ $_.Name -match '^python-(\d+\.\d+\.\d+)((a|b|rc)\d+)?-.+' } | `
    select -first 1 | `
    %{ $Matches[1], $Matches[2] }

"Uploading version $($p[0]) $($p[1])"
"  from: $build"
"    to: $($server):$target/$($p[0])"
""

if (-not $skipupload) {
    # Upload files to the server
    $pscp = find-putty-tool "pscp"
    $plink = find-putty-tool "plink"

    "Upload using $pscp and $plink"
    ""

    pushd $build
    $doc = gci python*.chm, python*.chm.asc
    popd

    $d = "$target/$($p[0])/"
    & $plink -batch $user@$server mkdir $d
    & $plink -batch $user@$server chgrp downloads $d
    & $plink -batch $user@$server chmod g-x,o+rx $d
    & $pscp -batch $doc.FullName "$user@${server}:$d"

    foreach ($a in gci "$build" -Directory) {
        "Uploading files from $($a.FullName)"
        pushd "$($a.FullName)"
        $exe = gci *.exe, *.exe.asc, *.zip, *.zip.asc
        $msi = gci *.msi, *.msi.asc, *.msu, *.msu.asc
        popd

        & $pscp -batch $exe.FullName "$user@${server}:$d"

        $sd = "$d$($a.Name)$($p[1])/"
        & $plink -batch $user@$server mkdir $sd
        & $plink -batch $user@$server chgrp downloads $sd
        & $plink -batch $user@$server chmod g-x,o+rx $sd
        & $pscp -batch $msi.FullName "$user@${server}:$sd"
        & $plink -batch $user@$server chgrp downloads $sd*
        & $plink -batch $user@$server chmod g-x,o+r $sd*
    }

    & $plink -batch $user@$server chgrp downloads $d*
    & $plink -batch $user@$server chmod g-x,o+r $d*
}

if (-not $skippurge) {
    # Run a CDN purge
    py purge.py "$($p[0])$($p[1])"
}

if (-not $skiptest) {
    # Use each web installer to produce a layout. This will download
    # each referenced file and validate their signatures/hashes.
    gci "$build\*-webinstall.exe" -r -File | %{
        $d = mkdir "$tests\$($_.BaseName)" -Force
        gci $d -r -File | del
        $ic = copy $_ $d -PassThru
        "Checking layout for $($ic.Name)"
        Start-Process -wait $ic "/passive", "/layout", "$d\layout", "/log", "$d\log\install.log"
        if (-not $?) {
            Write-Error "Failed to validate layout of $($inst.Name)"
        }
    }
}

if (-not $skiphash) {
    # Display MD5 hash and size of each downloadable file
    pushd $build
    gci python*.chm, *\*.exe, *\*.zip | `
        Sort-Object Name | `
        Format-Table Name, @{Label="MD5"; Expression={(Get-FileHash $_ -Algorithm MD5).Hash}}, Length
    popd
}
