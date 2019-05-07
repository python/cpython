from unittest import AsyncioTestCase


class TestAsyncCase(AsyncioTestCase):
    async def setUp(self):
        self._setup_called = 1

    async def test_func(self):
        assert self._setup_called == 1
        self._setup_called = 2

    async def tearDown(self):
        assert self._setup_called == 2
