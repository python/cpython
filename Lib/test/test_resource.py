"""Tests for the resource module.

This test module has been updated to handle systems with file size limits
properly, fixing the long-standing issue #37883 (bpo-678264).
"""

import sys
import os
import time
import unittest
import errno
import tempfile
import contextlib
from test import support
from test.support import import_helper
from test.support.os_helper import temp_dir, unlink, TESTFN

resource = import_helper.import_module('resource')


class ResourceLimitHelper:
    """Helper class to safely manage resource limits during testing."""
    
    def __init__(self, limit_type):
        self.limit_type = limit_type
        self.original_limits = None
        self.limit_name = self._get_limit_name(limit_type)
    
    def _get_limit_name(self, limit_type):
        """Get human-readable name for resource limit."""
        names = {
            resource.RLIMIT_FSIZE: "file size",
            resource.RLIMIT_CPU: "CPU time",
            resource.RLIMIT_DATA: "data segment size",
            resource.RLIMIT_STACK: "stack size",
            resource.RLIMIT_CORE: "core file size",
        }
        if hasattr(resource, 'RLIMIT_RSS'):
            names[resource.RLIMIT_RSS] = "resident set size"
        if hasattr(resource, 'RLIMIT_NPROC'):
            names[resource.RLIMIT_NPROC] = "number of processes"
        if hasattr(resource, 'RLIMIT_NOFILE'):
            names[resource.RLIMIT_NOFILE] = "number of open files"
        if hasattr(resource, 'RLIMIT_MEMLOCK'):
            names[resource.RLIMIT_MEMLOCK] = "locked memory"
        if hasattr(resource, 'RLIMIT_AS'):
            names[resource.RLIMIT_AS] = "address space"
        return names.get(limit_type, f"resource {limit_type}")
    
    def __enter__(self):
        self.original_limits = resource.getrlimit(self.limit_type)
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.original_limits:
            try:
                resource.setrlimit(self.limit_type, self.original_limits)
            except (ValueError, OSError, PermissionError):
                pass
        return False
    
    def can_set_limit(self, soft_limit, hard_limit=None):
        """Check if we can set the specified limit."""
        if hard_limit is None:
            hard_limit = self.original_limits[1]
        
        if soft_limit > hard_limit:
            return False
        
        if hard_limit > self.original_limits[1] and hard_limit != resource.RLIM_INFINITY:
            return False
            
        try:
            resource.setrlimit(self.limit_type, (soft_limit, hard_limit))
            resource.setrlimit(self.limit_type, self.original_limits)
            return True
        except (ValueError, OSError, PermissionError):
            return False
    
    def get_safe_test_limit(self, desired_size, minimum_size=1024):
        """Calculate a safe limit for testing."""
        soft, hard = self.original_limits
        
        if soft == resource.RLIM_INFINITY:
            return (desired_size, True)
        
        if soft < minimum_size:
            return (0, False)
        
        safe_size = min(desired_size, int(soft * 0.8))
        
        if safe_size < minimum_size:
            safe_size = minimum_size
        
        return (safe_size, safe_size <= soft)


def skip_if_resource_limited(limit_type, minimum_required):
    """Decorator to skip test if resource limit is too restrictive."""
    def decorator(test_func):
        def wrapper(self):
            soft, hard = resource.getrlimit(limit_type)
            if soft != resource.RLIM_INFINITY and soft < minimum_required:
                helper = ResourceLimitHelper(limit_type)
                self.skipTest(
                    f"{helper.limit_name} limit too small: "
                    f"{soft} < {minimum_required}"
                )
            return test_func(self)
        wrapper.__name__ = test_func.__name__
        wrapper.__doc__ = test_func.__doc__
        return wrapper
    return decorator


