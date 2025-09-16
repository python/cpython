# Installation script for Python Inline Runner (with External Packages)
# Run this script as Administrator

$MSIXPath = ".\PythonInline\output\msix\PythonInlineRunner_Release_x64.msix"
$CertPath = "C:\Users\manojkumar\source\repos\rpython4w\PCbuild\PythonInline\packaging\ContosoLab 2.pfx"
$CertPassword = "MSIX!Lab1809"

Write-Host "?? Installing Python Inline Runner..." -ForegroundColor Green

# Function to install certificate to multiple stores for better trust
function Install-Certificate {
    param([string]$CertificatePath, [string]$Password)
    
    try {
        # Import to Trusted Root Certification Authorities for system-wide trust
        Write-Host "Installing certificate to Trusted Root..." -ForegroundColor Yellow
        if ($Password) {
            $SecurePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force
            Import-PfxCertificate -FilePath $CertificatePath -CertStoreLocation Cert:\LocalMachine\Root -Password $SecurePassword -Confirm:$false | Out-Null
        } else {
            Import-Certificate -FilePath $CertificatePath -CertStoreLocation Cert:\LocalMachine\Root -Confirm:$false | Out-Null
        }
        
        # Also import to Trusted People for additional compatibility
        Write-Host "Installing certificate to Trusted People..." -ForegroundColor Yellow
        if ($Password) {
            Import-PfxCertificate -FilePath $CertificatePath -CertStoreLocation Cert:\LocalMachine\TrustedPeople -Password $SecurePassword -Confirm:$false | Out-Null
        } else {
            Import-Certificate -FilePath $CertificatePath -CertStoreLocation Cert:\LocalMachine\TrustedPeople -Confirm:$false | Out-Null
        }
        
        Write-Host "? Certificate installed successfully to both Trusted Root and Trusted People stores" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "? Failed to install certificate: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# Install certificate if it exists
if (Test-Path "$CertPath") {
    Write-Host "Installing certificate from: $CertPath" -ForegroundColor Yellow
    $certInstalled = Install-Certificate -CertificatePath "$CertPath" -Password "$CertPassword"
    
    if (-not $certInstalled) {
        Write-Host "??  Certificate installation failed. You may need to install it manually." -ForegroundColor Yellow
        Write-Host "Manual installation steps:" -ForegroundColor Cyan
        Write-Host "1. Double-click the certificate file: $CertPath" -ForegroundColor White
        Write-Host "2. Click 'Install Certificate...'" -ForegroundColor White
        Write-Host "3. Select 'Local Machine' and click 'Next'" -ForegroundColor White
        Write-Host "4. Select 'Place all certificates in the following store'" -ForegroundColor White
        Write-Host "5. Click 'Browse' and select 'Trusted Root Certification Authorities'" -ForegroundColor White
        Write-Host "6. Click 'Next' then 'Finish'" -ForegroundColor White
        
        $continue = Read-Host "Press Enter to continue with MSIX installation, or type 'exit' to stop"
        if ($continue -eq 'exit') {
            return
        }
    }
} else {
    Write-Host "??  Certificate file not found: $CertPath" -ForegroundColor Yellow
}

# Install MSIX package
Write-Host ""
Write-Host "Installing MSIX package..." -ForegroundColor Yellow
Write-Host "Package: $MSIXPath" -ForegroundColor Gray

try {
    # First, try to remove any existing version
    try {
        Get-AppxPackage -Name "PythonInlineRunner" | Remove-AppxPackage -Confirm:$false
        Write-Host "Removed existing version" -ForegroundColor Gray
    } catch {
        # Ignore errors if package doesn't exist
    }
    
    # Install the new package
    Add-AppxPackage -Path "$MSIXPath" -ForceApplicationShutdown
    Write-Host "? Installation completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Python Inline Runner is now available with external package support!" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Basic commands:" -ForegroundColor Cyan
    Write-Host "  python-inline -c "print('Hello, World!')"" -ForegroundColor White
    Write-Host "  pyinline --version" -ForegroundColor White
    Write-Host "  python-inline --file=script.py arg1 arg2" -ForegroundColor White
    Write-Host ""
    Write-Host "Package management:" -ForegroundColor Cyan
    Write-Host "  python-inline --list-packages" -ForegroundColor White
    Write-Host "  python-inline --add-package=requests -c "import requests; print('OK')"" -ForegroundColor White
    Write-Host ""
    Write-Host "Test external packages:" -ForegroundColor Cyan
    Write-Host "  python-inline -c "import requests; print('Requests available!')"" -ForegroundColor White
    
} catch {
    Write-Host "? Error installing package: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting steps:" -ForegroundColor Yellow
    Write-Host "1. Make sure you're running PowerShell as Administrator" -ForegroundColor White
    Write-Host "2. Enable Developer Mode in Windows Settings > Update & Security > For developers" -ForegroundColor White
    Write-Host "3. Try installing the certificate manually (see steps above)" -ForegroundColor White
    Write-Host "4. Check if Windows Defender or antivirus is blocking the installation" -ForegroundColor White
}
