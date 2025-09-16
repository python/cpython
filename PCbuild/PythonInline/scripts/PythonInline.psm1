# PowerShell Module for Python Inline Runner
# Save this as PythonInline.psm1 and import it

# Function to find the MSIX installation path
function Get-PythonInlineExePath {
    $package = Get-AppxPackage -Name "PythonInlineRunner" -ErrorAction SilentlyContinue
    if ($package) {
        $exePath = Join-Path $package.InstallLocation "python_inline.exe"
        if (Test-Path $exePath) {
            return $exePath
        }
    }
    
    # Fallback: try to find in common locations
    $fallbackPaths = @(
        "C:\Program Files\WindowsApps\PythonInlineRunner*\python_inline.exe",
        "$env:LOCALAPPDATA\Microsoft\WindowsApps\python_inline.exe",
        "$env:LOCALAPPDATA\Microsoft\WindowsApps\python-inline.exe",
        "$env:LOCALAPPDATA\Microsoft\WindowsApps\pyinline.exe"
    )
    
    foreach ($pattern in $fallbackPaths) {
        $found = Get-ChildItem $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }
    
    return $null
}

# Main function to run Python inline code
function Invoke-PythonInline {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true, Position=0)]
        [string]$Code,
        
        [Parameter(ValueFromRemainingArguments=$true)]
        [string[]]$Arguments = @(),
        
        [switch]$Interactive,
        [switch]$Quiet,
        [switch]$ShowHelp
    )
    
    $exePath = Get-PythonInlineExePath
    if (-not $exePath) {
        Write-Error "Python Inline Runner not found. Please install the MSIX package first."
        return
    }
    
    $args = @()
    
    if ($ShowHelp) {
        $args += "--help"
    } else {
        if ($Quiet) { $args += "--quiet" }
        if ($Interactive) { $args += "--interactive" }
        
        $args += "-c"
        $args += $Code
        
        if ($Arguments) {
            $args += $Arguments
        }
    }
    
    & $exePath $args
}

# Function to run Python file
function Invoke-PythonFile {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true, Position=0)]
        [string]$FilePath,
        
        [Parameter(ValueFromRemainingArguments=$true)]
        [string[]]$Arguments = @(),
        
        [switch]$Interactive,
        [switch]$Quiet
    )
    
    $exePath = Get-PythonInlineExePath
    if (-not $exePath) {
        Write-Error "Python Inline Runner not found. Please install the MSIX package first."
        return
    }
    
    $args = @()
    if ($Quiet) { $args += "--quiet" }
    if ($Interactive) { $args += "--interactive" }
    
    $args += "--file=$FilePath"
    
    if ($Arguments) {
        $args += $Arguments
    }
    
    & $exePath $args
}

# Convenient aliases
function python-inline {
    param([string]$Code, [string[]]$Arguments)
    if ($Code -eq "--help" -or $Code -eq "-h") {
        Invoke-PythonInline -ShowHelp
    } elseif ($Code -like "--file=*") {
        $file = $Code.Substring(7)
        Invoke-PythonFile -FilePath $file @Arguments
    } elseif ($Code -eq "-c" -and $Arguments.Count -gt 0) {
        Invoke-PythonInline -Code $Arguments[0] -Arguments $Arguments[1..($Arguments.Count-1)]
    } else {
        Invoke-PythonInline -Code $Code -Arguments $Arguments
    }
}

function pyinline {
    python-inline @args
}

function python_inline {
    python-inline @args
}

# Export functions
Export-ModuleMember -Function Invoke-PythonInline, Invoke-PythonFile, python-inline, pyinline, python_inline

# Auto-load message
Write-Host "? Python Inline Runner module loaded!" -ForegroundColor Green
$exePath = Get-PythonInlineExePath
if ($exePath) {
    Write-Host "   Executable found at: $exePath" -ForegroundColor Gray
    Write-Host "   Usage: python-inline 'print(\"Hello World!\")'" -ForegroundColor Cyan
} else {
    Write-Host "   ??  Executable not found. Install MSIX package first." -ForegroundColor Yellow
}