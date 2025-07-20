"""
Test suite for zipfile validation features.
"""

import io
import os
import struct
import tempfile
import unittest
import zipfile
from zipfile import (
    ZipFile, ZipValidationLevel, ZipStructuralError, ZipValidationError,
    BadZipFile, sizeEndCentDir, stringEndArchive, structEndArchive,
    sizeCentralDir, stringCentralDir, structCentralDir,
    sizeFileHeader, stringFileHeader, structFileHeader,
    _ECD_ENTRIES_TOTAL, _ECD_SIZE, _ECD_OFFSET, _ECD_COMMENT_SIZE
)
from test.support.os_helper import TESTFN, unlink


class TestZipValidation(unittest.TestCase):
    """Test zipfile validation functionality."""

    def setUp(self):
        """Set up test fixtures."""
        self.temp_files = []

    def tearDown(self):
        """Clean up test fixtures."""
        for temp_file in self.temp_files:
            try:
                unlink(temp_file)
            except OSError:
                pass

    def create_temp_file(self, content=b''):
        """Create a temporary file with given content."""
        fd, path = tempfile.mkstemp()
        self.temp_files.append(path)
        with os.fdopen(fd, 'wb') as f:
            f.write(content)
        return path

    def test_basic_validation_backward_compatibility(self):
        """Test that basic validation mode maintains backward compatibility."""
        # Create a valid ZIP file
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            zf.writestr('test.txt', 'Hello, World!')

        # Test default behavior (should be BASIC validation)
        with ZipFile(temp_path, 'r') as zf:
            self.assertEqual(zf._strict_validation, ZipValidationLevel.BASIC)
            self.assertEqual(zf.read('test.txt'), b'Hello, World!')

        # Test explicit BASIC validation
        with ZipFile(temp_path, 'r', strict_validation=ZipValidationLevel.BASIC) as zf:
            self.assertEqual(zf._strict_validation, ZipValidationLevel.BASIC)
            self.assertEqual(zf.read('test.txt'), b'Hello, World!')

    def test_validation_level_enum(self):
        """Test validation level enum values."""
        self.assertEqual(ZipValidationLevel.BASIC, 0)
        self.assertEqual(ZipValidationLevel.STRUCTURAL, 1)
        self.assertEqual(ZipValidationLevel.STRICT, 2)

        # Test enum conversion
        self.assertEqual(ZipValidationLevel(0), ZipValidationLevel.BASIC)
        self.assertEqual(ZipValidationLevel(1), ZipValidationLevel.STRUCTURAL)
        self.assertEqual(ZipValidationLevel(2), ZipValidationLevel.STRICT)

    def test_structural_validation_valid_file(self):
        """Test structural validation with a valid ZIP file."""
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            zf.writestr('test.txt', 'Hello, World!')
            zf.writestr('dir/nested.txt', 'Nested content')

        # Should pass structural validation
        with ZipFile(temp_path, 'r', strict_validation=ZipValidationLevel.STRUCTURAL) as zf:
            self.assertEqual(len(zf.filelist), 2)
            self.assertEqual(zf.read('test.txt'), b'Hello, World!')
            self.assertEqual(zf.read('dir/nested.txt'), b'Nested content')

    def test_strict_validation_valid_file(self):
        """Test strict validation with a valid ZIP file."""
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            zf.writestr('test.txt', 'Hello, World!')

        # Should pass strict validation
        with ZipFile(temp_path, 'r', strict_validation=ZipValidationLevel.STRICT) as zf:
            self.assertEqual(zf.read('test.txt'), b'Hello, World!')

    def test_malformed_eocd_too_many_entries(self):
        """Test detection of EOCD with too many entries."""
        # Create a basic ZIP file first
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            zf.writestr('test.txt', 'Hello')

        # Read the file and modify the EOCD to claim too many entries
        with open(temp_path, 'rb') as f:
            data = bytearray(f.read())

        # Find EOCD signature and modify entry count
        eocd_pos = data.rfind(stringEndArchive)
        if eocd_pos >= 0:
            # Modify total entries field to exceed limit (65535 is max for H format)
            struct.pack_into('<H', data, eocd_pos + 10, 65535)  # _ECD_ENTRIES_TOTAL offset is 10

        malformed_path = self.create_temp_file(data)

        # Should fail with structural validation - will catch entry count mismatch first
        with self.assertRaises(ZipStructuralError) as cm:
            with ZipFile(malformed_path, 'r', strict_validation=ZipValidationLevel.STRUCTURAL):
                pass
        # Could be either "Too many entries" or "Entry count mismatch" depending on which check runs first
        error_msg = str(cm.exception)
        self.assertTrue("Too many entries" in error_msg or "Entry count mismatch" in error_msg)

        # Should pass with basic validation (backward compatibility)
        with ZipFile(malformed_path, 'r', strict_validation=ZipValidationLevel.BASIC):
            pass

    def test_exception_hierarchy(self):
        """Test that new exceptions are subclasses of BadZipFile."""
        self.assertTrue(issubclass(ZipStructuralError, BadZipFile))
        self.assertTrue(issubclass(ZipValidationError, BadZipFile))

        # Test exception creation
        exc1 = ZipStructuralError("Structure error")
        exc2 = ZipValidationError("Validation error")

        self.assertIsInstance(exc1, BadZipFile)
        self.assertIsInstance(exc2, BadZipFile)

    def test_compression_ratio_detection(self):
        """Test detection of suspicious compression ratios."""
        # This is a simplified test - creating an actual zip bomb would be complex
        # Instead we'll test the validation logic directly
        from zipfile import _validate_zipinfo_fields, ZipInfo

        zinfo = ZipInfo('test.txt')
        zinfo.compress_size = 1  # 1 byte compressed
        zinfo.file_size = 2000  # 2000 bytes uncompressed (ratio = 2000)
        zinfo.header_offset = 0
        zinfo.compress_type = zipfile.ZIP_DEFLATED

        # Should trigger zip bomb detection with ratio > 1000
        with self.assertRaises(ZipStructuralError) as cm:
            _validate_zipinfo_fields(zinfo, ZipValidationLevel.STRUCTURAL)
        self.assertIn("Suspicious compression ratio", str(cm.exception))

    def test_constructor_parameter_validation(self):
        """Test validation of constructor parameters."""
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            zf.writestr('test.txt', 'Hello')

        # Test invalid validation level
        with self.assertRaises(ValueError):
            ZipFile(temp_path, 'r', strict_validation=99)

        # Test valid validation levels
        for level in [ZipValidationLevel.BASIC, ZipValidationLevel.STRUCTURAL, ZipValidationLevel.STRICT]:
            with ZipFile(temp_path, 'r', strict_validation=level) as zf:
                self.assertEqual(zf._strict_validation, level)


