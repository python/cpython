# Package Installation Script for Python Inline
# This script installs external packages at build time for bundling

param(
    [Parameter(Mandatory=$false)]
    [string]$RequirementsFile = "requirements.txt",
    
    [Parameter(Mandatory=$false)]
    [string]$SitePackagesPath = ".\PythonInline\packaging\site-packages",
    
    [Parameter(Mandatory=$false)]
    [string]$PythonExecutable = "",
    
    [Parameter(Mandatory=$false)]
    [switch]$Force = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$VerboseOutput = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$NoDeps = $false
)

$ErrorActionPreference = "Continue"

Write-Host "?? Python Inline Package Bundler" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host "Requirements File: $RequirementsFile" -ForegroundColor Yellow
Write-Host "Target Directory: $SitePackagesPath" -ForegroundColor Yellow
Write-Host "Bundle Mode: Build-time package bundling" -ForegroundColor Cyan
Write-Host ""

function Write-SuccessMessage {
    param([string]$Message)
    Write-Host "? $Message" -ForegroundColor Green
}

function Write-ErrorMessage {
    param([string]$Message, [string]$Solution = "")
    Write-Host "? ERROR: $Message" -ForegroundColor Red
    if ($Solution) {
        Write-Host "?? Solution: $Solution" -ForegroundColor Yellow
    }
}

function Write-WarningMessage {
    param([string]$Message)
    Write-Host "??  WARNING: $Message" -ForegroundColor Yellow
}

function Write-InfoMessage {
    param([string]$Message)
    Write-Host "??  $Message" -ForegroundColor Cyan
}

# Find Python executable if not specified
if ([string]::IsNullOrEmpty($PythonExecutable)) {
    Write-InfoMessage "Looking for Python executable..."
    
    $PossiblePythonPaths = @(
        "python.exe",
        "python3.exe",
        "py.exe",
        "${env:LOCALAPPDATA}\Programs\Python\Python312\python.exe",
        "${env:LOCALAPPDATA}\Programs\Python\Python311\python.exe",
        "${env:LOCALAPPDATA}\Programs\Python\Python310\python.exe",
        "${env:LOCALAPPDATA}\Programs\Python\Python39\python.exe",
        "C:\Python312\python.exe",
        "C:\Python311\python.exe",
        "C:\Python310\python.exe",
        "C:\Python39\python.exe"
    )
    
    foreach ($PyPath in $PossiblePythonPaths) {
        try {
            $null = & $PyPath --version 2>$null
            if ($LASTEXITCODE -eq 0) {
                $PythonExecutable = $PyPath
                Write-SuccessMessage "Found Python: $PythonExecutable"
                break
            }
        } catch {
            # Continue searching
        }
    }
    
    if ([string]::IsNullOrEmpty($PythonExecutable)) {
        Write-ErrorMessage "Python executable not found" "Please install Python or specify the path with -PythonExecutable"
        exit 1
    }
}

# Verify Python and pip
Write-InfoMessage "Verifying Python installation..."
try {
    $PythonVersion = & $PythonExecutable --version 2>&1
    Write-SuccessMessage "Python version: $PythonVersion"
} catch {
    Write-ErrorMessage "Failed to run Python executable: $PythonExecutable"
    exit 1
}

# Check if pip is available
Write-InfoMessage "Checking pip availability..."
try {
    $null = & $PythonExecutable -m pip --version 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-SuccessMessage "pip is available"
    } else {
        Write-ErrorMessage "pip is not available" "Install pip with: $PythonExecutable -m ensurepip --upgrade"
        exit 1
    }
} catch {
    Write-ErrorMessage "Failed to check pip availability"
    exit 1
}

# Check if requirements file exists
if (-not (Test-Path $RequirementsFile)) {
    Write-WarningMessage "Requirements file not found: $RequirementsFile"
    Write-InfoMessage "This is normal if no external packages are needed."
    Write-InfoMessage "To add packages, create $RequirementsFile with package names (one per line)"
    
    # Create basic requirements.txt with requests as example
    $ExampleRequirements = @"
# Python Inline External Packages Configuration
# Uncomment packages you want to bundle with your application
# 
# Popular packages for bundling:

# HTTP requests
requests==2.31.0

# Data analysis
# numpy>=1.24.0
# pandas>=2.0.0

# Web scraping
# beautifulsoup4>=4.12.0

# Add your packages below:
# your_package_name>=1.0.0
"@
    
    try {
        $ExampleRequirements | Out-File $RequirementsFile -Encoding UTF8
        Write-SuccessMessage "Created example requirements.txt file"
        Write-InfoMessage "Edit $RequirementsFile to enable packages, then rebuild"
        exit 0
    } catch {
        Write-ErrorMessage "Failed to create requirements.txt file"
        exit 1
    }
}

# Read and validate requirements
Write-InfoMessage "Reading requirements file..."
$Requirements = Get-Content $RequirementsFile | Where-Object { 
    $_.Trim() -ne "" -and -not $_.Trim().StartsWith("#") 
}

if ($Requirements.Count -eq 0) {
    Write-InfoMessage "No packages specified in requirements file (all lines are comments or empty)"
    Write-InfoMessage "This is normal if no external packages are needed."
    Write-InfoMessage "To bundle packages, uncomment package names in $RequirementsFile"
    exit 0
}

Write-SuccessMessage "Found $($Requirements.Count) packages to bundle:"
foreach ($Package in $Requirements) {
    Write-Host "  ?? $Package" -ForegroundColor White
}

