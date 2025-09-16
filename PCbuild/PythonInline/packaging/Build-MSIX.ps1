# PowerShell script to build and package Python Inline as MSIX with external packages

param(
    [Parameter(Mandatory=$false)]
    [string]$Configuration = "Release",
    
    [Parameter(Mandatory=$false)]
    [string]$Platform = "x64",
    
    [Parameter(Mandatory=$false)]
    [string]$OutputPath = ".\PythonInline\output\msix",
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipBuild = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipPackages = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$InstallAfterBuild = $false,
    
    [Parameter(Mandatory=$false)]
    [string]$RequirementsFile = ".\PythonInline\packaging\requirements.txt"
)

# Set error action preference for graceful handling
$ErrorActionPreference = "Continue"

# Define paths - Updated for correct structure
$RootPath = Get-Location
$PythonInlinePath = Join-Path $RootPath "PythonInline"
# Fix: Use the main project file in the root, not the one in PythonInline\build
$ProjectPath = Join-Path $RootPath "python_inline.vcxproj"
$MSIXManifestPath = Join-Path $PythonInlinePath "packaging\msix\Package.appxmanifest"
$PackageInstallScript = Join-Path $PythonInlinePath "packaging\Install-Packages.ps1"
# Fix: Use correct build output directory mapping
$BuildPath = if ($Platform -eq "x64") { Join-Path $RootPath "amd64" } else { Join-Path $RootPath $Platform }
$MSIXOutputPath = $OutputPath

Write-Host "?? Python Inline MSIX Builder (with Package Support)" -ForegroundColor Green
Write-Host "=====================================================" -ForegroundColor Green
Write-Host "Configuration: $Configuration" -ForegroundColor Yellow
Write-Host "Platform: $Platform" -ForegroundColor Yellow
Write-Host "Project Path: $ProjectPath" -ForegroundColor Yellow
Write-Host "Build Path: $BuildPath" -ForegroundColor Yellow
Write-Host "Output Path: $MSIXOutputPath" -ForegroundColor Yellow
Write-Host "Requirements File: $RequirementsFile" -ForegroundColor Yellow
Write-Host ""

# Function to write error messages gracefully
function Write-ErrorMessage {
    param([string]$Message, [string]$Solution = "")
    Write-Host "? ERROR: $Message" -ForegroundColor Red
    if ($Solution) {
        Write-Host "?? Solution: $Solution" -ForegroundColor Yellow
    }
}

function Write-SuccessMessage {
    param([string]$Message)
    Write-Host "? $Message" -ForegroundColor Green
}

function Write-WarningMessage {
    param([string]$Message)
    Write-Host "??  WARNING: $Message" -ForegroundColor Yellow
}

# Check if new structure exists
if (-not (Test-Path $PythonInlinePath)) {
    Write-ErrorMessage "PythonInline directory not found" "Make sure the project has been reorganized into the new structure"
    exit 1
}

if (-not (Test-Path $ProjectPath)) {
    Write-ErrorMessage "Project file not found: $ProjectPath" "Make sure the project structure is properly set up"
    exit 1
}

# Install external packages if not skipped
if (-not $SkipPackages) {
    Write-Host "?? Installing external packages..." -ForegroundColor Cyan
    
    if (Test-Path $RequirementsFile) {
        Write-Host "Found requirements file: $RequirementsFile" -ForegroundColor Gray
        
        try {
            $SitePackagesPath = Join-Path $PythonInlinePath "packaging\site-packages"
            & $PackageInstallScript -RequirementsFile $RequirementsFile -SitePackagesPath $SitePackagesPath -Force
            
            if ($LASTEXITCODE -eq 0) {
                Write-SuccessMessage "External packages installed successfully"
            } else {
                Write-WarningMessage "Some packages may not have been installed correctly"
                Write-Host "Continuing with MSIX build..." -ForegroundColor Yellow
            }
        } catch {
            Write-WarningMessage "Failed to install packages: $($_.Exception.Message)"
            Write-Host "Continuing with MSIX build..." -ForegroundColor Yellow
        }
    } else {
        Write-WarningMessage "Requirements file not found: $RequirementsFile"
        Write-Host "Creating example requirements.txt..." -ForegroundColor Gray
        
        # Create the requirements file using the package installer
        try {
            & $PackageInstallScript -RequirementsFile $RequirementsFile -SitePackagesPath "temp" > $null
            Write-Host "? Created example requirements.txt" -ForegroundColor Green
            Write-Host "Edit $RequirementsFile to add packages, then run build again" -ForegroundColor Cyan
        } catch {
            Write-WarningMessage "Could not create requirements.txt file"
        }
    }
} else {
    Write-Host "??  Skipping package installation as requested" -ForegroundColor Yellow
}

