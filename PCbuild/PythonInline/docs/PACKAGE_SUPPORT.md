# Python Inline External Package Support

## ?? Overview

Python Inline now supports bundling and using external Python packages! This allows you to create self-contained Python executables that include popular packages like `requests`, `numpy`, `pandas`, and more.

## ?? How It Works

1. **Build Time**: External packages are installed and bundled into the MSIX package
2. **Runtime**: Python Inline automatically configures the Python path to find bundled packages
3. **Usage**: Your scripts can import and use the packages as if they were installed system-wide

## ??? Configuration

### Requirements File

Create or edit `PythonInline/packaging/requirements.txt` to specify which packages to bundle:

```text
# Popular packages for bundling
requests==2.31.0
numpy>=1.24.0
pandas>=2.0.0
beautifulsoup4>=4.12.0
openpyxl>=3.1.0
python-dateutil>=2.8.0
```

### Package Format

- **Exact version**: `requests==2.31.0`
- **Minimum version**: `numpy>=1.24.0`
- **Latest version**: `pandas` (no version specified)
- **Comments**: Lines starting with `#` are ignored

## ??? Building with Packages

### Automatic Package Installation

The build process automatically installs packages:

```powershell
# Build MSIX with external packages
.\PythonInline\packaging\Build-MSIX.ps1

# Skip package installation (use previously installed)
.\PythonInline\packaging\Build-MSIX.ps1 -SkipPackages
```

### Manual Package Installation

You can also install packages separately:

```powershell
# Install packages to site-packages directory
.\PythonInline\packaging\Install-Packages.ps1 -RequirementsFile "requirements.txt" -Force
```

## ?? Runtime Usage

### Command Line Options

```cmd
# List configured packages
python-inline --list-packages

# Load packages from custom config file
python-inline --packages=my-requirements.txt --file=script.py

# Add a package at runtime (if available)
python-inline --add-package=requests -c "import requests; print('OK')"
```

### Example Scripts

#### Basic HTTP Request
```cmd
python-inline -c "import requests; r = requests.get('https://httpbin.org/json'); print(r.json())"
```

#### Data Analysis with Pandas
```cmd
python-inline -c "import pandas as pd; df = pd.DataFrame({'A': [1,2,3], 'B': [4,5,6]}); print(df.head())"
```

#### Web Scraping
```cmd
python-inline -c "from bs4 import BeautifulSoup; import requests; r = requests.get('https://httpbin.org/html'); soup = BeautifulSoup(r.text, 'html.parser'); print(soup.title.text)"
```

#### Excel File Processing
```cmd
python-inline --file=process_excel.py input.xlsx output.xlsx
```

Where `process_excel.py` contains:
```python
import sys
import openpyxl

input_file = sys.argv[1]
output_file = sys.argv[2]

wb = openpyxl.load_workbook(input_file)
ws = wb.active
# Process data...
wb.save(output_file)
print(f"Processed {input_file} -> {output_file}")
```

## ?? Advanced Configuration

### Custom Package Configuration

Create a custom requirements file:

```text
# my-packages.txt
requests==2.31.0
click>=8.1.0
pyyaml>=6.0
```

Use it at runtime:
```cmd
python-inline --packages=my-packages.txt --file=my-script.py
```

### Package Loading Priority

Python Inline sets up the package search path in this order:
1. **Bundled site-packages** (highest priority)
2. **System Python packages** (fallback)
3. **Built-in modules** (standard library)

### Troubleshooting Package Issues

#### Check Available Packages
```cmd
python-inline --list-packages
```

#### Test Package Import
```cmd
python-inline -c "import requests; print(f'Requests version: {requests.__version__}')"
```

#### Verbose Package Information
```cmd
python-inline -c "import sys; print('Python path:'); [print(f'  {p}') for p in sys.path]"
```

## ?? Supported Package Categories

### ? Well-Supported Packages

- **HTTP/Web**: `requests`, `urllib3`, `httpx`
- **Data Analysis**: `pandas`, `numpy` (basic operations)
- **Web Scraping**: `beautifulsoup4`, `lxml`
- **File Formats**: `openpyxl`, `xlsxwriter`, `PyYAML`
- **Date/Time**: `python-dateutil`, `pytz`
- **Text Processing**: `markdown`, `jinja2`
- **CLI Tools**: `click`, `argparse` (built-in)
- **Configuration**: `configparser` (built-in), `PyYAML`

### ?? Limited Support

- **GUI Libraries**: `tkinter` (built-in only), avoid `PyQt`, `wxPython`
- **Heavy Scientific**: Large numpy operations, `scipy`, `matplotlib`
- **Database**: Simple `sqlite3` (built-in), avoid heavy database drivers
- **Network**: Basic requests, avoid complex networking libraries

### ? Not Recommended

- **System-Dependent**: Packages requiring system libraries
- **Compiled Extensions**: Complex C extensions that need compilation
- **Large Dependencies**: Packages with huge dependency trees
- **GPU Libraries**: `tensorflow`, `pytorch`, `cupy`

## ?? Best Practices

### Package Selection

1. **Keep it lightweight**: Choose packages with minimal dependencies
2. **Version pinning**: Use exact versions for reproducible builds
3. **Test thoroughly**: Verify packages work in the bundled environment
4. **Document requirements**: Comment your requirements.txt file

### Script Development

1. **Handle import errors gracefully**:
   ```python
   try:
       import requests
   except ImportError:
       print("requests package not available")
       sys.exit(1)
   ```

2. **Check package versions**:
   ```python
   import requests
   print(f"Using requests version: {requests.__version__}")
   ```

3. **Use built-in alternatives when possible**:
   ```python
   # Prefer urllib (built-in) for simple HTTP requests
   import urllib.request
   
   # Use json (built-in) for JSON processing
   import json
   ```

### Build Optimization

1. **Minimize package count**: Only include packages you actually use
2. **Use --SkipPackages**: Skip reinstallation during development
3. **Clean builds**: Use `-Force` to ensure fresh package installation
4. **Test package imports**: Verify all packages work before building MSIX

## ?? Example Workflows

### Data Processing Pipeline

1. **Create requirements.txt**:
   ```text
   pandas==2.1.0
   openpyxl==3.1.2
   requests==2.31.0
   ```

2. **Build and install**:
   ```powershell
   .\PythonInline\packaging\Build-MSIX.ps1 -InstallAfterBuild
   ```

3. **Use in scripts**:
   ```cmd
   python-inline --file=data-pipeline.py input.xlsx
   ```

### Web Scraping Tool

1. **Configure packages**:
   ```text
   requests==2.31.0
   beautifulsoup4==4.12.2
   lxml==4.9.3
   ```

2. **Create scraper script**:
   ```python
   import requests
   from bs4 import BeautifulSoup
   import sys
   
   url = sys.argv[1]
   response = requests.get(url)
   soup = BeautifulSoup(response.content, 'html.parser')
   
   # Extract data and process...
   ```

3. **Run scraper**:
   ```cmd
   python-inline --file=scraper.py https://example.com
   ```

## ?? Package Manifest

After building, Python Inline creates a package manifest at:
`site-packages/python_inline_packages.txt`

This file contains:
- List of successfully installed packages
- Installation timestamp
- Python version used
- Failed packages (if any)

---

**With external package support, Python Inline becomes a powerful tool for creating portable, self-contained Python applications! ??**
