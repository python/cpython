# Python Inline Runner

A custom Python entry point that can execute inline Python scripts and package as MSIX for Windows distribution.

## Features

- Execute Python code directly from command line using `-c` or `--script=` options
- Execute Python files using `--file=` option
- Pass arguments to Python scripts
- Interactive mode support with `-i` flag
- Quiet mode with `-q` flag
- Full Python standard library support
- Package as MSIX for easy Windows distribution
- Console application aliases (`python-inline`, `pyinline`)

## Building

### Prerequisites

- Visual Studio 2019 or later with C++ development tools
- Windows 10/11 SDK
- CPython source code (this is a fork/modification)

### Build Steps

1. **Build the Python Inline executable:**
   ```powershell
   # Build using MSBuild
   MSBuild pcbuild.sln /p:Configuration=Release /p:Platform=x64 /t:python_inline
   
   # Or using Visual Studio
   # Open pcbuild.sln and build the python_inline project
   ```

2. **Create MSIX package:**
   ```powershell
   # Run the MSIX build script
   .\Build-MSIX.ps1 -Configuration Release -Platform x64
   
   # Or with installation
   .\Build-MSIX.ps1 -Configuration Release -Platform x64 -InstallAfterBuild
   ```

### Build Script Options

```powershell
.\Build-MSIX.ps1 [options]

Parameters:
  -Configuration    Build configuration (Debug, Release) [Default: Release]
  -Platform         Target platform (x86, x64, ARM64) [Default: x64]
  -OutputPath       Output directory for MSIX package [Default: .\MSIX_Output]
  -SkipBuild        Skip building and use existing binaries
  -InstallAfterBuild Install the package after building
```

## Usage

### Command Line Syntax

```
python_inline [OPTIONS] -c "SCRIPT" [ARGS...]
python_inline [OPTIONS] --script="SCRIPT" [ARGS...]
python_inline [OPTIONS] --file=FILENAME [ARGS...]
```

### Options

- `-c "SCRIPT"` - Execute the Python script string
- `--script="SCRIPT"` - Execute the Python script string
- `--file=FILENAME` - Execute Python script from file
- `-h, --help` - Show help message and exit
- `-v, --version` - Show Python version and exit
- `-q, --quiet` - Don't print version and copyright messages
- `-i, --interactive` - Inspect interactively after running script

### Examples

#### Basic Usage

```bash
# Simple Hello World
python_inline -c "print('Hello, World!')"

# Math calculations
python_inline -c "import math; print(f'Pi = {math.pi:.6f}')"

# Multiple statements
python_inline -c "
x = 10
y = 20
print(f'{x} + {y} = {x + y}')
"
```

#### Working with Arguments

```bash
# Pass arguments to script
python_inline -c "import sys; print(f'Args: {sys.argv[1:]}')" arg1 arg2 arg3

# Process command line arguments
python_inline -c "
import sys
if len(sys.argv) > 1:
    print(f'Hello, {sys.argv[1]}!')
else:
    print('Hello, World!')
" "John Doe"
```

#### File Execution

```bash
# Execute a Python file
python_inline --file=script.py

# Execute file with arguments
python_inline --file=script.py arg1 arg2
```

#### Interactive Mode

```bash
# Run script then enter interactive mode
python_inline -i -c "x = 42; print(f'x = {x}')"
```

#### Advanced Examples

```bash
# JSON processing
python_inline --script="import json; data={'name': 'Test'}; print(json.dumps(data, indent=2))"

# Date and time
python_inline -c "from datetime import datetime; print(datetime.now().strftime('%Y-%m-%d %H:%M:%S'))"

# List processing
python_inline -c "numbers = range(1, 11); squares = [x**2 for x in numbers if x % 2 == 0]; print(squares)"

# File operations
python_inline -c "
import os
print(f'Current directory: {os.getcwd()}')
print(f'Files: {os.listdir(\".\")[:5]}')  # Show first 5 files
"
```

## MSIX Package

### Installation

After building the MSIX package:

