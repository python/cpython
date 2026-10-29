"""Utilities for extracting module names from file paths."""

import os
import site
import sys
from pathlib import Path


def get_python_path_info():
    """Get information about Python's search paths.

    Returns:
        dict: Dictionary containing stdlib path, site-packages paths, and sys.path entries.
    """
    info = {
        'stdlib': None,
        'site_packages': [],
        'sys_path': []
    }

    # Get standard library path from os module location
    try:
        if hasattr(os, '__file__') and os.__file__:
            info['stdlib'] = Path(os.__file__).parent
    except (AttributeError, OSError):
        pass  # Silently continue if we can't determine stdlib path

    # Get site-packages directories
    site_packages = []
    try:
        site_packages.extend(Path(p) for p in site.getsitepackages())
    except (AttributeError, OSError):
        pass  # Continue without site packages if unavailable

    # Get user site-packages
    try:
        user_site = site.getusersitepackages()
        if user_site and Path(user_site).exists():
            site_packages.append(Path(user_site))
    except (AttributeError, OSError):
        pass  # Continue without user site packages

    info['site_packages'] = site_packages
    info['sys_path'] = [Path(p) for p in sys.path if p]

    return info


def extract_module_name(filename, path_info):
    """Extract Python module name and type from file path.

    Args:
        filename: Path to the Python file
        path_info: Dictionary from get_python_path_info()

    Returns:
        tuple: (module_name, module_type) where module_type is one of:
               'stdlib', 'site-packages', 'project', or 'other'
    """
    if not filename:
        return ('unknown', 'other')

    try:
        file_path = Path(filename)
    except (ValueError, OSError):
        return (str(filename), 'other')

    # Check if it's in stdlib
    if path_info['stdlib'] and file_path.is_relative_to(path_info['stdlib']):
        return (_path_to_module(file_path.relative_to(path_info['stdlib'])), 'stdlib')

    # Check site-packages
    for site_pkg in path_info['site_packages']:
        if file_path.is_relative_to(site_pkg):
            return (_path_to_module(file_path.relative_to(site_pkg)), 'site-packages')

    # Check other sys.path entries (project files)
    if not str(file_path).startswith(('<', '[')):  # Skip special files
        for path_entry in path_info['sys_path']:
            if file_path.is_relative_to(path_entry):
                return (_path_to_module(file_path.relative_to(path_entry)), 'project')

    # Fallback: just use the filename
    return (_path_to_module(file_path), 'other')


def _path_to_module(path):
    if isinstance(path, str):
        path = Path(path)

    # Remove .py extension
    if path.suffix == '.py':
        path = path.with_suffix('')

    # Convert path separators to dots, stripping root/drive (e.g. "/" or "C:\")
    parts = [p for p in path.parts if p != path.root and p != path.drive]

    # Handle __init__ files - they represent the package itself
    if parts and parts[-1] == '__init__':
        parts = parts[:-1]

    return '.'.join(parts) if parts else path.stem
