import asyncio

try:
    from asyncio import run_in_subinterpreter
except ImportError:
    raise unittest.SkipTest("subinterpreters not supported")

import unittest

def simple_func():
    return 42

def sum_func(a, b):
    return a + b

def raise_func():
    raise ValueError("Test error")

def large_number():
    return 10**15 * 10**15

def return_objects():
    return {"a": [1, 2, 3], "b": (4, 5), "c": {"nested": 42}}

def multiply_by_3(x):
    return x * 3

def multiply_by_2(x):
    return x * 2


class TestRunInSubinterpreter(unittest.IsolatedAsyncioTestCase):
    
    async def asyncSetUp(self) -> None:
        return await super().asyncSetUp()

    async def test_basic_return(self):
        """Check that simple function returns correct result."""
        result = await run_in_subinterpreter(simple_func)
        self.assertEqual(result, 42)

    async def test_arguments(self):
        """Check that positional arguments are passed correctly."""
        result = await run_in_subinterpreter(sum_func, 10, 32)
        self.assertEqual(result, 42)

    async def test_exception_propagation(self):
        """Check that exceptions inside subinterpreter are propagated.
        """
        with self.assertRaises(ValueError) as cm:
            await run_in_subinterpreter(raise_func)
        self.assertEqual(str(cm.exception), "Test error")

    async def test_multiple_calls(self):
        """Check that multiple sequential calls work."""
        results = []
        for i in range(5):
            result = await run_in_subinterpreter(multiply_by_2, i)
            results.append(result)
        self.assertEqual(results, [0, 2, 4, 6, 8])
        
    async def test_parallel_calls(self):
        """Check that multiple calls can be awaited concurrently."""
        tasks = [
            run_in_subinterpreter(multiply_by_3, i)
            for i in range(5)
        ]
        results = await asyncio.gather(*tasks)
        self.assertEqual(results, [0, 3, 6, 9, 12])

    async def test_complex_objects(self):
        """Check that function can return complex objects."""
        result = await run_in_subinterpreter(return_objects)
        self.assertEqual(
            result, 
            {"a": [1, 2, 3], "b": (4, 5), "c": {"nested": 42}}
        )

    async def test_large_numbers(self):
        """Check CPU-bound operation with large numbers."""
        result = await run_in_subinterpreter(large_number)
        self.assertEqual(result, 10**15 * 10**15)