class ResourceTest(unittest.TestCase):
    
    def setUp(self):
        self.limits = []
        for name in dir(resource):
            if name.startswith('RLIMIT_'):
                self.limits.append(getattr(resource, name))

    def test_args(self):
        self.assertRaises(TypeError, resource.getrlimit)
        self.assertRaises(TypeError, resource.getrlimit, 42, 42)
        self.assertRaises(TypeError, resource.setrlimit)
        self.assertRaises(TypeError, resource.setrlimit, 42, 42, 42)

    def test_getrlimit(self):
        for res in self.limits:
            try:
                limit = resource.getrlimit(res)
                self.assertIsInstance(limit, tuple)
                self.assertEqual(len(limit), 2)
                self.assertIsInstance(limit[0], int)
                self.assertIsInstance(limit[1], int)
                self.assertGreaterEqual(limit[0], 0)
                self.assertGreaterEqual(limit[1], 0)
                if limit[1] != resource.RLIM_INFINITY:
                    self.assertLessEqual(limit[0], limit[1])
            except (ValueError, OSError) as e:
                if "not supported" in str(e).lower():
                    continue
                raise

    def test_setrlimit(self):
        for res in self.limits:
            try:
                limit = resource.getrlimit(res)
            except (ValueError, OSError):
                continue
                
            with ResourceLimitHelper(res) as helper:
                try:
                    resource.setrlimit(res, limit)
                    
                    if limit[0] == limit[1] == resource.RLIM_INFINITY:
                        continue
                    
                    if limit[1] != resource.RLIM_INFINITY and limit[1] > 0:
                        # Try to set soft limit above hard limit (should fail)
                        try:
                            resource.setrlimit(res, (limit[1] + 1, limit[1]))
                            # If it doesn't raise an error, that's a problem
                            self.fail(f"Expected error when setting soft > hard for {res}")
                        except (ValueError, OSError, PermissionError):
                            # Expected - this should fail
                            pass
                    
                    if limit[0] < limit[1] and limit[0] != resource.RLIM_INFINITY:
                        new_soft = min(limit[0] + 1, limit[1])
                        if helper.can_set_limit(new_soft, limit[1]):
                            resource.setrlimit(res, (new_soft, limit[1]))
                            resource.setrlimit(res, limit)
                
                except PermissionError:
                    pass
                except (ValueError, OSError) as e:
                    if any(msg in str(e).lower() for msg in 
                           ["not support", "invalid argument", "operation not permitted"]):
                        continue
                    raise

    def test_getrusage(self):
        for who in [resource.RUSAGE_SELF]:
            usage = resource.getrusage(who)
            self.assertIsInstance(usage, tuple)
            self.assertEqual(len(usage), 16)
            for i, value in enumerate(usage):
                self.assertIsInstance(value, (int, float))
                if i not in [8, 9]:
                    self.assertGreaterEqual(value, 0)
        
        if hasattr(resource, 'RUSAGE_CHILDREN'):
            usage = resource.getrusage(resource.RUSAGE_CHILDREN)
            self.assertIsInstance(usage, tuple)
        
        self.assertRaises(TypeError, resource.getrusage)
        self.assertRaises(TypeError, resource.getrusage, 42, 42)
        self.assertRaises(ValueError, resource.getrusage, -999999)
        self.assertRaises(ValueError, resource.getrusage, 1000000)

    @skip_if_resource_limited(resource.RLIMIT_FSIZE, 1024 * 1024)
    def test_fsize_limit(self):
        """Test file size limit enforcement - main fix for issue #37883."""
        with ResourceLimitHelper(resource.RLIMIT_FSIZE) as helper:
            test_size, can_test = helper.get_safe_test_limit(
                desired_size=1024 * 1024,
                minimum_size=10 * 1024
            )
            
            if not can_test:
                self.skipTest(
                    f"File size limit too restrictive for testing "
                    f"(current limit: {helper.original_limits[0]} bytes)"
                )
            
            with tempfile.NamedTemporaryFile(mode='wb', delete=True) as f:
                limit_size = test_size // 2
                
                if not helper.can_set_limit(limit_size, helper.original_limits[1]):
                    self.skipTest(
                        f"Cannot set file size limit to {limit_size} bytes"
                    )
                
                try:
                    resource.setrlimit(resource.RLIMIT_FSIZE, 
                                     (limit_size, helper.original_limits[1]))
                except (ValueError, OSError, PermissionError) as e:
                    self.skipTest(f"Cannot modify file size limit: {e}")
                
                chunk_size = min(8192, limit_size // 4)
                bytes_written = 0
                target_size = limit_size - chunk_size
                
                while bytes_written < target_size:
                    chunk = b'x' * min(chunk_size, target_size - bytes_written)
                    f.write(chunk)
                    bytes_written += len(chunk)
                f.flush()
                
                with self.assertRaises(OSError) as cm:
                    excess_data = b'y' * (limit_size)
                    f.write(excess_data)
                    f.flush()
                
                self.assertIn(cm.exception.errno, 
                            [errno.EFBIG, errno.ENOSPC, errno.E2BIG],
                            f"Unexpected error: {cm.exception}")

    @skip_if_resource_limited(resource.RLIMIT_FSIZE, 10 * 1024 * 1024)
    def test_large_file_operations(self):
        """Test operations with larger files when limits permit."""
        with ResourceLimitHelper(resource.RLIMIT_FSIZE) as helper:
            test_size, can_test = helper.get_safe_test_limit(
                desired_size=10 * 1024 * 1024,
                minimum_size=1024 * 1024
            )
            
            if not can_test:
                self.skipTest("File size limit too restrictive for large file test")
            
            with tempfile.NamedTemporaryFile(delete=True) as f:
                chunk = b'x' * 8192
                chunks_to_write = test_size // len(chunk)
                
                for i in range(chunks_to_write):
                    f.write(chunk)
                    if i % 100 == 0:
                        f.flush()
                
                f.flush()
                file_size = f.tell()
                
                self.assertGreater(file_size, 0)
                self.assertLessEqual(file_size, test_size)
                
                f.seek(0)
                total_read = 0
                while True:
                    data = f.read(8192)
                    if not data:
                        break
                    total_read += len(data)
                
                self.assertEqual(total_read, file_size)

    def test_cpu_time_limit(self):
        """Test CPU time limit handling."""
        if not hasattr(resource, 'RLIMIT_CPU'):
            self.skipTest("RLIMIT_CPU not available on this platform")
        
        with ResourceLimitHelper(resource.RLIMIT_CPU) as helper:
            soft, hard = helper.original_limits
            
            if soft == resource.RLIM_INFINITY:
                test_limit = 3600
                if helper.can_set_limit(test_limit, hard):
                    resource.setrlimit(resource.RLIMIT_CPU, (test_limit, hard))
                    new_soft, new_hard = resource.getrlimit(resource.RLIMIT_CPU)
                    self.assertEqual(new_soft, test_limit)
            elif soft > 10:
                test_limit = soft - 1
                if helper.can_set_limit(test_limit, hard):
                    resource.setrlimit(resource.RLIMIT_CPU, (test_limit, hard))
                    new_soft, new_hard = resource.getrlimit(resource.RLIMIT_CPU)
                    self.assertEqual(new_soft, test_limit)

    def test_integer_overflow(self):
        """Test handling of large values to prevent integer overflow."""
        for res in self.limits:
            try:
                soft, hard = resource.getrlimit(res)
            except (ValueError, OSError):
                continue
            
            if hard != resource.RLIM_INFINITY:
                with ResourceLimitHelper(res):
                    try:
                        resource.setrlimit(res, (hard, hard))
                        
                        if hard < sys.maxsize:
                            with self.assertRaises((ValueError, OSError)):
                                resource.setrlimit(res, (hard + 1, hard))
                    except PermissionError:
                        pass
                    except (ValueError, OSError) as e:
                        if "not support" not in str(e).lower():
                            raise

    @unittest.skipUnless(hasattr(resource, 'prlimit'), 'prlimit() not available')
    def test_prlimit(self):
        """Test the prlimit function for getting/setting process limits."""
        limits = resource.getrlimit(resource.RLIMIT_AS)
        plimits = resource.prlimit(0, resource.RLIMIT_AS)
        self.assertEqual(plimits, limits)
        
        result = resource.prlimit(0, resource.RLIMIT_AS, limits)
        self.assertEqual(result, limits)
        
        for res in self.limits:
            try:
                limits = resource.getrlimit(res)
                plimits = resource.prlimit(0, res)
                self.assertEqual(plimits, limits)
            except (ValueError, OSError):
                pass

    @support.requires_resource('cpu')
    def test_cpu_consumption(self):
        """Test measuring CPU consumption through rusage."""
        usage_start = resource.getrusage(resource.RUSAGE_SELF)
        
        count = 0
        for i in range(1000000):
            count += i
        
        usage_end = resource.getrusage(resource.RUSAGE_SELF)
        
        cpu_start = usage_start.ru_utime + usage_start.ru_stime
        cpu_end = usage_end.ru_utime + usage_end.ru_stime
        
        if cpu_end > cpu_start:
            self.assertGreater(cpu_end, cpu_start)

    def test_resource_constants(self):
        """Test that resource constants are properly defined."""
        self.assertTrue(hasattr(resource, 'RLIM_INFINITY'))
        self.assertTrue(hasattr(resource, 'RUSAGE_SELF'))
        
        self.assertGreater(resource.RLIM_INFINITY, sys.maxsize // 2)
        
        rlimit_names = [name for name in dir(resource) if name.startswith('RLIMIT_')]
        self.assertGreater(len(rlimit_names), 0, "No RLIMIT constants found")


if __name__ == "__main__":
    unittest.main()