# Check if Windows SDK is available with more versions
Write-Host "?? Checking for Windows SDK..." -ForegroundColor Cyan
$WindowsSdkPath = ""
$PossibleSdkPaths = @(
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.26100.0\x64",
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22621.0\x64",
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22000.0\x64",
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.19041.0\x64", 
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.18362.0\x64"
)

foreach ($SdkPath in $PossibleSdkPaths) {
    if (Test-Path (Join-Path $SdkPath "makeappx.exe")) {
        $WindowsSdkPath = $SdkPath
        Write-SuccessMessage "Found Windows SDK: $SdkPath"
        break
    }
}

if ([string]::IsNullOrEmpty($WindowsSdkPath)) {
    Write-ErrorMessage "Windows SDK not found" "Please install Windows 10/11 SDK from Visual Studio Installer"
    Write-Host "Searched paths:" -ForegroundColor Gray
    foreach ($Path in $PossibleSdkPaths) {
        Write-Host "  $Path" -ForegroundColor Gray
    }
    exit 1
}

$MakeAppxPath = Join-Path $WindowsSdkPath "makeappx.exe"
$SignToolPath = Join-Path $WindowsSdkPath "signtool.exe"

# Build the project if not skipped
if (-not $SkipBuild) {
    Write-Host "?? Building Python Inline project..." -ForegroundColor Cyan
    
    # Check if MSBuild is available
    $MSBuildPath = ""
    $PossibleMSBuildPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    
    foreach ($MSBuild in $PossibleMSBuildPaths) {
        if (Test-Path $MSBuild) {
            $MSBuildPath = $MSBuild
            break
        }
    }
    
    if ([string]::IsNullOrEmpty($MSBuildPath)) {
        Write-ErrorMessage "MSBuild not found" "Please install Visual Studio 2019 or later"
        exit 1
    }
    
    Write-Host "Building with MSBuild: $MSBuildPath" -ForegroundColor Gray
    
    try {
        # Skip package installation since we already did it above
        & "$MSBuildPath" $ProjectPath /p:Configuration=$Configuration /p:Platform=$Platform /p:SkipPackageInstallation=true /v:minimal
        
        if ($LASTEXITCODE -eq 0) {
            Write-SuccessMessage "Build completed successfully"
        } else {
            Write-ErrorMessage "Build failed with exit code: $LASTEXITCODE" "Try building manually: MSBuild $ProjectPath /p:Configuration=$Configuration /p:Platform=$Platform"
            exit 1
        }
    } catch {
        Write-ErrorMessage "Build process failed: $($_.Exception.Message)"
        exit 1
    }
} else {
    Write-Host "??  Skipping build as requested" -ForegroundColor Yellow
}

# Verify executable exists and find correct path
Write-Host "?? Looking for python_inline.exe..." -ForegroundColor Cyan
$ExePath = Join-Path $BuildPath "python_inline.exe"
if (-not (Test-Path $ExePath)) {
    # Try alternative build output locations
    $AlternatePaths = @(
        (Join-Path $RootPath "x64\$Configuration\python_inline.exe"),
        (Join-Path $RootPath "$Platform\$Configuration\python_inline.exe"),
        (Join-Path $RootPath "python_inline.exe")
    )
    
    $Found = $false
    foreach ($AltPath in $AlternatePaths) {
        if (Test-Path $AltPath) {
            $ExePath = $AltPath
            $BuildPath = Split-Path $AltPath -Parent
            Write-SuccessMessage "Found executable at: $ExePath"
            $Found = $true
            break
        }
    }
    
    if (-not $Found) {
        Write-ErrorMessage "Built executable not found" "Make sure the build completed successfully"
        Write-Host "Expected path: $ExePath" -ForegroundColor Gray
        Write-Host "Alternative paths searched:" -ForegroundColor Gray
        foreach ($AltPath in $AlternatePaths) {
            Write-Host "  $AltPath" -ForegroundColor Gray
        }
        exit 1
    }
} else {
    Write-SuccessMessage "Found executable at: $ExePath"
}

