# Build-Time Package Bundling for Python Inline

## ?? Overview

Python Inline now supports **automatic build-time package bundling**, which means external Python packages are downloaded and bundled during the build process, not at runtime. This creates truly self-contained executables that don't require external package installation.

## ?? How It Works

### Build Process Flow

1. **Pre-Build**: Check `PythonInline/packaging/requirements.txt`
2. **Package Installation**: Download packages to `PythonInline/packaging/site-packages/`
3. **Build**: Compile Python Inline executable
4. **Post-Build**: Copy bundled packages to output directory
5. **Result**: Self-contained executable with bundled packages

### Architecture

```
PythonInline/
??? packaging/
?   ??? requirements.txt          # Package specifications
?   ??? site-packages/           # Bundled packages (build-time)
?   ??? Install-Packages.ps1     # Package bundling script
??? build/
?   ??? python_inline.vcxproj    # Build with package targets
??? output/
    ??? python_inline.exe        # Main executable
    ??? site-packages/           # Runtime package directory
```

## ?? Configuration

### requirements.txt Format

The `PythonInline/packaging/requirements.txt` file specifies which packages to bundle:

```text
# HTTP and web APIs
requests==2.31.0

# Data analysis (uncomment to enable)
# numpy>=1.24.0
# pandas>=2.0.0

# Web scraping
# beautifulsoup4>=4.12.0
# lxml>=4.9.0

# File handling
# openpyxl>=3.1.0
# PyYAML>=6.0

# Custom packages
# your_package_name>=1.0.0
```

### Package Specification Formats

- **Exact version**: `requests==2.31.0`
- **Minimum version**: `numpy>=1.24.0`
- **Latest version**: `pandas` (no version specified)
- **Version ranges**: `django>=4.0.0,<5.0.0`
- **Comments**: Lines starting with `#` are ignored
- **Empty lines**: Ignored

## ?? Usage

### Automatic Bundling (Recommended)

Packages are automatically bundled during build:

```cmd
# Build with automatic package bundling
msbuild PythonInline\build\python_inline.vcxproj /p:Configuration=Release

# Or from main project
msbuild python_inline.vcxproj /p:Configuration=Release
```

### Manual Package Installation

You can also bundle packages manually:

```powershell
# Install packages to bundling directory
.\PythonInline\packaging\Install-Packages.ps1

# Force reinstall all packages
.\PythonInline\packaging\Install-Packages.ps1 -Force

# Bundle with minimal dependencies
.\PythonInline\packaging\Install-Packages.ps1 -NoDeps

# Verbose output for debugging
.\PythonInline\packaging\Install-Packages.ps1 -Verbose
```

### Using Bundled Packages

Once built, packages are automatically available:

```cmd
# HTTP requests
python_inline.exe -c "import requests; r = requests.get('https://httpbin.org/json'); print(r.json())"

# Data analysis (if numpy/pandas are bundled)
python_inline.exe -c "import pandas as pd; print(pd.__version__)"

# File processing (if openpyxl is bundled)
python_inline.exe -c "import openpyxl; print('Excel support available')"

# List bundled packages
python_inline.exe --list-packages
```

## ?? Build Integration

### MSBuild Targets

The build system includes these targets:

1. **InstallExternalPackages** (BeforeTargets="Build")
   - Checks if `requirements.txt` exists
   - Compares timestamps to avoid unnecessary reinstalls
   - Runs PowerShell package installation script
   - Creates package manifest

2. **CopySitePackages** (AfterTargets="Build")
   - Copies bundled packages to output directory
   - Preserves directory structure
   - Skips unchanged files for performance

### Incremental Building

The system is optimized for incremental builds:

- **Smart Detection**: Only reinstalls if `requirements.txt` is newer than existing packages
- **Timestamp Checking**: Compares file modification times
- **Selective Updates**: Only processes changed requirements
- **Cache Friendly**: Preserves packages between builds when possible

### Build Conditions

Package bundling only occurs when:

1. `PythonInline/packaging/requirements.txt` exists
2. File contains uncommented package specifications
3. PowerShell execution is available
4. Python and pip are accessible

## ?? Bundle Optimization

### Size Optimization

The bundling process automatically optimizes bundle size:

```powershell
# Files removed to reduce size:
*.pyc            # Compiled Python files
__pycache__      # Python cache directories
*.pyo            # Optimized Python files
tests/           # Test directories
*.dist-info/WHEEL # Wheel metadata
```

### Dependency Management

You can control dependency bundling:

```powershell
# Full bundling (default) - includes all dependencies
.\Install-Packages.ps1

# Minimal bundling - only specified packages
.\Install-Packages.ps1 -NoDeps
```

## ??? Troubleshooting

### Common Issues

#### Build Fails with Package Errors

```
Error: Failed to install package 'some_package'
```

