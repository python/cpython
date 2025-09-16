# Python Inline Runner

A custom Python entry point that can execute inline scripts and package as MSIX for Windows, now with **enhanced build-time package bundling**!

## ?? Quick Start

### Installation from MSIX Package

1. **Build the MSIX package with bundled packages:**
   ```powershell
   # From PCbuild directory - packages are automatically bundled during build
   .\PythonInline\packaging\Build-MSIX.ps1
   ```

2. **Install the package:**
   ```powershell
   # Run as Administrator
   .\PythonInline\output\msix\Install.ps1
   ```

3. **Test the installation with bundled packages:**
   ```cmd
   python-inline -c "print('Hello, World!')"
   python-inline --list-packages
   python-inline -c "import requests; print('Requests bundled and ready!')"
   ```

### Development Build

```powershell
# Build from source - packages automatically bundled from requirements.txt
msbuild PythonInline\build\python_inline.vcxproj /p:Configuration=Release /p:Platform=x64

# Run directly with bundled packages
.\amd64\python_inline.exe -c "print('Hello from Python Inline with bundled packages!')"
```

## ?? Build-Time Package Bundling (Enhanced!)

Python Inline now features **automatic build-time package bundling** - packages are downloaded and bundled during compilation, creating truly self-contained executables.

### ?? How It Works

1. **Configure**: Edit `PythonInline/packaging/requirements.txt`
2. **Build**: Run `msbuild` - packages are automatically installed
3. **Bundle**: Packages are copied to output directory
4. **Deploy**: Single executable with all dependencies included

### ?? Configure Packages

Edit `PythonInline/packaging/requirements.txt`:
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

# Add your packages:
# your_package_name>=1.0.0
```

### ?? Build with Bundled Packages

```cmd
# Automatic bundling during build
msbuild python_inline.vcxproj /p:Configuration=Release

# Manual package bundling (optional)
.\PythonInline\packaging\Install-Packages.ps1 -Force
```

### ?? Use Bundled Packages

```cmd
# HTTP requests (if requests is bundled)
python_inline.exe -c "import requests; r = requests.get('https://httpbin.org/json'); print(r.json())"

# Data analysis (if pandas is bundled)
python_inline.exe -c "import pandas as pd; df = pd.DataFrame({'A': [1,2,3]}); print(df)"

# Web scraping (if beautifulsoup4 is bundled)
python_inline.exe -c "from bs4 import BeautifulSoup; print('BeautifulSoup ready!')"

# List all bundled packages
python_inline.exe --list-packages
```

### ? Key Features

- **?? Automatic**: Packages bundled during build process
- **?? Self-Contained**: No external dependencies needed
- **? Smart Caching**: Only reinstalls when requirements.txt changes
- **?? Optimized**: Removes unnecessary files to minimize size
- **?? Flexible**: Support for version specifications and private repositories

## ??? Usage

### Command Line Interface

The Python Inline Runner supports multiple ways to execute Python code:

#### Execute inline scripts:
```cmd
python-inline -c "print('Hello, World!')"
python-inline --script="import sys; print(sys.version)"
```

#### Execute scripts from files:
```cmd
python-inline --file=script.py
python-inline --file=script.py arg1 arg2
```

#### Package management:
```cmd
python-inline --list-packages                    # List bundled packages
python-inline --packages=requirements.txt --file=script.py  # Load custom packages
python-inline --add-package=requests -c "import requests"   # Add package at runtime
```

#### Additional options:
```cmd
python-inline -h                    # Show help
python-inline --version             # Show Python version
python-inline -q -c "print('Hi')"   # Quiet mode
python-inline -i -c "x = 42"        # Interactive mode after script
```

## ??? Building

### Prerequisites

- **Visual Studio 2019 or later** with C++ build tools
- **Windows SDK 10.0.17763.0 or later**
- **Python 3.8+** (for package installation and base Python build)

### Build Steps

1. **Configure packages to bundle**:
   ```text
   # Edit PythonInline/packaging/requirements.txt
   requests==2.31.0
   pandas>=2.0.0
   beautifulsoup4
   ```

2. **Build with automatic bundling**:
   ```cmd
   msbuild PythonInline\build\python_inline.vcxproj /p:Configuration=Release /p:Platform=x64
   ```
   *Packages are automatically installed during build!*

3. **Build MSIX package with bundled packages**:
   ```powershell
   .\PythonInline\packaging\Build-MSIX.ps1 -Configuration Release -Platform x64
   ```

4. **Test bundled packages**:
   ```cmd
   .\amd64\python_inline.exe --list-packages
   .\amd64\python_inline.exe -c "import requests; print('? Bundled packages work!')"
   ```

### Build Process Features

- **?? Incremental**: Only reinstalls packages when requirements.txt changes
- **? Fast**: Smart timestamp checking avoids unnecessary work
- **??? Safe**: Continues building even if optional packages fail
- **?? Informative**: Clear progress messages during bundling

## ?? Advanced Package Bundling

### Bundle Optimization

```powershell
# Full bundling with dependencies (default)
.\PythonInline\packaging\Install-Packages.ps1