# Prepare MSIX package directory
Write-Host "?? Preparing MSIX package directory..." -ForegroundColor Cyan

try {
    if (Test-Path $MSIXOutputPath) {
        Remove-Item $MSIXOutputPath -Recurse -Force -ErrorAction SilentlyContinue
    }
    New-Item -ItemType Directory -Path $MSIXOutputPath -Force | Out-Null

    $PackageDir = Join-Path $MSIXOutputPath "Package"
    New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null
    Write-SuccessMessage "Created package directory: $PackageDir"
} catch {
    Write-ErrorMessage "Failed to create package directory: $($_.Exception.Message)"
    exit 1
}

# Copy manifest from new location
Write-Host "?? Copying package manifest..." -ForegroundColor Cyan
try {
    if (Test-Path $MSIXManifestPath) {
        Copy-Item $MSIXManifestPath (Join-Path $PackageDir "AppxManifest.xml") -Force
        Write-SuccessMessage "Copied manifest from new location"
    } else {
        Write-ErrorMessage "Manifest not found at: $MSIXManifestPath"
        exit 1
    }
} catch {
    Write-ErrorMessage "Failed to copy manifest: $($_.Exception.Message)"
    exit 1
}

# Create Assets directory and copy assets from new location
Write-Host "?? Setting up assets..." -ForegroundColor Cyan
$AssetsDir = Join-Path $PackageDir "Assets"
try {
    New-Item -ItemType Directory -Path $AssetsDir -Force | Out-Null

    # Copy assets from the new structure
    $SourceAssetsDir = Join-Path $PythonInlinePath "packaging\msix\Assets"
    if (Test-Path $SourceAssetsDir) {
        Copy-Item "$SourceAssetsDir\*" $AssetsDir -Force -ErrorAction SilentlyContinue
        Write-SuccessMessage "Copied assets from new structure"
    } else {
        # Create minimal placeholder assets
        $LogoBytes = [Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==")
        [System.IO.File]::WriteAllBytes((Join-Path $AssetsDir "logo.png"), $LogoBytes)
        Write-SuccessMessage "Created placeholder logo assets"
    }
} catch {
    Write-ErrorMessage "Failed to create assets: $($_.Exception.Message)"
    exit 1
}

# Copy main executable
Write-Host "?? Copying executable..." -ForegroundColor Cyan
try {
    Copy-Item $ExePath $PackageDir -Force
    Write-SuccessMessage "Copied python_inline.exe"
} catch {
    Write-ErrorMessage "Failed to copy executable: $($_.Exception.Message)"
    exit 1
}

# Copy required Python DLLs and libraries
Write-Host "?? Copying Python dependencies..." -ForegroundColor Cyan
$RequiredFiles = @(
    "python*.dll",
    "python*_d.dll",  # Debug version
    "_*.pyd",
    "*.pyd"
)

$CopiedFiles = 0
foreach ($Pattern in $RequiredFiles) {
    try {
        $Files = Get-ChildItem (Join-Path $BuildPath $Pattern) -ErrorAction SilentlyContinue
        foreach ($File in $Files) {
            Copy-Item $File.FullName $PackageDir -Force
            $CopiedFiles++
        }
    } catch {
        Write-WarningMessage "Could not copy some files matching pattern: $Pattern"
    }
}
Write-SuccessMessage "Copied $CopiedFiles dependency files"

# Copy external packages if they exist
$ExternalPackagesPath = Join-Path $PythonInlinePath "packaging\site-packages"
if (Test-Path $ExternalPackagesPath) {
    Write-Host "?? Copying external packages..." -ForegroundColor Cyan
    try {
        $ExtPackageDestPath = Join-Path $PackageDir "site-packages"
        Copy-Item $ExternalPackagesPath $ExtPackageDestPath -Recurse -Force
        
        # Count packages
        $PackageCount = (Get-ChildItem $ExtPackageDestPath -Directory | Measure-Object).Count
        $PackageSize = (Get-ChildItem $ExtPackageDestPath -Recurse -File | Measure-Object -Property Length -Sum).Sum
        
        Write-SuccessMessage "Copied external packages ($PackageCount packages, $([math]::Round($PackageSize / 1MB, 2)) MB)"
        
        # Show manifest if it exists
        $ManifestPath = Join-Path $ExtPackageDestPath "python_inline_packages.txt"
        if (Test-Path $ManifestPath) {
            Write-Host "?? Installed packages:" -ForegroundColor Gray
            $Manifest = Get-Content $ManifestPath | Where-Object { $_ -notlike "#*" -and $_.Trim() -ne "" }
            foreach ($Package in $Manifest) {
                Write-Host "   ?? $Package" -ForegroundColor White
            }
        }
    } catch {
        Write-WarningMessage "Could not copy external packages: $($_.Exception.Message)"
    }
} else {
    Write-Host "?? No external packages to copy (site-packages not found)" -ForegroundColor Gray
}

# Copy Python standard library (essential parts only to reduce size)
Write-Host "?? Copying Python standard library..." -ForegroundColor Cyan
$PythonLibPath = Join-Path $RootPath "..\Lib"
if (Test-Path $PythonLibPath) {
    try {
        $LibDestPath = Join-Path $PackageDir "Lib"
        
        # Copy essential modules only to reduce package size
        $EssentialModules = @(
            "encodings", "importlib", "collections", "json", "re", "os.py", 
            "sys.py", "io.py", "codecs.py", "locale.py", "abc.py", "types.py",
            "warnings.py", "platform.py", "stat.py", "genericpath.py", "posixpath.py",
            "ntpath.py", "_collections_abc.py", "keyword.py", "operator.py",
            "functools.py", "reprlib.py", "weakref.py", "linecache.py", "tokenize.py",
            "site.py", "sitepackages"  # Important for external packages
        )
        
        New-Item -ItemType Directory -Path $LibDestPath -Force | Out-Null
        
        foreach ($Module in $EssentialModules) {
            $SourcePath = Join-Path $PythonLibPath $Module
            if (Test-Path $SourcePath) {
                $DestPath = Join-Path $LibDestPath $Module
                if (Test-Path $SourcePath -PathType Container) {
                    Copy-Item $SourcePath $DestPath -Recurse -Force -ErrorAction SilentlyContinue
                } else {
                    Copy-Item $SourcePath $DestPath -Force -ErrorAction SilentlyContinue
                }
            }
        }
        Write-SuccessMessage "Copied essential Python standard library modules"
    } catch {
        Write-WarningMessage "Could not copy Python library: $($_.Exception.Message)"
    }
} else {
    Write-WarningMessage "Python library path not found: $PythonLibPath"
}

# Create the MSIX package
Write-Host "?? Creating MSIX package..." -ForegroundColor Cyan
$MSIXFileName = "PythonInlineRunner_$($Configuration)_$($Platform).msix"
$MSIXFilePath = Join-Path $MSIXOutputPath $MSIXFileName

Write-Host "Running: makeappx pack..." -ForegroundColor Gray
try {
    # Remove old package if exists
    if (Test-Path $MSIXFilePath) {
        Remove-Item $MSIXFilePath -Force
    }
    
    $ProcessInfo = Start-Process -FilePath $MakeAppxPath -ArgumentList "pack", "/d", "`"$PackageDir`"", "/p", "`"$MSIXFilePath`"", "/overwrite" -Wait -PassThru -RedirectStandardOutput "$MSIXOutputPath\makeappx_output.log" -RedirectStandardError "$MSIXOutputPath\makeappx_error.log"
    
    if ($ProcessInfo.ExitCode -eq 0) {
        Write-SuccessMessage "MSIX package created successfully"
        if (Test-Path $MSIXFilePath) {
            $Size = (Get-Item $MSIXFilePath).Length / 1MB
            Write-Host "?? Package size: $([math]::Round($Size, 2)) MB" -ForegroundColor Cyan
        }
    } else {
        Write-ErrorMessage "MSIX package creation failed with exit code: $($ProcessInfo.ExitCode)"
        if (Test-Path "$MSIXOutputPath\makeappx_error.log") {
            Write-Host "Error details:" -ForegroundColor Yellow
            Get-Content "$MSIXOutputPath\makeappx_error.log" | Write-Host -ForegroundColor Red
        }
        exit 1
    }
} catch {
    Write-ErrorMessage "Error running makeappx: $($_.Exception.Message)"
    exit 1
}

# Sign the package (optional) - Look for certificate in new location
Write-Host "?? Signing package..." -ForegroundColor Cyan
$CertPath = Join-Path $PythonInlinePath "packaging\ContosoLab 2.pfx"
$CertPassword = "MSIX!Lab1809"

$SignedSuccessfully = $false

# Try certificate from new location
if (Test-Path $CertPath) {
    Write-Host "Using certificate from new location..." -ForegroundColor Gray
    try {
        $SignProcess = Start-Process -FilePath $SignToolPath -ArgumentList "sign", "/f", "`"$CertPath`"", "/p", $CertPassword, "/fd", "SHA256", "`"$MSIXFilePath`"" -Wait -PassThru
        if ($SignProcess.ExitCode -eq 0) {
            $SignedSuccessfully = $true
            Write-SuccessMessage "Package signed with certificate"
        }
    } catch {
        Write-WarningMessage "Could not use certificate: $($_.Exception.Message)"
    }
} else {
    Write-WarningMessage "Certificate not found at: $CertPath"
}

if (-not $SignedSuccessfully) {
    Write-WarningMessage "Failed to sign package. Package created but unsigned"
}

# Create installation script with package information
Write-Host "?? Creating installation script..." -ForegroundColor Cyan
$InstallScript = @"
# Installation script for Python Inline Runner (with External Packages)
# Run this script as Administrator

`$MSIXPath = "$MSIXFilePath"
`$CertPath = "$CertPath"
`$CertPassword = "$CertPassword"

Write-Host "?? Installing Python Inline Runner..." -ForegroundColor Green

# Function to install certificate to multiple stores for better trust
function Install-Certificate {
    param([string]`$CertificatePath, [string]`$Password)
    
    try {
        # Import to Trusted Root Certification Authorities for system-wide trust
        Write-Host "Installing certificate to Trusted Root..." -ForegroundColor Yellow
        if (`$Password) {
            `$SecurePassword = ConvertTo-SecureString -String `$Password -AsPlainText -Force
            Import-PfxCertificate -FilePath `$CertificatePath -CertStoreLocation Cert:\LocalMachine\Root -Password `$SecurePassword -Confirm:`$false | Out-Null
        } else {
            Import-Certificate -FilePath `$CertificatePath -CertStoreLocation Cert:\LocalMachine\Root -Confirm:`$false | Out-Null
        }
        
        # Also import to Trusted People for additional compatibility
        Write-Host "Installing certificate to Trusted People..." -ForegroundColor Yellow
        if (`$Password) {
            Import-PfxCertificate -FilePath `$CertificatePath -CertStoreLocation Cert:\LocalMachine\TrustedPeople -Password `$SecurePassword -Confirm:`$false | Out-Null
        } else {
            Import-Certificate -FilePath `$CertificatePath -CertStoreLocation Cert:\LocalMachine\TrustedPeople -Confirm:`$false | Out-Null
        }
        
        Write-Host "? Certificate installed successfully to both Trusted Root and Trusted People stores" -ForegroundColor Green
        return `$true
    } catch {
        Write-Host "? Failed to install certificate: `$(`$_.Exception.Message)" -ForegroundColor Red
        return `$false
    }
}

# Install certificate if it exists
if (Test-Path "`$CertPath") {
    Write-Host "Installing certificate from: `$CertPath" -ForegroundColor Yellow
    `$certInstalled = Install-Certificate -CertificatePath "`$CertPath" -Password "`$CertPassword"
    
    if (-not `$certInstalled) {
        Write-Host "??  Certificate installation failed. You may need to install it manually." -ForegroundColor Yellow
        Write-Host "Manual installation steps:" -ForegroundColor Cyan
        Write-Host "1. Double-click the certificate file: `$CertPath" -ForegroundColor White
        Write-Host "2. Click 'Install Certificate...'" -ForegroundColor White
        Write-Host "3. Select 'Local Machine' and click 'Next'" -ForegroundColor White
        Write-Host "4. Select 'Place all certificates in the following store'" -ForegroundColor White
        Write-Host "5. Click 'Browse' and select 'Trusted Root Certification Authorities'" -ForegroundColor White
        Write-Host "6. Click 'Next' then 'Finish'" -ForegroundColor White
        
        `$continue = Read-Host "Press Enter to continue with MSIX installation, or type 'exit' to stop"
        if (`$continue -eq 'exit') {
            return
        }
    }
} else {
    Write-Host "??  Certificate file not found: `$CertPath" -ForegroundColor Yellow
}

