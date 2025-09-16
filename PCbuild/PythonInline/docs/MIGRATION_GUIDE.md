# Migration Guide: New PythonInline Folder Structure

This guide helps you transition from the old scattered file organization to the new structured PythonInline directory.

## ?? What Changed

### Before (Old Structure)
```
PCbuild/
??? python_inline.vcxproj         # Mixed with other projects
??? Build-MSIX.ps1                # Mixed with build files
??? Install-Certificate.ps1       
??? Test-MSIX-Installation.ps1    
??? PythonInline.psm1             
??? test_python_inline.py         
??? README.md                     # Mixed documentation
??? QUICKSTART.md                 
??? Examples/usage_examples.bat   
??? MSIX/Package.appxmanifest     
??? MSIX_Output/                  

Programs/
??? python_inline.c               # Mixed with CPython sources
??? python_inline_*.c/.h          
??? README_STRUCTURE.md           
```

### After (New Structure)
```
PCbuild/PythonInline/             # Self-contained project
??? src/                          # All source code
??? build/                        # Build system files
??? packaging/                    # MSIX and distribution
??? scripts/                      # Utility scripts
??? tests/                        # Testing files
??? docs/                         # All documentation
??? output/                       # Build outputs
```

## ?? Quick Migration Steps

### For Existing Builds

1. **Update build commands:**
   ```powershell
   # OLD:
   msbuild python_inline.vcxproj
   
   # NEW:
   msbuild PythonInline\build\python_inline.vcxproj
   ```

2. **Update MSIX build:**
   ```powershell
   # OLD:
   .\Build-MSIX.ps1
   
   # NEW:
   .\PythonInline\packaging\Build-MSIX.ps1
   ```

3. **Update paths in scripts:**
   - Change any hardcoded paths to use the new structure
   - Update include directories in custom build scripts

### For Development

1. **Source file locations:**
   - All `.c` and `.h` files are now in `PythonInline\src\`
   - Use relative paths from the build directory

2. **Build configuration:**
   - Project files are in `PythonInline\build\`
   - Include paths updated automatically

3. **Documentation:**
   - All docs consolidated in `PythonInline\docs\`
   - Main README is `PythonInline\README.md`

## ?? Breaking Changes

### File Paths
| Old Location | New Location |
|--------------|--------------|
| `python_inline.vcxproj` | `PythonInline\build\python_inline.vcxproj` |
| `Build-MSIX.ps1` | `PythonInline\packaging\Build-MSIX.ps1` |
| `MSIX\Package.appxmanifest` | `PythonInline\packaging\msix\Package.appxmanifest` |
| `Examples\usage_examples.bat` | `PythonInline\scripts\examples\usage_examples.bat` |
| `test_python_inline.py` | `PythonInline\tests\test_python_inline.py` |
| `README.md` | `PythonInline\docs\README.md` (old) ? `PythonInline\README.md` (main) |

### Include Paths
```cpp
// OLD:
#include "../Programs/python_inline_config.h"

// NEW:
#include "python_inline_config.h"  // Now in same directory
```

### Build Scripts
```powershell
# OLD build script references:
$ProjectPath = "python_inline.vcxproj"
$MSIXPath = "MSIX\Package.appxmanifest"

# NEW build script references:
$ProjectPath = "PythonInline\build\python_inline.vcxproj"  
$MSIXPath = "PythonInline\packaging\msix\Package.appxmanifest"
```

## ? Migration Checklist

### For Developers
- [ ] Update IDE project references to new locations
- [ ] Update any custom build scripts with new paths
- [ ] Check that include paths work correctly
- [ ] Test build process with new structure
- [ ] Update documentation references

### For CI/CD Systems
- [ ] Update build pipeline paths
- [ ] Update artifact collection paths
- [ ] Update deployment scripts
- [ ] Test full build and package process

### For Users
- [ ] Update any shortcuts or scripts that reference old paths
- [ ] Re-run MSIX build process
- [ ] Verify all functionality works as expected

## ?? Backward Compatibility

### What Still Works
- **Command-line interface**: All commands work identically
- **MSIX package functionality**: Same features and capabilities
- **PowerShell module**: Same functions and behavior

### What Requires Updates
- **Build processes**: Need to use new paths
- **Custom scripts**: Need path updates
- **Development workflows**: Need to use new structure

## ??? Tools and Helpers

### Path Converter Script
```powershell
# Helper function to convert old paths to new structure
function Convert-PythonInlinePath {
    param([string]$OldPath)
    
    $conversions = @{
        "python_inline.vcxproj" = "PythonInline\build\python_inline.vcxproj"
        "Build-MSIX.ps1" = "PythonInline\packaging\Build-MSIX.ps1"
        "MSIX\Package.appxmanifest" = "PythonInline\packaging\msix\Package.appxmanifest"
        "test_python_inline.py" = "PythonInline\tests\test_python_inline.py"
        "Examples\usage_examples.bat" = "PythonInline\scripts\examples\usage_examples.bat"
    }
    
    return $conversions[$OldPath] ?? $OldPath
}
```

### Build Validation
```powershell
# Test that new structure works
.\PythonInline\packaging\Build-MSIX.ps1 -SkipBuild
if ($LASTEXITCODE -eq 0) {
    Write-Host "? New structure validated successfully" -ForegroundColor Green
} else {
    Write-Host "? Migration incomplete - check paths" -ForegroundColor Red
}
```

## ?? Getting Help

If you encounter issues during migration:

1. **Check the structure**: Verify all files are in the expected new locations
2. **Review the logs**: Build and package scripts provide detailed output
3. **Test incrementally**: Build, then package, then install step by step
4. **Consult documentation**: Check `PythonInline\docs\` for detailed guides

## ?? Benefits of New Structure

After migration, you'll enjoy:

- **Better Organization**: Clear separation of concerns
- **Easier Development**: All related files in one place
- **Simpler Building**: Self-contained build process
- **Professional Layout**: Industry-standard project structure
- **Enhanced Maintainability**: Easier to find and modify components

---

**The new structure makes Python Inline development more professional and maintainable!**
