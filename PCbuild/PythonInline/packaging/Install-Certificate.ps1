# Certificate Installation Script for ContosoLab Certificate
# Run this script as Administrator BEFORE installing the MSIX package

param(
    [Parameter(Mandatory=$false)]
    [string]$CertPath = "C:\Users\manojkumar\Downloads\ContosoLab\ContosoLab 2.pfx",
    
    [Parameter(Mandatory=$false)]
    [string]$CertPassword = "MSIX!Lab1809"
)

Write-Host "?? ContosoLab Certificate Installation Script" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green

# Check if running as Administrator
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "? This script must be run as Administrator" -ForegroundColor Red
    Write-Host "Right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if certificate file exists
if (-not (Test-Path $CertPath)) {
    Write-Host "? Certificate file not found: $CertPath" -ForegroundColor Red
    Write-Host "Please verify the path and try again." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "?? Certificate file: $CertPath" -ForegroundColor Cyan
Write-Host ""

try {
    # Convert password to secure string
    $SecurePassword = ConvertTo-SecureString -String $CertPassword -AsPlainText -Force
    
    # Install to Trusted Root Certification Authorities (most important for MSIX)
    Write-Host "Installing to Trusted Root Certification Authorities..." -ForegroundColor Yellow
    $rootCert = Import-PfxCertificate -FilePath $CertPath -CertStoreLocation Cert:\LocalMachine\Root -Password $SecurePassword -Confirm:$false
    Write-Host "? Installed to Trusted Root: $($rootCert.Subject)" -ForegroundColor Green
    
    # Install to Trusted People (additional compatibility)
    Write-Host "Installing to Trusted People..." -ForegroundColor Yellow
    $peopleCert = Import-PfxCertificate -FilePath $CertPath -CertStoreLocation Cert:\LocalMachine\TrustedPeople -Password $SecurePassword -Confirm:$false
    Write-Host "? Installed to Trusted People: $($peopleCert.Subject)" -ForegroundColor Green
    
    # Install to Personal store (for signing operations)
    Write-Host "Installing to Personal store..." -ForegroundColor Yellow
    $personalCert = Import-PfxCertificate -FilePath $CertPath -CertStoreLocation Cert:\LocalMachine\My -Password $SecurePassword -Confirm:$false
    Write-Host "? Installed to Personal store: $($personalCert.Subject)" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "?? Certificate installation completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Certificate Details:" -ForegroundColor Cyan
    Write-Host "  Subject: $($rootCert.Subject)" -ForegroundColor White
    Write-Host "  Issuer: $($rootCert.Issuer)" -ForegroundColor White
    Write-Host "  Thumbprint: $($rootCert.Thumbprint)" -ForegroundColor White
    Write-Host "  Valid From: $($rootCert.NotBefore)" -ForegroundColor White
    Write-Host "  Valid To: $($rootCert.NotAfter)" -ForegroundColor White
    
    # Check if certificate is valid
    $now = Get-Date
    if ($rootCert.NotAfter -lt $now) {
        Write-Host "??  WARNING: Certificate has expired!" -ForegroundColor Red
    } elseif ($rootCert.NotBefore -gt $now) {
        Write-Host "??  WARNING: Certificate is not yet valid!" -ForegroundColor Yellow
    } else {
        Write-Host "? Certificate is currently valid" -ForegroundColor Green
    }
    
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "1. You can now install the MSIX package without certificate errors" -ForegroundColor White
    Write-Host "2. Run: .\Build-MSIX.ps1 -SkipBuild -InstallAfterBuild" -ForegroundColor White
    Write-Host "3. Or run: .\MSIX_Output\Install.ps1" -ForegroundColor White
    
} catch {
    Write-Host "? Certificate installation failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual installation steps:" -ForegroundColor Yellow
    Write-Host "1. Double-click the certificate file: $CertPath" -ForegroundColor White
    Write-Host "2. Enter password: $CertPassword" -ForegroundColor White
    Write-Host "3. Select 'Local Machine' and click 'Next'" -ForegroundColor White
    Write-Host "4. Select 'Place all certificates in the following store'" -ForegroundColor White
    Write-Host "5. Click 'Browse' and select 'Trusted Root Certification Authorities'" -ForegroundColor White
    Write-Host "6. Click 'Next' then 'Finish'" -ForegroundColor White
    Write-Host "7. Repeat for 'Trusted People' store if needed" -ForegroundColor White
}

Read-Host "Press Enter to exit"