# Minimal bundling without dependencies
.\PythonInline\packaging\Install-Packages.ps1 -NoDeps

# Force reinstall all packages
.\PythonInline\packaging\Install-Packages.ps1 -Force

# Verbose output for debugging
.\PythonInline\packaging\Install-Packages.ps1 -Verbose
```

### Multiple Package Configurations

```powershell
# Production packages
.\Install-Packages.ps1 -RequirementsFile "requirements-prod.txt"

# Development packages  
.\Install-Packages.ps1 -RequirementsFile "requirements-dev.txt"

# Minimal packages
.\Install-Packages.ps1 -RequirementsFile "requirements-minimal.txt"
```

### Custom Python Installation

```powershell
# Use specific Python version for bundling
.\Install-Packages.ps1 -PythonExecutable "C:\Python311\python.exe"
```

## ?? Documentation

Comprehensive documentation available:

- **[BUILD_TIME_BUNDLING.md](PythonInline/docs/BUILD_TIME_BUNDLING.md)** - Complete bundling guide
- **[PACKAGE_SUPPORT.md](PythonInline/docs/PACKAGE_SUPPORT.md)** - Package management documentation  
- **[QUICKSTART.md](PythonInline/docs/QUICKSTART.md)** - Quick start guide
- **[PACKAGE_IMPLEMENTATION.md](PythonInline/docs/PACKAGE_IMPLEMENTATION.md)** - Implementation details

## ?? Testing

### Quick Test

```cmd
# Run the bundling demo
.\PythonInline\scripts\examples\test_bundling.bat
```

### Manual Testing

```cmd
# Test basic functionality
python-inline -c "print('? Basic execution works')"

# Test bundled packages
python-inline --list-packages
python-inline -c "import requests; print('? Bundled packages work')" 

# Test file execution with bundled packages
echo import requests; print("Making request..."); r = requests.get("https://httpbin.org/json"); print(r.json()) > test.py
python-inline --file=test.py
```

### Bundle Verification

```cmd
# Check what's bundled
dir amd64\site-packages

# Test package imports
python-inline -c "import sys; [print(p) for p in sys.path if 'site-packages' in p]"

# Verify specific packages
python-inline -c "import pkg_resources; [print(f'{pkg.key} {pkg.version}') for pkg in pkg_resources.working_set]"
```

## ?? Benefits

### For Developers
- **?? Automatic**: Packages bundled during build - no extra steps
- **??? Reliable**: Consistent package versions across builds
- **? Fast**: Smart caching avoids unnecessary reinstalls  
- **?? Flexible**: Support for complex package requirements

### For Users
- **?? Self-Contained**: Single executable with all dependencies
- **?? Zero Setup**: No Python installation or package management needed
- **? Guaranteed**: All packages available, no import errors
- **?? Portable**: Works on any Windows machine

### For Distribution
- **?? Simple**: Single file deployment
- **??? Secure**: No external downloads at runtime
- **?? Predictable**: Known package versions and dependencies
- **?? Professional**: Clean, polished user experience

## ?? Success Story

**Before**: Users needed Python + pip + package installation
**After**: Single executable with everything bundled!

```cmd
# Everything bundled and ready to go!
python_inline.exe -c "
import requests
import json
r = requests.get('https://api.github.com/repos/python/cpython')
data = r.json()
print(f'? CPython has {data[\"stargazers_count\"]} stars!')
"
```

---

**Built with ?? for Python developers who need portable, self-contained executables with external package support!**
