<#
.Synopsis
    Compiles and signs an APPX package
.Description
    Given the file listing, ensures all the contents are signed
    and builds and signs the final package.
.Parameter mapfile
    The location on disk of the text mapping file.
.Parameter msix
    The path and name to store the APPX/MSIX.
.Parameter sign
    When set, signs the APPX/MSIX. Packages to be published to
    the store should not be signed.
.Parameter description
    Description to embed in the signature (optional).
.Parameter certname
    The name of the certificate to sign with (optional).
.Parameter certsha1
    The SHA1 hash of the certificate to sign with (optional).
#>
param(
    [Parameter(Mandatory=$true)][string]$layout,
    [Parameter(Mandatory=$true)][string]$msix,
    [switch]$sign,
    [string]$description,
    [string]$certname,
    [string]$certsha1,
    [string]$certfile
)

$tools = $script:MyInvocation.MyCommand.Path | Split-Path -parent;
Import-Module $tools\sdktools.psm1 -WarningAction SilentlyContinue -Force

Set-Alias makeappx (Find-Tool "makeappx.exe") -Scope Script
Set-Alias makepri (Find-Tool "makepri.exe") -Scope Script

$msixdir = Split-Path $msix -Parent
if ($msixdir) {
    $msixdir = (mkdir -Force $msixdir).FullName
} else {
    $msixdir = Get-Location
}
$msix = Join-Path $msixdir (Split-Path $msix -Leaf)

pushd $layout
try {
    if (Test-Path resources.pri) {
        del resources.pri
    }
    $name = ([xml](gc AppxManifest.xml)).Package.Identity.Name
    makepri new /pr . /mn AppxManifest.xml /in $name /cf _resources.xml /of _resources.pri /mf appx /o
    if (-not $? -or -not (Test-Path _resources.map.txt)) {
        throw "makepri step failed"
    }
    $lines = gc _resources.map.txt
    $lines | ?{ -not ($_ -match '"_resources[\w\.]+?"') } | Out-File _resources.map.txt -Encoding utf8
    makeappx pack /f _resources.map.txt /m AppxManifest.xml /o /p $msix
    if (-not $?) {
        throw "makeappx step failed"
    }
} finally {
    popd
}

if ($sign) {
    Sign-File -certname $certname -certsha1 $certsha1 -certfile $certfile -description $description -files $msix

    if (-not $?) {
        throw "Package signing failed"
    }
}