class TestValidationIntegration(unittest.TestCase):
    """Test integration of validation with existing zipfile functionality."""

    def setUp(self):
        self.temp_files = []

    def tearDown(self):
        for temp_file in self.temp_files:
            try:
                unlink(temp_file)
            except OSError:
                pass

    def create_temp_file(self, content=b''):
        fd, path = tempfile.mkstemp()
        self.temp_files.append(path)
        with os.fdopen(fd, 'wb') as f:
            f.write(content)
        return path

    def test_existing_methods_work_with_validation(self):
        """Test that existing ZipFile methods work with validation enabled."""
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            zf.writestr('test1.txt', 'Content 1')
            zf.writestr('test2.txt', 'Content 2')

        with ZipFile(temp_path, 'r', strict_validation=ZipValidationLevel.STRUCTURAL) as zf:
            # Test namelist
            names = zf.namelist()
            self.assertEqual(set(names), {'test1.txt', 'test2.txt'})

            # Test infolist
            infos = zf.infolist()
            self.assertEqual(len(infos), 2)

            # Test getinfo
            info = zf.getinfo('test1.txt')
            self.assertEqual(info.filename, 'test1.txt')

            # Test read
            content = zf.read('test1.txt')
            self.assertEqual(content, b'Content 1')

            # Test testzip
            result = zf.testzip()
            self.assertIsNone(result)  # No errors

    def test_validation_with_different_compression_methods(self):
        """Test validation works with different compression methods."""
        temp_path = self.create_temp_file()
        with ZipFile(temp_path, 'w') as zf:
            # Test different compression methods
            zf.writestr('stored.txt', 'Stored content', compress_type=zipfile.ZIP_STORED)
            try:
                import zlib
                zf.writestr('deflated.txt', 'Deflated content', compress_type=zipfile.ZIP_DEFLATED)
                has_zlib = True
            except ImportError:
                has_zlib = False

        # Should work with structural validation
        with ZipFile(temp_path, 'r', strict_validation=ZipValidationLevel.STRUCTURAL) as zf:
            self.assertEqual(zf.read('stored.txt'), b'Stored content')
            if has_zlib:
                self.assertEqual(zf.read('deflated.txt'), b'Deflated content')


if __name__ == '__main__':
    unittest.main()