# Create target directory
Write-InfoMessage "Preparing site-packages directory for bundling..."
if (Test-Path $SitePackagesPath) {
    if ($Force) {
        Write-InfoMessage "Removing existing site-packages directory..."
        Remove-Item $SitePackagesPath -Recurse -Force
    } else {
        Write-WarningMessage "Site-packages directory already exists"
        Write-InfoMessage "Use -Force to overwrite existing packages"
    }
}

try {
    New-Item -ItemType Directory -Path $SitePackagesPath -Force | Out-Null
    Write-SuccessMessage "Created site-packages directory: $SitePackagesPath"
} catch {
    Write-ErrorMessage "Failed to create site-packages directory: $($_.Exception.Message)"
    exit 1
}

# Install packages for bundling
Write-InfoMessage "Installing packages for bundling..."
$InstallArgs = @(
    "-m", "pip", "install",
    "--target", $SitePackagesPath,
    "--upgrade"
)

# Add --no-deps flag if specified (for minimal bundling)
if ($NoDeps) {
    $InstallArgs += "--no-deps"
    Write-InfoMessage "Installing without dependencies (minimal bundling)"
} else {
    Write-InfoMessage "Installing with dependencies (full bundling)"
}

if (-not $VerboseOutput) {
    $InstallArgs += "--quiet"
}

if ($VerboseOutput) {
    $InstallArgs += "--verbose"
}

$SuccessCount = 0
$FailedPackages = @()

foreach ($Package in $Requirements) {
    Write-Host "?? Bundling $Package..." -ForegroundColor Cyan
    
    try {
        $InstallCommand = @($PythonExecutable) + $InstallArgs + @($Package)
        & $InstallCommand[0] $InstallCommand[1..($InstallCommand.Length-1)]
        
        if ($LASTEXITCODE -eq 0) {
            Write-SuccessMessage "Bundled: $Package"
            $SuccessCount++
        } else {
            Write-ErrorMessage "Failed to bundle: $Package"
            $FailedPackages += $Package
        }
    } catch {
        Write-ErrorMessage "Exception bundling $Package`: $($_.Exception.Message)"
        $FailedPackages += $Package
    }
}

# Create packages manifest for tracking
Write-InfoMessage "Creating packages manifest..."
$ManifestPath = Join-Path $SitePackagesPath "python_inline_packages.txt"
$ManifestContent = @"
# Python Inline Bundled Packages Manifest
# Generated on: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
# Python version: $PythonVersion
# Total packages: $SuccessCount
# Bundle mode: Build-time bundling
#
# These packages are bundled with the application and available
# to all Python scripts run through Python Inline.

"@

foreach ($Package in ($Requirements | Where-Object { $_ -notin $FailedPackages })) {
    $ManifestContent += "$Package`n"
}

if ($FailedPackages.Count -gt 0) {
    $ManifestContent += "`n# Failed packages:`n"
    foreach ($Package in $FailedPackages) {
        $ManifestContent += "# FAILED: $Package`n"
    }
}

try {
    $ManifestContent | Out-File $ManifestPath -Encoding UTF8
    Write-SuccessMessage "Created packages manifest: $ManifestPath"
} catch {
    Write-WarningMessage "Failed to create packages manifest"
}

# Clean up unnecessary files to reduce bundle size
Write-InfoMessage "Optimizing bundle size..."
$FilesToRemove = @(
    "*.pyc",
    "__pycache__",
    "*.pyo",
    "*.dist-info\WHEEL",
    "*.dist-info\top_level.txt",
    "tests",
    "test",
    "*.dist-info\direct_url.json"
)

foreach ($Pattern in $FilesToRemove) {
    try {
        $ItemsToRemove = Get-ChildItem -Path $SitePackagesPath -Recurse -Include $Pattern -Force -ErrorAction SilentlyContinue
        foreach ($Item in $ItemsToRemove) {
            Remove-Item $Item -Recurse -Force -ErrorAction SilentlyContinue
        }
    } catch {
        # Ignore cleanup errors
    }
}

# Calculate installed size
$TotalSize = 0
if (Test-Path $SitePackagesPath) {
    $TotalSize = (Get-ChildItem $SitePackagesPath -Recurse -File | Measure-Object -Property Length -Sum).Sum
}

# Summary
Write-Host ""
Write-Host "?? Package Bundling Summary" -ForegroundColor Green
Write-Host "===========================" -ForegroundColor Green
Write-Host "? Successfully bundled: $SuccessCount packages" -ForegroundColor Green
if ($FailedPackages.Count -gt 0) {
    Write-Host "? Failed packages: $($FailedPackages.Count)" -ForegroundColor Red
    foreach ($Package in $FailedPackages) {
        Write-Host "   - $Package" -ForegroundColor Red
    }
}
Write-Host "?? Bundle location: $SitePackagesPath" -ForegroundColor White
Write-Host "?? Bundle size: $([math]::Round($TotalSize / 1MB, 2)) MB" -ForegroundColor White
Write-Host ""

if ($SuccessCount -gt 0) {
    Write-SuccessMessage "Packages are bundled and ready for use with Python Inline!"
    Write-InfoMessage "These packages will be automatically available when running Python Inline scripts"
    Write-InfoMessage "Example: python_inline.exe -c `"import requests; print('requests available!')`""
} else {
    Write-WarningMessage "No packages were successfully bundled"
    Write-InfoMessage "This means no external packages will be available in Python Inline scripts"
    if ($FailedPackages.Count -gt 0) {
        exit 1
    }
}