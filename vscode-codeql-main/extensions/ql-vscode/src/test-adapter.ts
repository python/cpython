import * as path from 'path';
import * as vscode from 'vscode';
import {
  TestAdapter,
  TestLoadStartedEvent,
  TestLoadFinishedEvent,
  TestRunStartedEvent,
  TestRunFinishedEvent,
  TestSuiteEvent,
  TestEvent,
  TestSuiteInfo,
  TestInfo,
  TestHub
} from 'vscode-test-adapter-api';
import { TestAdapterRegistrar } from 'vscode-test-adapter-util';
import { QLTestFile, QLTestNode, QLTestDirectory, QLTestDiscovery } from './qltest-discovery';
import { Event, EventEmitter, CancellationTokenSource, CancellationToken } from 'vscode';
import { DisposableObject } from './vscode-utils/disposable-object';
import { QLPackDiscovery } from './qlpack-discovery';
import { CodeQLCliServer } from './cli';
import { getOnDiskWorkspaceFolders } from './helpers';
import { testLogger } from './logging';

/**
 * Get the full path of the `.expected` file for the specified QL test.
 * @param testPath The full path to the test file.
 */
export function getExpectedFile(testPath: string): string {
  return getTestOutputFile(testPath, '.expected');
}

/**
 * Get the full path of the `.actual` file for the specified QL test.
 * @param testPath The full path to the test file.
 */
export function getActualFile(testPath: string): string {
  return getTestOutputFile(testPath, '.actual');
}

/**
 * Get the directory containing the specified QL test.
 * @param testPath The full path to the test file.
 */
export function getTestDirectory(testPath: string): string {
  return path.dirname(testPath);
}

/**
 * Gets the the full path to a particular output file of the specified QL test.
 * @param testPath The full path to the QL test.
 * @param extension The file extension of the output file.
 */
function getTestOutputFile(testPath: string, extension: string): string {
  return changeExtension(testPath, extension);
}

/**
 * A factory service that creates `QLTestAdapter` objects for workspace folders on demand.
 */
export class QLTestAdapterFactory extends DisposableObject {
  constructor(testHub: TestHub, cliServer: CodeQLCliServer) {
    super();

    // this will register a QLTestAdapter for each WorkspaceFolder
    this.push(new TestAdapterRegistrar(
      testHub,
      workspaceFolder => new QLTestAdapter(workspaceFolder, cliServer)
    ));
  }
}

/**
 * Change the file extension of the specified path.
 * @param p The original file path.
 * @param ext The new extension, including the `.`.
 */
function changeExtension(p: string, ext: string): string {
  return p.substr(0, p.length - path.extname(p).length) + ext;
}

/**
 * Test adapter for QL tests.
 */
export class QLTestAdapter extends DisposableObject implements TestAdapter {
  private readonly qlPackDiscovery: QLPackDiscovery;
  private readonly qlTestDiscovery: QLTestDiscovery;
  private readonly _tests = this.push(
    new EventEmitter<TestLoadStartedEvent | TestLoadFinishedEvent>());
  private readonly _testStates = this.push(
    new EventEmitter<TestRunStartedEvent | TestRunFinishedEvent | TestSuiteEvent | TestEvent>());
  private readonly _autorun = this.push(new EventEmitter<void>());
  private runningTask?: vscode.CancellationTokenSource = undefined;

  constructor(
    public readonly workspaceFolder: vscode.WorkspaceFolder,
    private readonly cliServer: CodeQLCliServer
  ) {
    super();

    this.qlPackDiscovery = this.push(new QLPackDiscovery(workspaceFolder, cliServer));
    this.qlTestDiscovery = this.push(new QLTestDiscovery(this.qlPackDiscovery, workspaceFolder, cliServer));
    this.qlPackDiscovery.refresh();
    this.qlTestDiscovery.refresh();

    this.push(this.qlTestDiscovery.onDidChangeTests(this.discoverTests, this));
  }

  public get tests(): Event<TestLoadStartedEvent | TestLoadFinishedEvent> {
    return this._tests.event;
  }

  public get testStates(): Event<TestRunStartedEvent | TestRunFinishedEvent | TestSuiteEvent | TestEvent> {
    return this._testStates.event;
  }

  public get autorun(): Event<void> | undefined {
    return this._autorun.event;
  }

  private static createTestOrSuiteInfos(testNodes: readonly QLTestNode[]): (TestSuiteInfo | TestInfo)[] {
    return testNodes.map((childNode) => {
      return QLTestAdapter.createTestOrSuiteInfo(childNode);
    });
  }

  private static createTestOrSuiteInfo(testNode: QLTestNode): TestSuiteInfo | TestInfo {
    if (testNode instanceof QLTestFile) {
      return QLTestAdapter.createTestInfo(testNode);
    } else if (testNode instanceof QLTestDirectory) {
      return QLTestAdapter.createTestSuiteInfo(testNode, testNode.name);
    } else {
      throw new Error('Unexpected test type.');
    }
  }

  private static createTestInfo(testFile: QLTestFile): TestInfo {
    return {
      type: 'test',
      id: testFile.path,
      label: testFile.name,
      tooltip: testFile.path,
      file: testFile.path
    };
  }

  private static createTestSuiteInfo(testDirectory: QLTestDirectory, label: string): TestSuiteInfo {
    return {
      type: 'suite',
      id: testDirectory.path,
      label: label,
      children: QLTestAdapter.createTestOrSuiteInfos(testDirectory.children),
      tooltip: testDirectory.path
    };
  }

  public async load(): Promise<void> {
    this.discoverTests();
  }

  private discoverTests(): void {
    this._tests.fire({ type: 'started' } as TestLoadStartedEvent);

    const testDirectories = this.qlTestDiscovery.testDirectories;
    const children = testDirectories.map(
      testDirectory => QLTestAdapter.createTestSuiteInfo(testDirectory, testDirectory.name)
    );
    const testSuite: TestSuiteInfo = {
      type: 'suite',
      label: 'CodeQL',
      id: '.',
      children
    };

    this._tests.fire({
      type: 'finished',
      suite: children.length > 0 ? testSuite : undefined
    } as TestLoadFinishedEvent);
  }

  public async run(tests: string[]): Promise<void> {
    if (this.runningTask !== undefined) {
      throw new Error('Tests already running.');
    }

    testLogger.outputChannel.clear();
    testLogger.outputChannel.show(true);

    this.runningTask = this.track(new CancellationTokenSource());

    this._testStates.fire({ type: 'started', tests: tests } as TestRunStartedEvent);

    try {
      await this.runTests(tests, this.runningTask.token);
    }
    catch (e) {
      /**/
    }
    this._testStates.fire({ type: 'finished' } as TestRunFinishedEvent);
    this.clearTask();
  }

  private clearTask(): void {
    if (this.runningTask !== undefined) {
      const runningTask = this.runningTask;
      this.runningTask = undefined;
      this.disposeAndStopTracking(runningTask);
    }
  }

  public cancel(): void {
    if (this.runningTask !== undefined) {
      testLogger.log('Cancelling test run...');
      this.runningTask.cancel();
      this.clearTask();
    }
  }

  private async runTests(tests: string[], cancellationToken: CancellationToken): Promise<void> {
    const workspacePaths = await getOnDiskWorkspaceFolders();
    for await (const event of await this.cliServer.runTests(tests, workspacePaths, {
      cancellationToken: cancellationToken,
      logger: testLogger
    })) {
      this._testStates.fire({
        type: 'test',
        state: event.pass ? 'passed' : 'failed',
        test: event.test
      });
    }
  }
}