**Solutions:**
1. Check Python installation: `python --version`
2. Verify pip availability: `python -m pip --version`
3. Test package individually: `pip install some_package`
4. Check network connectivity
5. Use specific versions: `some_package==1.2.3`

#### Packages Not Found at Runtime

```
ModuleNotFoundError: No module named 'requests'
```

**Solutions:**
1. Verify packages were bundled: `python_inline.exe --list-packages`
2. Check output directory: `dir output_dir\site-packages`
3. Rebuild with force: `Install-Packages.ps1 -Force`
4. Verify requirements.txt format

#### PowerShell Execution Policy

```
Execution of scripts is disabled on this system
```

**Solutions:**
1. Run as Administrator: `Set-ExecutionPolicy RemoteSigned`
2. Bypass for single command: `powershell -ExecutionPolicy Bypass -File script.ps1`
3. Use `msbuild` directly (skips PowerShell)

### Debug Information

Enable verbose output for debugging:

```powershell
# Verbose package installation
.\Install-Packages.ps1 -Verbose

# MSBuild with detailed output
msbuild python_inline.vcxproj /p:Configuration=Release /verbosity:detailed
```

### Package Verification

Verify bundled packages:

```cmd
# List all bundled packages
python_inline.exe --list-packages

# Check specific package
python_inline.exe -c "import package_name; print(package_name.__version__)"

# List Python path
python_inline.exe -c "import sys; [print(p) for p in sys.path]"
```

## ?? Best Practices

### Package Selection

1. **Choose Stable Versions**: Use specific versions for reproducible builds
2. **Minimize Dependencies**: Only bundle what you need
3. **Test Compatibility**: Verify packages work together
4. **Monitor Size**: Large packages increase executable size

### Requirements Management

```text
# Good: Specific versions for reproducibility
requests==2.31.0
numpy==1.24.3

# Good: Minimum versions for compatibility
pandas>=2.0.0

# Avoid: Unspecified versions (can break builds)
# some_package

# Good: Comments for documentation
# Data analysis packages
numpy>=1.24.0
pandas>=2.0.0
```

### Build Performance

1. **Use Incremental Builds**: Don't force reinstall unless needed
2. **Cache Packages**: Keep `site-packages` directory between builds
3. **Selective Updates**: Only change `requirements.txt` when necessary
4. **Parallel Builds**: MSBuild can handle multiple projects

### Deployment

1. **Test Thoroughly**: Verify all bundled packages work in target environment
2. **Document Dependencies**: Keep `requirements.txt` updated
3. **Version Control**: Include `requirements.txt` in source control
4. **Exclude Build Artifacts**: Add `site-packages/` to `.gitignore`

## ?? Advanced Features

### Custom Python Executable

Use specific Python version for bundling:

```powershell
# Use specific Python installation
.\Install-Packages.ps1 -PythonExecutable "C:\Python311\python.exe"

# Use Python from PATH
.\Install-Packages.ps1 -PythonExecutable "python3.11"
```

### Custom Package Sources

Configure pip for private repositories:

```powershell
# Set pip configuration before bundling
pip config set global.index-url https://your-private-repo.com/simple/
.\Install-Packages.ps1
```

### Multiple Requirements Files

Use different requirements for different scenarios:

```powershell
# Development packages
.\Install-Packages.ps1 -RequirementsFile "requirements-dev.txt"

# Production packages
.\Install-Packages.ps1 -RequirementsFile "requirements-prod.txt"

# Minimal packages
.\Install-Packages.ps1 -RequirementsFile "requirements-minimal.txt"
```

## ?? Bundle Analysis

### Size Analysis

Monitor bundle size impact:

```powershell
# Check bundle size
Get-ChildItem "PythonInline\packaging\site-packages" -Recurse | Measure-Object -Property Length -Sum

# Largest packages
Get-ChildItem "PythonInline\packaging\site-packages" -Directory | ForEach-Object {
    $size = (Get-ChildItem $_.FullName -Recurse | Measure-Object -Property Length -Sum).Sum
    [PSCustomObject]@{Package=$_.Name; SizeMB=[math]::Round($size/1MB, 2)}
} | Sort-Object SizeMB -Descending
```

### Dependency Analysis

Understand package dependencies:

```cmd
# Show package dependencies
python_inline.exe -c "import pkg_resources; [print(f'{pkg.key}: {[str(req) for req in pkg.requires()]}') for pkg in pkg_resources.working_set]"
```

## ?? Success!

With build-time package bundling, Python Inline creates truly portable executables that include all necessary dependencies. Users can run scripts with external packages without any additional installation or configuration!

Example workflow:
1. Edit `PythonInline/packaging/requirements.txt`
2. Build: `msbuild python_inline.vcxproj`
3. Distribute: Single executable with bundled packages
4. Use: `python_inline.exe -c "import requests; print('Ready!')"`
