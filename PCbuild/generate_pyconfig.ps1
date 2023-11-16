#
# Generates pyconfig.h from PC\pyconfig.h.in
#

param (
    [string[]]$define,
    [string]$File
)

$definedValues = @{}

foreach ($arg in $define) {
    $parts = $arg -split '='

    if ($parts.Count -eq 1) {
        $key = $parts[0]
        $definedValues[$key] = ""
    } elseif ($parts.Count -eq 2) {
        $key = $parts[0]
        $value = $parts[1]
        $definedValues[$key] = $value
    } else {
        Write-Host "Invalid argument: $arg"
        exit 1
    }
}

$cpythonRoot = Split-Path $PSScriptRoot -Parent
$pyconfigPath = Join-Path $cpythonRoot "PC\pyconfig.h.in"

$header = "/* pyconfig.h.  Generated from PC\pyconfig.h.in by generate_pyconfig.ps1.  */"

$lines = Get-Content -Path $pyconfigPath
$lines = @($header) + $lines

foreach ($i in 0..($lines.Length - 1)) {
    if ($lines[$i] -match "^#undef (\w+)$") {
        $key = $Matches[1]
        if ($definedValues.ContainsKey($key)) {
            $value = $definedValues[$key]
            $lines[$i] = "#define $key $value".TrimEnd()
        } else {
            $lines[$i] = "/* #undef $key */"
        }
    }
}

$ParentDir = Split-Path -Path $File -Parent
New-Item -ItemType Directory -Force -Path $ParentDir | Out-Null
Set-Content -Path $File -Value $lines
