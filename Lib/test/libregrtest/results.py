import os

from test.support import os_helper

from . import State, TestResult, TestStats, TestsTuple, FilterDict
from .utils import count, printlist, format_duration, print_warning


TestsList = list[str]

EXITCODE_BAD_TEST = 2
EXITCODE_INTERRUPTED = 130
EXITCODE_ENV_CHANGED = 3
EXITCODE_NO_TESTS_RAN = 4
EXITCODE_RERUN_FAIL = 5


class Results:
    def __init__(self, xmlpath: str | None = None):
        self.bad: TestsList = []
        self.good: TestsList = []
        self.rerun_bad: TestsList = []
        self.skipped: TestsList = []
        self.resource_denied: TestsList = []
        self.environment_changed: TestsList = []
        self.run_no_tests: TestsList = []
        self.rerun: TestsList = []

        self.need_rerun: list[TestResult] = []
        self.interrupted = False
        self.total_stats = TestStats()

        # used by --slow
        self.test_times = []

        # used by --junit-xml
        self.xmlpath = xmlpath
        if self.xmlpath:
            self.testsuite_xml = []
        else:
            self.testsuite_xml = None

    def get_executed(self):
        return (set(self.good) | set(self.bad) | set(self.skipped)
                | set(self.resource_denied) | set(self.environment_changed)
                | set(self.run_no_tests))

    def no_tests_run(self):
        return not any((self.good, self.bad, self.skipped, self.interrupted,
                        self.environment_changed))

    def rerun_needed(self):
        return bool(self.need_rerun)

    @staticmethod
    def get_rerun_match(rerun_list) -> FilterDict:
        rerun_match_tests: FilterDict = {}
        for result in rerun_list:
            match_tests = result.get_rerun_match_tests()
            # ignore empty match list
            if match_tests:
                rerun_match_tests[result.test_name] = match_tests
        return rerun_match_tests

    def prepare_rerun(self) -> tuple[TestsTuple, FilterDict]:
        tests = tuple(result.test_name for result in self.need_rerun)
        match_tests_dict = self.get_rerun_match(self.need_rerun)

        # Clear previously failed tests
        self.rerun_bad.extend(self.bad)
        self.bad.clear()
        self.need_rerun.clear()

        return (tests, match_tests_dict)

    def add_junit(self, xml_data: str):
        import xml.etree.ElementTree as ET
        for item in xml_data:
            try:
                node = ET.fromstring(item)
                self.testsuite_xml.append(node)
            except ET.ParseError:
                print_warning("failed to parse XML: {xml_data!r}")
                raise

    def accumulate_result(self, result, runtests):
        fail_env_changed = runtests.fail_env_changed
        rerun = runtests.rerun
        test_name = result.test_name

        match result.state:
            case State.PASSED:
                self.good.append(test_name)
            case State.ENV_CHANGED:
                self.environment_changed.append(test_name)
            case State.SKIPPED:
                self.skipped.append(test_name)
            case State.RESOURCE_DENIED:
                self.resource_denied.append(test_name)
            case State.INTERRUPTED:
                self.interrupted = True
            case State.DID_NOT_RUN:
                self.run_no_tests.append(test_name)
            case _:
                if result.is_failed(fail_env_changed):
                    self.bad.append(test_name)
                    self.need_rerun.append(result)
                else:
                    raise ValueError(f"invalid test state: {result.state!r}")

        if result.has_meaningful_duration() and not rerun:
            self.test_times.append((result.duration, test_name))
        if result.stats is not None:
            self.total_stats.accumulate(result.stats)
        if rerun:
            self.rerun.append(test_name)

        if result.xml_data:
            self.add_junit(result.xml_data)

    def use_junit(self):
        return (self.xmlpath is not None)

    def write_junit(self):
        if not self.use_junit():
            return
        if not self.testsuite_xml:
            # Don't write empty JUnit file
            return

        import xml.etree.ElementTree as ET
        root = ET.Element("testsuites")

        # Manually count the totals for the overall summary
        totals = {'tests': 0, 'errors': 0, 'failures': 0}
        for suite in self.testsuite_xml:
            root.append(suite)
            for k in totals:
                try:
                    totals[k] += int(suite.get(k, 0))
                except ValueError:
                    pass

        for k, v in totals.items():
            root.set(k, str(v))

        xmlpath = os.path.join(os_helper.SAVEDCWD, self.xmlpath)
        with open(xmlpath, 'wb') as f:
            for s in ET.tostringlist(root):
                f.write(s)

    def get_exitcode(self, *, fail_env_changed, fail_rerun):
        exitcode = 0
        if self.bad:
            exitcode = EXITCODE_BAD_TEST
        elif self.interrupted:
            exitcode = EXITCODE_INTERRUPTED
        elif fail_env_changed and self.environment_changed:
            exitcode = EXITCODE_ENV_CHANGED
        elif self.no_tests_run():
            exitcode = EXITCODE_NO_TESTS_RAN
        elif self.rerun and fail_rerun:
            exitcode = EXITCODE_RERUN_FAIL
        return exitcode

    def get_state(self, *, fail_env_changed: bool, first_state: bool):
        result = []
        if self.bad:
            result.append("FAILURE")
        elif fail_env_changed and self.environment_changed:
            result.append("ENV CHANGED")
        elif self.no_tests_run():
            result.append("NO TESTS RAN")

        if self.interrupted:
            result.append("INTERRUPTED")

        if not result:
            result.append("SUCCESS")

        result = ', '.join(result)
        if first_state:
            result = '%s then %s' % (first_state, result)
        return result

    def display_result(self, selected, *, quiet: bool, print_slowest: bool,
                       fail_env_changed: bool, first_state: bool):
        print()
        state = self.get_state(
            fail_env_changed=fail_env_changed,
            first_state=first_state)
        print(f"== Tests result: {state} ==")

        if self.interrupted:
            print("Test suite interrupted by signal SIGINT.")

        omitted = set(selected) - self.get_executed()
        if omitted:
            print()
            print(count(len(omitted), "test"), "omitted:")
            printlist(omitted)

        if self.good and not quiet:
            print()
            if (not self.bad
                and not self.skipped
                and not self.interrupted
                and len(self.good) > 1):
                print("All", end=' ')
            print(count(len(self.good), "test"), "OK.")

        if print_slowest:
            self.test_times.sort(reverse=True)
            print()
            print("10 slowest tests:")
            for test_time, test in self.test_times[:10]:
                print("- %s: %s" % (test, format_duration(test_time)))

        if self.bad:
            print()
            print(count(len(self.bad), "test"), "failed:")
            printlist(self.bad)

        if self.environment_changed:
            print()
            print("{} altered the execution environment:".format(
                     count(len(self.environment_changed), "test")))
            printlist(self.environment_changed)

        if self.skipped and not quiet:
            print()
            print(count(len(self.skipped), "test"), "skipped:")
            printlist(self.skipped)

        if self.resource_denied and not quiet:
            print()
            print(count(len(self.resource_denied), "test"), "skipped (resource denied):")
            printlist(self.resource_denied)

        if self.rerun:
            print()
            print("%s:" % count(len(self.rerun), "re-run test"))
            printlist(self.rerun)

        if self.run_no_tests:
            print()
            print(count(len(self.run_no_tests), "test"), "run no tests:")
            printlist(self.run_no_tests)
