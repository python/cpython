import unittest
import zoneinfo
import tracemalloc

class Test(unittest.TestCase):
    def test_foo(self):
        # snapshot1 = tracemalloc.take_snapshot()
        zoneinfo.ZoneInfo.no_cache("America/Los_Angeles")
        # snapshot2 = tracemalloc.take_snapshot()
        # top_stats = snapshot2.compare_to(snapshot1, 'lineno')

        # print("[ Top 10 differences ]")
        # for stat in top_stats[:10]:
        #     print(stat)


