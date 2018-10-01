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
    [Parameter(Mandatory=$true)][string]$mapfile,
    [Parameter(Mandatory=$true)][string]$msix,
    [switch]$sign,
    [string]$description,
    [string]$certname,
    [string]$certsha1
)

$tools = $script:MyInvocation.MyCommand.Path | Split-Path -parent;
Import-Module $tools\sdktools.psm1 -WarningAction SilentlyContinue -Force

Set-Alias makeappx (Find-Tool "makeappx.exe") -Scope Script

mkdir -Force ($msix | Split-Path -parent);
makeappx pack /f $mapfile /o /p $msix
if (-not $?) {
    throw "makeappx step failed"
}

if ($sign) {
    Sign-File -certname $certname -certsha1 $certsha1 -description $description -files $msix

    if (-not $?) {
        throw "Package signing failed"
    }
}
