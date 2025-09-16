# Quick Start Guide - Python Inline Runner

## TL;DR - Get Started in 5 Minutes

### 1. Build the Custom Entry Point

```powershell
# Navigate to PCbuild directory
cd PCbuild

# Build the python_inline project
MSBuild pcbuild.sln /p:Configuration=Release /p:Platform=x64 /t:python_inline
```

### 2. Test It Works

```powershell
# Test basic functionality
.\x64\python_inline.exe -c "print('Hello, World!')"

# Test with arguments
.\x64\python_inline.exe -c "import sys; print(f'Args: {sys.argv[1:]}')" test1 test2
```

### 3. Create MSIX Package

```powershell
# Run the automated build script
.\Build-MSIX.ps1 -Configuration Release -Platform x64

# Or build and install in one step
.\Build-MSIX.ps1 -Configuration Release -Platform x64 -InstallAfterBuild
```

### 4. Install and Use

```powershell
# Install (run as Administrator)
.\MSIX_Output\Install.ps1

# Use the installed app
python-inline -c "print('Hello from MSIX!')"
pyinline --script="import datetime; print(datetime.datetime.now())"
```

## Common Use Cases

### 1. Quick Calculations
```bash
python-inline -c "print(f'2^10 = {2**10}')"
python-inline -c "import math; print(f'sin(?/2) = {math.sin(math.pi/2)}')"
```

### 2. String Processing
```bash
python-inline -c "text='Hello World'; print(text.upper().replace(' ', '_'))"
```

### 3. JSON Processing
```bash
python-inline -c "import json; print(json.dumps({'name': 'test', 'value': 42}, indent=2))"
```

### 4. File Operations
```bash
python-inline -c "import os; print('\\n'.join(os.listdir('.')))"
```

### 5. Date/Time Operations
```bash
python-inline -c "from datetime import datetime; print(datetime.now().strftime('%Y-%m-%d %H:%M:%S'))"
```

### 6. Running Scripts with Arguments
```bash
# Create a simple script file
echo "import sys; print(f'Hello {sys.argv[1] if len(sys.argv) > 1 else \"World\"}!')" > hello.py

# Run it
python-inline --file=hello.py "Python User"
```

## What Makes This Special?

1. **Self-Contained**: Everything Python needs is in the MSIX package
2. **No Installation Required**: MSIX handles all dependencies
3. **Windows Store Ready**: Can be distributed through Microsoft Store
4. **Command Line Friendly**: Works great in PowerShell, CMD, and Windows Terminal
5. **Aliases**: Use `python-inline` or `pyinline` - whichever you prefer

## Next Steps

- Check out [Examples/usage_examples.bat](Examples/usage_examples.bat) for more examples
- Read the full [README.md](README.md) for advanced usage
- Run `python test_python_inline.py` to validate your build
- Customize the entry point in `Programs/python_inline.c` for your needs

## Troubleshooting

**Build fails?**
- Make sure Visual Studio 2019+ with C++ tools is installed
- Ensure Windows 10/11 SDK is available

**MSIX creation fails?**
- Run PowerShell as Administrator
- Install Windows SDK if missing

**Can't install MSIX?**
- Enable Developer Mode in Windows Settings
- Install the test certificate first

**Runtime errors?**
- Check that Python standard library is included
- Verify all DLLs are in the package directory

Happy coding! ??