# Install MSIX package
Write-Host ""
Write-Host "Installing MSIX package..." -ForegroundColor Yellow
Write-Host "Package: `$MSIXPath" -ForegroundColor Gray

try {
    # First, try to remove any existing version
    try {
        Get-AppxPackage -Name "PythonInlineRunner" | Remove-AppxPackage -Confirm:`$false
        Write-Host "Removed existing version" -ForegroundColor Gray
    } catch {
        # Ignore errors if package doesn't exist
    }
    
    # Install the new package
    Add-AppxPackage -Path "`$MSIXPath" -ForceApplicationShutdown
    Write-Host "? Installation completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Python Inline Runner is now available with external package support!" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Basic commands:" -ForegroundColor Cyan
    Write-Host "  python-inline -c `"print('Hello, World!')`"" -ForegroundColor White
    Write-Host "  pyinline --version" -ForegroundColor White
    Write-Host "  python-inline --file=script.py arg1 arg2" -ForegroundColor White
    Write-Host ""
    Write-Host "Package management:" -ForegroundColor Cyan
    Write-Host "  python-inline --list-packages" -ForegroundColor White
    Write-Host "  python-inline --add-package=requests -c `"import requests; print('OK')`"" -ForegroundColor White
    Write-Host ""
    Write-Host "Test external packages:" -ForegroundColor Cyan
    Write-Host "  python-inline -c `"import requests; print('Requests available!')`"" -ForegroundColor White
    
} catch {
    Write-Host "? Error installing package: `$(`$_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting steps:" -ForegroundColor Yellow
    Write-Host "1. Make sure you're running PowerShell as Administrator" -ForegroundColor White
    Write-Host "2. Enable Developer Mode in Windows Settings > Update & Security > For developers" -ForegroundColor White
    Write-Host "3. Try installing the certificate manually (see steps above)" -ForegroundColor White
    Write-Host "4. Check if Windows Defender or antivirus is blocking the installation" -ForegroundColor White
}
"@

try {
    $InstallScriptPath = Join-Path $MSIXOutputPath "Install.ps1"
    $InstallScript | Out-File $InstallScriptPath -Encoding UTF8
    Write-SuccessMessage "Created installation script"
} catch {
    Write-WarningMessage "Could not create installation script: $($_.Exception.Message)"
}

# Final summary
Write-Host ""
Write-Host "?? MSIX Package Build Complete!" -ForegroundColor Green
Write-Host "===============================" -ForegroundColor Green
Write-Host "?? Package: $MSIXFilePath" -ForegroundColor White
Write-Host "?? Install Script: $InstallScriptPath" -ForegroundColor White
if (Test-Path $CertPath) {
    Write-Host "?? Certificate: $CertPath" -ForegroundColor White
}

# Show package information
if (Test-Path $ExternalPackagesPath) {
    $ManifestPath = Join-Path $ExternalPackagesPath "python_inline_packages.txt"
    if (Test-Path $ManifestPath) {
        Write-Host "?? Bundled Packages:" -ForegroundColor White
        $Manifest = Get-Content $ManifestPath | Where-Object { $_ -notlike "#*" -and $_.Trim() -ne "" }
        foreach ($Package in $Manifest) {
            Write-Host "   ?? $Package" -ForegroundColor Cyan
        }
    }
}

Write-Host ""
Write-Host "To install:" -ForegroundColor Cyan
Write-Host "1. Open PowerShell as Administrator" -ForegroundColor White
Write-Host "2. Run: & '$InstallScriptPath'" -ForegroundColor White
Write-Host ""
Write-Host "Usage examples after installation:" -ForegroundColor Cyan
Write-Host "  python-inline -c `"print('Hello, World!')`"" -ForegroundColor White
Write-Host "  python-inline --list-packages" -ForegroundColor White
Write-Host "  python-inline -c `"import requests; print('Requests working!')`"" -ForegroundColor White
Write-Host "  python-inline --file=script.py arg1 arg2" -ForegroundColor White
Write-Host ""

# Install if requested
if ($InstallAfterBuild) {
    Write-Host "?? Installing package as requested..." -ForegroundColor Cyan
    try {
        Start-Process PowerShell -ArgumentList "-Command", "& '$InstallScriptPath'" -Verb RunAs -Wait
        Write-SuccessMessage "Installation process completed"
    } catch {
        Write-WarningMessage "Could not auto-install. Please run the install script manually as Administrator"
    }
}