1. **Install the certificate (for test packages):**
   ```powershell
   # Run as Administrator
   Import-Certificate -FilePath "MSIX_Output\TestCert.pfx" -CertStoreLocation Cert:\LocalMachine\TrustedPeople
   ```

2. **Install the MSIX package:**
   ```powershell
   # Run as Administrator
   Add-AppxPackage -Path "MSIX_Output\PythonInlineRunner_Release_x64.msix"
   ```

3. **Or use the provided install script:**
   ```powershell
   # Run as Administrator
   .\MSIX_Output\Install.ps1
   ```

### Usage After Installation

Once installed, you can use the following aliases:

```bash
# Using the aliases
python-inline -c "print('Hello from MSIX!')"
pyinline --script="import sys; print(sys.version)"

# File association (if configured)
python-inline script.py
```

## Development

### Project Structure

```
??? Programs/
?   ??? python.c              # Original Python entry point
?   ??? python_inline.c       # Custom inline Python entry point
??? PC/
?   ??? python_exe.rc         # Original Python resources
?   ??? python_inline.rc      # Custom entry point resources
??? MSIX/
?   ??? Package.appxmanifest  # MSIX package manifest
??? Examples/
?   ??? usage_examples.bat    # Usage examples
??? python_inline.vcxproj     # Visual Studio project file
??? python_inline.vcxproj.filters # Project filters
??? Build-MSIX.ps1            # MSIX build script
??? test_python_inline.py     # Test suite
??? README.md                 # This file
```

### Testing

Run the test suite to validate functionality:

```powershell
# Build the project first
MSBuild pcbuild.sln /p:Configuration=Release /p:Platform=x64 /t:python_inline

# Run tests
python test_python_inline.py
```

### Customization

You can customize the entry point by modifying:

1. **`Programs/python_inline.c`** - Main logic and command line parsing
2. **`PC/python_inline.rc`** - Version information and resources
3. **`MSIX/Package.appxmanifest`** - MSIX package metadata and capabilities
4. **`Build-MSIX.ps1`** - Build and packaging process

## Technical Details

### How It Works

1. **Command Line Parsing**: The entry point parses command line arguments to determine execution mode
2. **Python Configuration**: Uses PyConfig API to configure the Python interpreter
3. **Script Execution**: Depending on mode, executes inline code or files using Python's run APIs
4. **MSIX Packaging**: Bundles the executable with Python runtime and standard library

### Dependencies

- **Python Core**: Links against pythoncore for interpreter functionality
- **Standard Library**: Includes Python standard library for full compatibility
- **Windows Runtime**: Uses Windows APIs for process management
- **MSIX Runtime**: Requires Windows 10 version 1709 or later for MSIX support

### Security Considerations

- **Full Trust**: The MSIX package runs with full trust (unrestricted)
- **Code Execution**: Can execute arbitrary Python code passed via command line
- **File Access**: Has access to file system within MSIX container constraints
- **Network Access**: Can make network requests if Python modules support it

## Troubleshooting

### Common Issues

1. **Build Errors**:
   - Ensure Visual Studio 2019+ with C++ tools installed
   - Check that pythoncore project builds successfully first
   - Verify Windows SDK is installed

2. **MSIX Creation Errors**:
   - Install Windows 10/11 SDK
   - Run PowerShell as Administrator for signing
   - Check that all required files are copied to package directory

3. **Installation Issues**:
   - Install test certificate before installing package
   - Use PowerShell as Administrator
   - Enable Developer Mode in Windows Settings

4. **Runtime Errors**:
   - Ensure Python standard library is included in package
   - Check that all required DLLs are present
   - Verify Python path configuration

### Debug Mode

Build in Debug configuration for detailed error information:

```powershell
.\Build-MSIX.ps1 -Configuration Debug -Platform x64
```

## License

This project follows the same license as CPython. See the CPython license for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## Changelog

### Version 1.0.0
- Initial release
- Basic inline script execution
- File execution support
- MSIX packaging
- Command line argument processing
- Interactive mode support
