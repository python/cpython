import * as path from 'path';
import { QLPackDiscovery, QLPack } from './qlpack-discovery';
import { Discovery } from './discovery';
import { EventEmitter, Event, Uri, RelativePattern, WorkspaceFolder, env, workspace } from 'vscode';
import { MultiFileSystemWatcher } from './vscode-utils/multi-file-system-watcher';
import { CodeQLCliServer } from './cli';

/**
 * A node in the tree of tests. This will be either a `QLTestDirectory` or a `QLTestFile`.
 */
export abstract class QLTestNode {
  constructor(private _path: string, private _name: string) {
  }

  public get path(): string {
    return this._path;
  }

  public get name(): string {
    return this._name;
  }

  public abstract get children(): readonly QLTestNode[];

  public abstract finish(): void;
}

/**
 * A directory containing one or more QL tests or other test directories.
 */
export class QLTestDirectory extends QLTestNode {
  private _children: QLTestNode[] = [];

  constructor(_path: string, _name: string) {
    super(_path, _name);
  }

  public get children(): readonly QLTestNode[] {
    return this._children;
  }

  public addChild(child: QLTestNode): void {
    this._children.push(child);
  }

  public createDirectory(relativePath: string): QLTestDirectory {
    const dirName = path.dirname(relativePath);
    if (dirName === '.') {
      return this.createChildDirectory(relativePath);
    }
    else {
      const parent = this.createDirectory(dirName);
      return parent.createDirectory(path.basename(relativePath));
    }
  }

  public finish(): void {
    this._children.sort((a, b) => a.name.localeCompare(b.name, env.language));
    for (const child of this._children) {
      child.finish();
    }
  }

  private createChildDirectory(name: string): QLTestDirectory {
    const existingChild = this._children.find((child) => child.name === name);
    if (existingChild !== undefined) {
      return existingChild as QLTestDirectory;
    }
    else {
      const newChild = new QLTestDirectory(path.join(this.path, name), name);
      this.addChild(newChild);
      return newChild;
    }
  }
}

/**
 * A single QL test. This will be either a `.ql` file or a `.qlref` file.
 */
export class QLTestFile extends QLTestNode {
  constructor(_path: string, _name: string) {
    super(_path, _name);
  }

  public get children(): readonly QLTestNode[] {
    return [];
  }

  public finish(): void {
    /**/
  }
}

/**
 * The results of discovering QL tests.
 */
interface QLTestDiscoveryResults {
  /**
   * The root test directory for each QL pack that contains tests.
   */
  testDirectories: QLTestDirectory[];
  /**
   * The list of file system paths to watch. If any of these paths changes, the discovery results
   * may be out of date.
   */
  watchPaths: string[];
}

/**
 * Discovers all QL tests contained in the QL packs in a given workspace folder.
 */
export class QLTestDiscovery extends Discovery<QLTestDiscoveryResults> {
  private readonly _onDidChangeTests = this.push(new EventEmitter<void>());
  private readonly watcher: MultiFileSystemWatcher = this.push(new MultiFileSystemWatcher());
  private _testDirectories: QLTestDirectory[] = [];

  constructor(
    private readonly qlPackDiscovery: QLPackDiscovery,
    private readonly workspaceFolder: WorkspaceFolder,
    private readonly cliServer: CodeQLCliServer
  ) {
    super('QL Test Discovery');

    this.push(this.qlPackDiscovery.onDidChangeQLPacks(this.handleDidChangeQLPacks, this));
    this.push(this.watcher.onDidChange(this.handleDidChange, this));
  }

  /**
   * Event to be fired when the set of discovered tests may have changed.
   */
  public get onDidChangeTests(): Event<void> { return this._onDidChangeTests.event; }

  /**
   * The root test directory for each QL pack that contains tests.
   */
  public get testDirectories(): QLTestDirectory[] { return this._testDirectories; }

  private handleDidChangeQLPacks(): void {
    this.refresh();
  }

  private handleDidChange(uri: Uri): void {
    if (!QLTestDiscovery.ignoreTestPath(uri.fsPath)) {
      this.refresh();
    }
  }

  protected async discover(): Promise<QLTestDiscoveryResults> {
    const testDirectories: QLTestDirectory[] = [];
    const watchPaths: string[] = [];
    const qlPacks = this.qlPackDiscovery.qlPacks;
    for (const qlPack of qlPacks) {
      //HACK: Assume that only QL packs whose name ends with '-tests' contain tests.
      if (this.isRelevantQlPack(qlPack)) {
        watchPaths.push(qlPack.uri.fsPath);
        const testPackage = await this.discoverTests(qlPack.uri.fsPath, qlPack.name);
        if (testPackage !== undefined) {
          testDirectories.push(testPackage);
        }
      }
    }

    return { testDirectories, watchPaths };
  }

  protected update(results: QLTestDiscoveryResults): void {
    this._testDirectories = results.testDirectories;

    // Watch for changes to any `.ql` or `.qlref` file in any of the QL packs that contain tests.
    this.watcher.clear();
    results.watchPaths.forEach(watchPath => {
      this.watcher.addWatch(new RelativePattern(watchPath, '**/*.{ql,qlref}'));
    });
    this._onDidChangeTests.fire();
  }

  /**
   * Only include qlpacks suffixed with '-tests' that are contained
   * within the provided workspace folder.
   */
  private isRelevantQlPack(qlPack: QLPack): boolean {
    return qlPack.name.endsWith('-tests')
      && workspace.getWorkspaceFolder(qlPack.uri)?.index === this.workspaceFolder.index;
  }

  /**
   * Discover all QL tests in the specified directory and its subdirectories.
   * @param fullPath The full path of the test directory.
   * @param name The display name to use for the returned `TestDirectory` object.
   * @returns A `QLTestDirectory` object describing the contents of the directory, or `undefined` if
   *   no tests were found.
   */
  private async discoverTests(fullPath: string, name: string): Promise<QLTestDirectory | undefined> {
    const resolvedTests = (await this.cliServer.resolveTests(fullPath))
      .filter((testPath) => !QLTestDiscovery.ignoreTestPath(testPath));

    if (resolvedTests.length === 0) {
      return undefined;
    }
    else {
      const rootDirectory = new QLTestDirectory(fullPath, name);
      for (const testPath of resolvedTests) {
        const relativePath = path.normalize(path.relative(fullPath, testPath));
        const dirName = path.dirname(relativePath);
        const parentDirectory = rootDirectory.createDirectory(dirName);
        parentDirectory.addChild(new QLTestFile(testPath, path.basename(testPath)));
      }

      rootDirectory.finish();

      return rootDirectory;
    }
  }

  /**
   * Determine if the specified QL test should be ignored based on its filename.
   * @param testPath Path to the test file.
   */
  private static ignoreTestPath(testPath: string): boolean {
    switch (path.extname(testPath).toLowerCase()) {
      case '.ql':
      case '.qlref':
        return path.basename(testPath).startsWith('__');

      default:
        return false;
    }
  }
}
