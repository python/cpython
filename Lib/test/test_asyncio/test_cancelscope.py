"""Tests for asyncio/cancelscope.py"""

import unittest

import asyncio
from asyncio import tasks


def tearDownModule():
    asyncio.events._set_event_loop_policy(None)


class BaseCancelScopeTests:
    """Mixin of CancelScope tests run against both Python and C Task."""

    Task = None  # set by subclasses

    def _run(self, coro):
        """Run *coro* using asyncio.run() but with the desired Task class."""
        loop = asyncio.new_event_loop()
        if self.Task is not None:
            loop.set_task_factory(
                lambda loop, coro, **kw: self.Task(coro, loop=loop, **kw))
        try:
            return loop.run_until_complete(coro)
        finally:
            loop.close()

    # -- basic tests ---------------------------------------------------------

    def test_cancel_raises_at_next_await(self):
        async def main():
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                with self.assertRaises(asyncio.CancelledError):
                    await asyncio.sleep(0)
            # CancelledError was caught inside, so cancelled_caught is False
            self.assertFalse(scope.cancelled_caught)

        self._run(main())

    def test_cancel_propagates_to_scope(self):
        """CancelledError propagates to __aexit__ and is suppressed."""
        async def main():
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                await asyncio.sleep(0)  # raises CancelledError, NOT caught
            # Scope suppressed it
            self.assertTrue(scope.cancelled_caught)

        self._run(main())

    def test_cancel_called_property(self):
        async def main():
            scope = asyncio.CancelScope()
            self.assertFalse(scope.cancel_called)
            scope.cancel()
            self.assertTrue(scope.cancel_called)

        self._run(main())

    def test_scope_without_cancel(self):
        async def main():
            async with asyncio.CancelScope() as scope:
                await asyncio.sleep(0)
            self.assertFalse(scope.cancel_called)
            self.assertFalse(scope.cancelled_caught)

        self._run(main())

    def test_scope_requires_task(self):
        async def main():
            async with asyncio.CancelScope() as scope:
                pass
            self.assertFalse(scope.cancel_called)

        self._run(main())

    # -- level-triggered re-injection ----------------------------------------

    def test_level_triggered_reinjection(self):
        """Once cancelled, CancelledError re-raises at every subsequent await."""
        caught_count = 0

        async def main():
            nonlocal caught_count
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                for _ in range(5):
                    try:
                        await asyncio.sleep(0)
                    except asyncio.CancelledError:
                        caught_count += 1

        self._run(main())
        self.assertEqual(caught_count, 5)

    def test_level_triggered_successive_catches(self):
        """Multiple try/except blocks each catch a fresh CancelledError."""
        async def main():
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                try:
                    await asyncio.sleep(0)
                except asyncio.CancelledError:
                    pass
                # still cancelled — next await raises again
                try:
                    await asyncio.sleep(0)
                except asyncio.CancelledError:
                    pass
            # Error was caught inside each time; cancelled_caught is False
            self.assertFalse(scope.cancelled_caught)

        self._run(main())

    def test_no_reinjection_after_scope_exits(self):
        """CancelledError stops once the scope is exited."""
        async def main():
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                try:
                    await asyncio.sleep(0)
                except asyncio.CancelledError:
                    pass
            # Outside scope — no re-injection
            await asyncio.sleep(0)  # should NOT raise

        self._run(main())

    # -- deadline / timeout --------------------------------------------------

    def test_deadline_fires(self):
        """Deadline causes CancelledError which the scope suppresses."""
        async def main():
            loop = asyncio.get_running_loop()
            deadline = loop.time() + 0.01
            async with asyncio.CancelScope(deadline=deadline) as scope:
                await asyncio.sleep(10)
            self.assertTrue(scope.cancel_called)
            self.assertTrue(scope.cancelled_caught)

        self._run(main())

    def test_deadline_caught_inside(self):
        """Deadline fires, error caught inside, scope exits normally."""
        async def main():
            loop = asyncio.get_running_loop()
            deadline = loop.time() + 0.01
            async with asyncio.CancelScope(deadline=deadline) as scope:
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    pass
            self.assertTrue(scope.cancel_called)
            self.assertFalse(scope.cancelled_caught)

        self._run(main())

    def test_cancel_scope_convenience(self):
        """cancel_scope(delay) fires and suppresses the CancelledError."""
        async def main():
            async with asyncio.cancel_scope(0.01) as scope:
                await asyncio.sleep(10)
            self.assertTrue(scope.cancel_called)
            self.assertTrue(scope.cancelled_caught)

        self._run(main())

    def test_cancel_scope_at_convenience(self):
        """cancel_scope_at(when) fires and suppresses the CancelledError."""
        async def main():
            loop = asyncio.get_running_loop()
            async with asyncio.cancel_scope_at(loop.time() + 0.01) as scope:
                await asyncio.sleep(10)
            self.assertTrue(scope.cancel_called)
            self.assertTrue(scope.cancelled_caught)

        self._run(main())

    def test_reschedule(self):
        """reschedule() to a sooner deadline fires."""
        async def main():
            loop = asyncio.get_running_loop()
            async with asyncio.CancelScope(deadline=loop.time() + 100) as scope:
                scope.reschedule(loop.time() + 0.01)
                await asyncio.sleep(10)
            self.assertTrue(scope.cancel_called)
            self.assertTrue(scope.cancelled_caught)

        self._run(main())

    def test_reschedule_remove(self):
        async def main():
            loop = asyncio.get_running_loop()
            async with asyncio.CancelScope(deadline=loop.time() + 0.01) as scope:
                # Remove deadline
                scope.reschedule(None)
                await asyncio.sleep(0.05)
            self.assertFalse(scope.cancel_called)

        self._run(main())

    # -- shield --------------------------------------------------------------

    def test_shield_blocks_reinjection(self):
        """shield=True prevents level-triggered re-injection."""
        async def main():
            async with asyncio.CancelScope(shield=True) as scope:
                scope.cancel()
                # The initial cancel() still sends one edge-triggered
                # CancelledError; catch it.
                try:
                    await asyncio.sleep(0)
                except asyncio.CancelledError:
                    pass
                # With shield, subsequent awaits should NOT re-inject
                await asyncio.sleep(0)  # should NOT raise

        self._run(main())

    def test_shield_property(self):
        async def main():
            scope = asyncio.CancelScope(shield=True)
            self.assertTrue(scope.shield)
            scope.shield = False
            self.assertFalse(scope.shield)

        self._run(main())

    # -- nested scopes -------------------------------------------------------

    def test_inner_cancelled_outer_not(self):
        async def main():
            async with asyncio.CancelScope() as outer:
                async with asyncio.CancelScope() as inner:
                    inner.cancel()
                    await asyncio.sleep(0)  # raises, propagates to inner
                # inner suppressed the CancelledError
                self.assertTrue(inner.cancelled_caught)
                # Outer scope is not cancelled, should work fine
                await asyncio.sleep(0)
            self.assertFalse(outer.cancel_called)

        self._run(main())

    def test_outer_cancelled_inner_not(self):
        async def main():
            async with asyncio.CancelScope() as outer:
                outer.cancel()
                async with asyncio.CancelScope() as inner:
                    # inner is not cancelled; the edge-triggered cancel from
                    # outer.cancel() → task.cancel() comes through once.
                    try:
                        await asyncio.sleep(0)
                    except asyncio.CancelledError:
                        pass
                    # No level-triggered re-injection from inner (it's not cancelled)
                    await asyncio.sleep(0)

        self._run(main())

    def test_nested_both_cancelled(self):
        async def main():
            async with asyncio.CancelScope() as outer:
                outer.cancel()
                async with asyncio.CancelScope() as inner:
                    inner.cancel()
                    # CancelledError propagates to inner scope
                    await asyncio.sleep(0)
                # inner suppresses it
                self.assertTrue(inner.cancelled_caught)
                # outer is still cancelled; next await re-injects
                await asyncio.sleep(0)
            # outer suppresses its own CancelledError
            self.assertTrue(outer.cancelled_caught)

        self._run(main())

    # -- edge-triggered unchanged --------------------------------------------

    def test_plain_task_cancel_unchanged(self):
        """task.cancel() without CancelScope remains edge-triggered."""
        async def inner():
            try:
                await asyncio.sleep(10)
            except asyncio.CancelledError:
                # Swallow: edge-triggered, so it stays swallowed
                return 'swallowed'
            return 'completed'

        async def main():
            task = asyncio.ensure_future(inner())
            await asyncio.sleep(0)  # let inner start
            task.cancel()
            result = await task
            self.assertEqual(result, 'swallowed')

        self._run(main())

    # -- cancelled_caught property -------------------------------------------

    def test_cancelled_caught_true(self):
        """cancelled_caught is True when CancelledError propagates to scope."""
        async def main():
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                await asyncio.sleep(0)  # raises, NOT caught → to __aexit__
            self.assertTrue(scope.cancelled_caught)

        self._run(main())

    def test_cancelled_caught_false_when_not_cancelled(self):
        async def main():
            async with asyncio.CancelScope() as scope:
                await asyncio.sleep(0)
            self.assertFalse(scope.cancelled_caught)

        self._run(main())

    def test_cancelled_caught_false_when_caught_inside(self):
        """cancelled_caught is False when CancelledError caught inside scope."""
        async def main():
            async with asyncio.CancelScope() as scope:
                scope.cancel()
                try:
                    await asyncio.sleep(0)
                except asyncio.CancelledError:
                    pass
            self.assertTrue(scope.cancel_called)
            self.assertFalse(scope.cancelled_caught)

        self._run(main())

    # -- deadline property ---------------------------------------------------

    def test_deadline_property(self):
        async def main():
            scope = asyncio.CancelScope(deadline=42.0)
            self.assertEqual(scope.deadline, 42.0)
            scope.deadline = 99.0
            self.assertEqual(scope.deadline, 99.0)

        self._run(main())

    def test_deadline_none(self):
        async def main():
            scope = asyncio.CancelScope()
            self.assertIsNone(scope.deadline)

        self._run(main())


class PyTask_CancelScopeTests(BaseCancelScopeTests, unittest.TestCase):
    Task = tasks._PyTask


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class CTask_CancelScopeTests(BaseCancelScopeTests, unittest.TestCase):
    Task = getattr(tasks, '_CTask', None)

    def test_cancel_scope_inside_taskgroup(self):
        """CancelScope inside TaskGroup: scope cancellation doesn't abort TG."""
        async def main():
            results = []
            async with asyncio.TaskGroup() as tg:
                async with asyncio.CancelScope() as scope:
                    scope.cancel()
                    try:
                        await asyncio.sleep(0)
                    except asyncio.CancelledError:
                        results.append('caught')
                results.append('after_scope')
            self.assertEqual(results, ['caught', 'after_scope'])

        self._run(main())


if __name__ == '__main__':
    unittest.main()
