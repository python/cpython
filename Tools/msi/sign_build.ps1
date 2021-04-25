<#
.Synopsis
    Recursively signs the contents of a directory.
.Description
    Given the file patterns, code signs the contents.
.Parameter root
    The root directory to sign.
.Parameter patterns
    The file patterns to sign
.Parameter description
    The description to add to the signature (optional).
.Parameter certname
    The name of the certificate to sign with (optional).
.Parameter certsha1
    The SHA1 hash of the certificate to sign with (optional).
#>
param(
    [Parameter(Mandatory=$true)][string]$root,
    [string[]]$patterns=@("*.exe", "*.dll", "*.pyd", "*.cat"),
    [string]$description,
    [string]$certname,
    [string]$certsha1,
    [string]$certfile
)

$tools = $script:MyInvocation.MyCommand.Path | Split-Path -parent;
Import-Module $tools\sdktools.psm1 -WarningAction SilentlyContinue -Force

pushd $root
try {
    Sign-File -certname $certname -certsha1 $certsha1 -certfile $certfile -description $description -files (gci -r $patterns)
} finally {
    popd
}