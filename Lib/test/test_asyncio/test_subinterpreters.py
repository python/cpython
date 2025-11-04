from test.test_asyncio import utils as test_utils

from asyncio import run_in_subinterpreter


def simple_func():
    return 42

def sum_func(a, b):
    return a + b

def raise_func():
    raise ValueError("Test error")


class TestRunInSubinterpreter(test_utils.TestCase):

    async def test_basic_return(self):
        """Check that simple function returns correct result."""
        result = await run_in_subinterpreter(simple_func)
        self.assertEqual(result, 42)

    async def test_arguments(self):
        """Check that positional arguments are passed correctly."""
        result = await run_in_subinterpreter(sum_func, 10, 32)
        self.assertEqual(result, 42)

    async def test_exception_propagation(self):
        """Check that exceptions inside subinterpreter are propagated."""
        with self.assertRaises(ValueError) as cm:
            await run_in_subinterpreter(raise_func)
        self.assertEqual(str(cm.exception), "Test error")

    async def test_multiple_calls(self):
        """Check that multiple sequential calls work."""
        results = []
        for i in range(5):
            result = await run_in_subinterpreter(lambda x=i: x * 2)
            results.append(result)
        self.assertEqual(results, [0, 2, 4, 6, 8])