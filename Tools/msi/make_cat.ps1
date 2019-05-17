<#
.Synopsis
    Compiles and signs a catalog file.
.Description
    Given the CDF definition file, builds and signs a catalog.
.Parameter catalog
    The path to the catalog definition file to compile and
    sign. It is assumed that the .cat file will be the same
    name with a new extension.
.Parameter description
    The description to add to the signature (optional).
.Parameter certname
    The name of the certificate to sign with (optional).
.Parameter certsha1
    The SHA1 hash of the certificate to sign with (optional).
#>
param(
    [Parameter(Mandatory=$true)][string]$catalog,
    [switch]$sign,
    [string]$description,
    [string]$certname,
    [string]$certsha1,
    [string]$certfile
)

$tools = $script:MyInvocation.MyCommand.Path | Split-Path -parent;
Import-Module $tools\sdktools.psm1 -WarningAction SilentlyContinue -Force

Set-Alias MakeCat (Find-Tool "makecat.exe") -Scope Script

MakeCat $catalog
if (-not $?) {
    throw "Catalog compilation failed"
}
if ($sign) {
    Sign-File -certname $certname -certsha1 $certsha1 -certfile $certfile -description $description -files @($catalog -replace 'cdf$', 'cat')
}
