# Test Script to Find and Run Python Inline from MSIX Installation
# Run this to test if the MSIX package installed correctly

Write-Host "?? Searching for Python Inline Runner installation..." -ForegroundColor Cyan

# Check if package is installed
$package = Get-AppxPackage -Name "PythonInlineRunner" -ErrorAction SilentlyContinue
if ($package) {
    Write-Host "? Package found: $($package.Name)" -ForegroundColor Green
    Write-Host "   Version: $($package.Version)" -ForegroundColor Gray
    Write-Host "   Install Location: $($package.InstallLocation)" -ForegroundColor Gray
    
    # Try to find the executable
    $exePath = Join-Path $package.InstallLocation "python_inline.exe"
    if (Test-Path $exePath) {
        Write-Host "? Executable found at: $exePath" -ForegroundColor Green
        
        # Test the executable
        Write-Host "?? Testing executable..." -ForegroundColor Yellow
        try {
            $result = & $exePath -c "print('Hello from MSIX Python Inline!')" 2>&1
            Write-Host "? Test successful: $result" -ForegroundColor Green
            
            # Create convenience aliases
            Write-Host "?? Creating PowerShell aliases..." -ForegroundColor Cyan
            Write-Host "Add these to your PowerShell profile:" -ForegroundColor Yellow
            Write-Host "Set-Alias python-inline '$exePath'" -ForegroundColor White
            Write-Host "Set-Alias pyinline '$exePath'" -ForegroundColor White
            
            # Test with Python import
            Write-Host "?? Testing Python import..." -ForegroundColor Yellow
            $importResult = & $exePath -c "import os; print(f'Python working! CWD: {os.getcwd()}')" 2>&1
            Write-Host "? Import test: $importResult" -ForegroundColor Green
            
        } catch {
            Write-Host "? Test failed: $($_.Exception.Message)" -ForegroundColor Red
        }
    } else {
        Write-Host "? Executable not found at expected location" -ForegroundColor Red
    }
} else {
    Write-Host "? PythonInlineRunner package not found" -ForegroundColor Red
    
    # Search for any Python-related packages
    $pythonPackages = Get-AppxPackage | Where-Object {$_.Name -like "*Python*"}
    if ($pythonPackages) {
        Write-Host "Found other Python-related packages:" -ForegroundColor Yellow
        foreach ($pkg in $pythonPackages) {
            Write-Host "  - $($pkg.Name) v$($pkg.Version)" -ForegroundColor Gray
        }
    }
}

# Check execution aliases
Write-Host "?? Checking execution aliases..." -ForegroundColor Cyan
$aliases = @("python-inline", "pyinline")
foreach ($alias in $aliases) {
    try {
        $cmd = Get-Command $alias -ErrorAction Stop
        Write-Host "? $alias found: $($cmd.Source)" -ForegroundColor Green
    } catch {
        Write-Host "? $alias not found" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "?? If aliases don't work, you can run directly:" -ForegroundColor Cyan
if ($package -and $exePath) {
    Write-Host "`"$exePath`" -c `"print('Hello World!')`"" -ForegroundColor White
}