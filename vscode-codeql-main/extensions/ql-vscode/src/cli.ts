/* eslint-disable @typescript-eslint/camelcase */
import * as cpp from 'child-process-promise';
import * as child_process from 'child_process';
import * as fs from 'fs-extra';
import * as path from 'path';
import * as sarif from 'sarif';
import { SemVer } from 'semver';
import { Readable } from 'stream';
import { StringDecoder } from 'string_decoder';
import * as tk from 'tree-kill';
import { promisify } from 'util';
import { CancellationToken, Disposable } from 'vscode';

import { BQRSInfo, DecodedBqrsChunk } from './bqrs-cli-types';
import { CliConfig } from './config';
import { DistributionProvider, FindDistributionResultKind } from './distribution';
import { assertNever } from './helpers-pure';
import { QueryMetadata, SortDirection } from './interface-types';
import { Logger, ProgressReporter } from './logging';

/**
 * The version of the SARIF format that we are using.
 */
const SARIF_FORMAT = 'sarifv2.1.0';

/**
 * Flags to pass to all cli commands.
 */
const LOGGING_FLAGS = ['-v', '--log-to-stderr'];

/**
 * The expected output of `codeql resolve library-path`.
 */
export interface QuerySetup {
  libraryPath: string[];
  dbscheme: string;
  relativeName?: string;
  compilationCache?: string;
}

/**
 * The expected output of `codeql resolve database`.
 */
export interface DbInfo {
  sourceLocationPrefix: string;
  columnKind: string;
  unicodeNewlines: boolean;
  sourceArchiveZip: string;
  sourceArchiveRoot: string;
  datasetFolder: string;
  logsFolder: string;
}

/**
 * The expected output of `codeql resolve upgrades`.
 */
export interface UpgradesInfo {
  scripts: string[];
  finalDbscheme: string;
}

/**
 * The expected output of `codeql resolve qlpacks`.
 */
export type QlpacksInfo = { [name: string]: string[] };

// `codeql bqrs interpret` requires both of these to be present or
// both absent.
export interface SourceInfo {
  sourceArchive: string;
  sourceLocationPrefix: string;
}

/**
 * The expected output of `codeql resolve tests`.
 */
export type ResolvedTests = string[];

/**
 * Options for `codeql test run`.
 */
export interface TestRunOptions {
  cancellationToken?: CancellationToken;
  logger?: Logger;
}

/**
 * Event fired by `codeql test run`.
 */
export interface TestCompleted {
  test: string;
  pass: boolean;
  messages: string[];
  compilationMs: number;
  evaluationMs: number;
  expected: string;
}

/**
 * Optional arguments for the `bqrsDecode` function
 */
interface BqrsDecodeOptions {
  /** How many results to get. */
  pageSize?: number;
  /** The 0-based index of the first result to get. */
  offset?: number;
  /** The entity names to retrieve from the bqrs file. Default is url, string */
  entities?: string[];
}

/**
 * This class manages a cli server started by `codeql execute cli-server` to
 * run commands without the overhead of starting a new java
 * virtual machine each time. This class also controls access to the server
 * by queueing the commands sent to it.
 */
export class CodeQLCliServer implements Disposable {

  /**
   * CLI version where --kind=DIL was introduced
   */
  private static CLI_VERSION_WITH_DECOMPILE_KIND_DIL = new SemVer('2.3.0');

  /** The process for the cli server, or undefined if one doesn't exist yet */
  process?: child_process.ChildProcessWithoutNullStreams;
  /** Queue of future commands*/
  commandQueue: (() => void)[];
  /** Whether a command is running */
  commandInProcess: boolean;
  /**  A buffer with a single null byte. */
  nullBuffer: Buffer;

  /** Version of current cli, lazily computed by the `getVersion()` method */
  _version: SemVer | undefined;

  /** Path to current codeQL executable, or undefined if not running yet. */
  codeQlPath: string | undefined;

  constructor(
    private distributionProvider: DistributionProvider,
    private cliConfig: CliConfig,
    private logger: Logger,
  ) {
    this.commandQueue = [];
    this.commandInProcess = false;
    this.nullBuffer = Buffer.alloc(1);
    if (this.distributionProvider.onDidChangeDistribution) {
      this.distributionProvider.onDidChangeDistribution(() => {
        this.restartCliServer();
      });
    }
    if (this.cliConfig.onDidChangeConfiguration) {
      this.cliConfig.onDidChangeConfiguration(() => {
        this.restartCliServer();
        this._version = undefined;
      });
    }
  }

  dispose(): void {
    this.killProcessIfRunning();
  }

  killProcessIfRunning(): void {
    if (this.process) {
      // Tell the Java CLI server process to shut down.
      this.logger.log('Sending shutdown request');
      try {
        this.process.stdin.write(JSON.stringify(['shutdown']), 'utf8');
        this.process.stdin.write(this.nullBuffer);
        this.logger.log('Sent shutdown request');
      } catch (e) {
        // We are probably fine here, the process has already closed stdin.
        this.logger.log(`Shutdown request failed: process stdin may have already closed. The error was ${e}`);
        this.logger.log('Stopping the process anyway.');
      }
      // Close the stdin and stdout streams.
      // This is important on Windows where the child process may not die cleanly.
      this.process.stdin.end();
      this.process.kill();
      this.process.stdout.destroy();
      this.process.stderr.destroy();
      this.process = undefined;

    }
  }

  /**
   * Restart the server when the current command terminates
   */
  private restartCliServer(): void {
    const callback = (): void => {
      try {
        this.killProcessIfRunning();
      } finally {
        this.runNext();
      }
    };

    // If the server is not running a command run this immediately
    // otherwise add to the front of the queue (as we want to run this after the next command()).
    if (this.commandInProcess) {
      this.commandQueue.unshift(callback);
    } else {
      callback();
    }

  }

  /**
   * Get the path to the CodeQL CLI distribution, or throw an exception if not found.
   */
  private async getCodeQlPath(): Promise<string> {
    const codeqlPath = await this.distributionProvider.getCodeQlPathWithoutVersionCheck();
    if (!codeqlPath) {
      throw new Error('Failed to find CodeQL distribution.');
    }
    return codeqlPath;
  }

  /**
   * Launch the cli server
   */
  private async launchProcess(): Promise<child_process.ChildProcessWithoutNullStreams> {
    const codeQlPath = await this.getCodeQlPath();
    return await spawnServer(
      codeQlPath,
      'CodeQL CLI Server',
      ['execute', 'cli-server'],
      [],
      this.logger,
      _data => { /**/ }
    );
  }

  private async runCodeQlCliInternal(command: string[], commandArgs: string[], description: string): Promise<string> {
    const stderrBuffers: Buffer[] = [];
    if (this.commandInProcess) {
      throw new Error('runCodeQlCliInternal called while cli was running');
    }
    this.commandInProcess = true;
    try {
      //Launch the process if it doesn't exist
      if (!this.process) {
        this.process = await this.launchProcess();
      }
      // Grab the process so that typescript know that it is always defined.
      const process = this.process;
      // The array of fragments of stdout
      const stdoutBuffers: Buffer[] = [];

      // Compute the full args array
      const args = command.concat(LOGGING_FLAGS).concat(commandArgs);
      const argsString = args.join(' ');
      this.logger.log(`${description} using CodeQL CLI: ${argsString}...`);
      try {
        await new Promise((resolve, reject) => {
          // Start listening to stdout
          process.stdout.addListener('data', (newData: Buffer) => {
            stdoutBuffers.push(newData);
            // If the buffer ends in '0' then exit.
            // We don't have to check the middle as no output will be written after the null until
            // the next command starts
            if (newData.length > 0 && newData.readUInt8(newData.length - 1) === 0) {
              resolve();
            }
          });
          // Listen to stderr
          process.stderr.addListener('data', (newData: Buffer) => {
            stderrBuffers.push(newData);
          });
          // Listen for process exit.
          process.addListener('close', (code) => reject(code));
          // Write the command followed by a null terminator.
          process.stdin.write(JSON.stringify(args), 'utf8');
          process.stdin.write(this.nullBuffer);
        });
        // Join all the data together
        const fullBuffer = Buffer.concat(stdoutBuffers);
        // Make sure we remove the terminator;
        const data = fullBuffer.toString('utf8', 0, fullBuffer.length - 1);
        this.logger.log('CLI command succeeded.');
        return data;
      } catch (err) {
        // Kill the process if it isn't already dead.
        this.killProcessIfRunning();
        // Report the error (if there is a stderr then use that otherwise just report the error cod or nodejs error)
        const newError =
          stderrBuffers.length == 0
            ? new Error(`${description} failed: ${err}`)
            : new Error(`${description} failed: ${Buffer.concat(stderrBuffers).toString('utf8')}`);
        newError.stack += (err.stack || '');
        throw newError;
      } finally {
        this.logger.log(Buffer.concat(stderrBuffers).toString('utf8'));
        // Remove the listeners we set up.
        process.stdout.removeAllListeners('data');
        process.stderr.removeAllListeners('data');
        process.removeAllListeners('close');
      }
    } finally {
      this.commandInProcess = false;
      // start running the next command immediately
      this.runNext();
    }
  }

  /**
   * Run the next command in the queue
   */
  private runNext(): void {
    const callback = this.commandQueue.shift();
    if (callback) {
      callback();
    }
  }

  /**
   * Runs an asynchronous CodeQL CLI command without invoking the CLI server, returning any events
   * fired by the command as an asynchronous generator.
   *
   * @param command The `codeql` command to be run, provided as an array of command/subcommand names.
   * @param commandArgs The arguments to pass to the `codeql` command.
   * @param cancellationToken CancellationToken to terminate the test process.
   * @param logger Logger to write text output from the command.
   * @returns The sequence of async events produced by the command.
   */
  private async* runAsyncCodeQlCliCommandInternal(
    command: string[],
    commandArgs: string[],
    cancellationToken?: CancellationToken,
    logger?: Logger
  ): AsyncGenerator<string, void, unknown> {
    // Add format argument first, in case commandArgs contains positional parameters.
    const args = [
      ...command,
      '--format', 'jsonz',
      ...commandArgs
    ];

    // Spawn the CodeQL process
    const codeqlPath = await this.getCodeQlPath();
    const childPromise = cpp.spawn(codeqlPath, args);
    const child = childPromise.childProcess;

    let cancellationRegistration: Disposable | undefined = undefined;
    try {
      if (cancellationToken !== undefined) {
        cancellationRegistration = cancellationToken.onCancellationRequested(_e => {
          tk(child.pid);
        });
      }
      if (logger !== undefined) {
        // The human-readable output goes to stderr.
        logStream(child.stderr!, logger);
      }

      for await (const event of await splitStreamAtSeparators(child.stdout!, ['\0'])) {
        yield event;
      }

      await childPromise;
    }
    finally {
      if (cancellationRegistration !== undefined) {
        cancellationRegistration.dispose();
      }
    }
  }

  /**
   * Runs an asynchronous CodeQL CLI command without invoking the CLI server, returning any events
   * fired by the command as an asynchronous generator.
   *
   * @param command The `codeql` command to be run, provided as an array of command/subcommand names.
   * @param commandArgs The arguments to pass to the `codeql` command.
   * @param description Description of the action being run, to be shown in log and error messages.
   * @param cancellationToken CancellationToken to terminate the test process.
   * @param logger Logger to write text output from the command.
   * @returns The sequence of async events produced by the command.
   */
  public async* runAsyncCodeQlCliCommand<EventType>(
    command: string[],
    commandArgs: string[],
    description: string,
    cancellationToken?: CancellationToken,
    logger?: Logger
  ): AsyncGenerator<EventType, void, unknown> {
    for await (const event of await this.runAsyncCodeQlCliCommandInternal(command, commandArgs,
      cancellationToken, logger)) {
      try {
        yield JSON.parse(event) as EventType;
      } catch (err) {
        throw new Error(`Parsing output of ${description} failed: ${err.stderr || err}`);
      }
    }
  }

  /**
   * Runs a CodeQL CLI command on the server, returning the output as a string.
   * @param command The `codeql` command to be run, provided as an array of command/subcommand names.
   * @param commandArgs The arguments to pass to the `codeql` command.
   * @param description Description of the action being run, to be shown in log and error messages.
   * @param progressReporter Used to output progress messages, e.g. to the status bar.
   * @returns The contents of the command's stdout, if the command succeeded.
   */
  runCodeQlCliCommand(command: string[], commandArgs: string[], description: string, progressReporter?: ProgressReporter): Promise<string> {
    if (progressReporter) {
      progressReporter.report({ message: description });
    }

    return new Promise((resolve, reject) => {
      // Construct the command that actually does the work
      const callback = (): void => {
        try {
          this.runCodeQlCliInternal(command, commandArgs, description).then(resolve, reject);
        } catch (err) {
          reject(err);
        }
      };
      // If the server is not running a command, then run the given command immediately,
      // otherwise add to the queue
      if (this.commandInProcess) {
        this.commandQueue.push(callback);
      } else {
        callback();
      }
    });
  }

  /**
   * Runs a CodeQL CLI command, returning the output as JSON.
   * @param command The `codeql` command to be run, provided as an array of command/subcommand names.
   * @param commandArgs The arguments to pass to the `codeql` command.
   * @param description Description of the action being run, to be shown in log and error messages.
   * @param progressReporter Used to output progress messages, e.g. to the status bar.
   * @returns The contents of the command's stdout, if the command succeeded.
   */
  async runJsonCodeQlCliCommand<OutputType>(command: string[], commandArgs: string[], description: string, progressReporter?: ProgressReporter): Promise<OutputType> {
    // Add format argument first, in case commandArgs contains positional parameters.
    const args = ['--format', 'json'].concat(commandArgs);
    const result = await this.runCodeQlCliCommand(command, args, description, progressReporter);
    try {
      return JSON.parse(result) as OutputType;
    } catch (err) {
      throw new Error(`Parsing output of ${description} failed: ${err.stderr || err}`);
    }
  }

  /**
   * Resolve the library path and dbscheme for a query.
   * @param workspaces The current open workspaces
   * @param queryPath The path to the query
   */
  async resolveLibraryPath(workspaces: string[], queryPath: string): Promise<QuerySetup> {
    const subcommandArgs = [
      '--query', queryPath,
      '--additional-packs',
      workspaces.join(path.delimiter)
    ];
    return await this.runJsonCodeQlCliCommand<QuerySetup>(['resolve', 'library-path'], subcommandArgs, 'Resolving library paths');
  }

  /**
   * Finds all available QL tests in a given directory.
   * @param testPath Root of directory tree to search for tests.
   * @returns The list of tests that were found.
   */
  public async resolveTests(testPath: string): Promise<ResolvedTests> {
    const subcommandArgs = [
      testPath
    ];
    return await this.runJsonCodeQlCliCommand<ResolvedTests>(['resolve', 'tests'], subcommandArgs, 'Resolving tests');
  }

  /**
   * Runs QL tests.
   * @param testPaths Full paths of the tests to run.
   * @param workspaces Workspace paths to use as search paths for QL packs.
   * @param options Additional options.
   */
  public async* runTests(
    testPaths: string[], workspaces: string[], options: TestRunOptions
  ): AsyncGenerator<TestCompleted, void, unknown> {

    const subcommandArgs = [
      '--additional-packs', workspaces.join(path.delimiter),
      '--threads',
      this.cliConfig.numberTestThreads.toString(),
      ...testPaths
    ];

    for await (const event of await this.runAsyncCodeQlCliCommand<TestCompleted>(['test', 'run'],
      subcommandArgs, 'Run CodeQL Tests', options.cancellationToken, options.logger)) {
      yield event;
    }
  }

  /**
   * Gets the metadata for a query.
   * @param queryPath The path to the query.
   */
  async resolveMetadata(queryPath: string): Promise<QueryMetadata> {
    return await this.runJsonCodeQlCliCommand<QueryMetadata>(['resolve', 'metadata'], [queryPath], 'Resolving query metadata');
  }

  /**
   * Gets the RAM setting for the query server.
   * @param queryMemoryMb The maximum amount of RAM to use, in MB.
   * Leave `undefined` for CodeQL to choose a limit based on the available system memory.
   * @param progressReporter The progress reporter to send progress information to.
   * @returns String arguments that can be passed to the CodeQL query server,
   * indicating how to split the given RAM limit between heap and off-heap memory.
   */
  async resolveRam(queryMemoryMb: number | undefined, progressReporter?: ProgressReporter): Promise<string[]> {
    const args: string[] = [];
    if (queryMemoryMb !== undefined) {
      args.push('--ram', queryMemoryMb.toString());
    }
    return await this.runJsonCodeQlCliCommand<string[]>(['resolve', 'ram'], args, 'Resolving RAM settings', progressReporter);
  }
  /**
   * Gets the headers (and optionally pagination info) of a bqrs.
   * @param bqrsPath The path to the bqrs.
   * @param pageSize The page size to precompute offsets into the binary file for.
   */
  async bqrsInfo(bqrsPath: string, pageSize?: number): Promise<BQRSInfo> {
    const subcommandArgs = (
      pageSize ? ['--paginate-rows', pageSize.toString()] : []
    ).concat(
      bqrsPath
    );
    return await this.runJsonCodeQlCliCommand<BQRSInfo>(['bqrs', 'info'], subcommandArgs, 'Reading bqrs header');
  }

  /**
  * Gets the results from a bqrs.
  * @param bqrsPath The path to the bqrs.
  * @param resultSet The result set to get.
  * @param options Optional BqrsDecodeOptions arguments
  */
  async bqrsDecode(
    bqrsPath: string,
    resultSet: string,
    { pageSize, offset, entities = ['url', 'string'] }: BqrsDecodeOptions = {}
  ): Promise<DecodedBqrsChunk> {

    const subcommandArgs = [
      `--entities=${entities.join(',')}`,
      '--result-set', resultSet,
    ].concat(
      pageSize ? ['--rows', pageSize.toString()] : []
    ).concat(
      offset ? ['--start-at', offset.toString()] : []
    ).concat([bqrsPath]);
    return await this.runJsonCodeQlCliCommand<DecodedBqrsChunk>(['bqrs', 'decode'], subcommandArgs, 'Reading bqrs data');
  }

  async interpretBqrs(metadata: { kind: string; id: string }, resultsPath: string, interpretedResultsPath: string, sourceInfo?: SourceInfo): Promise<sarif.Log> {
    const args = [
      `-t=kind=${metadata.kind}`,
      `-t=id=${metadata.id}`,
      '--output', interpretedResultsPath,
      '--format', SARIF_FORMAT,
      // TODO: This flag means that we don't group interpreted results
      // by primary location. We may want to revisit whether we call
      // interpretation with and without this flag, or do some
      // grouping client-side.
      '--no-group-results',
    ];
    if (sourceInfo !== undefined) {
      args.push(
        '--source-archive', sourceInfo.sourceArchive,
        '--source-location-prefix', sourceInfo.sourceLocationPrefix
      );
    }
    args.push(resultsPath);
    await this.runCodeQlCliCommand(['bqrs', 'interpret'], args, 'Interpreting query results');

    let output: string;
    try {
      output = await fs.readFile(interpretedResultsPath, 'utf8');
    } catch (err) {
      throw new Error(`Reading output of interpretation failed: ${err.stderr || err}`);
    }
    try {
      return JSON.parse(output) as sarif.Log;
    } catch (err) {
      throw new Error(`Parsing output of interpretation failed: ${err.stderr || err}`);
    }
  }


  async sortBqrs(resultsPath: string, sortedResultsPath: string, resultSet: string, sortKeys: number[], sortDirections: SortDirection[]): Promise<void> {
    const sortDirectionStrings = sortDirections.map(direction => {
      switch (direction) {
        case SortDirection.asc:
          return 'asc';
        case SortDirection.desc:
          return 'desc';
        default:
          return assertNever(direction);
      }
    });

    await this.runCodeQlCliCommand(['bqrs', 'decode'],
      [
        '--format=bqrs',
        `--result-set=${resultSet}`,
        `--output=${sortedResultsPath}`,
        `--sort-key=${sortKeys.join(',')}`,
        `--sort-direction=${sortDirectionStrings.join(',')}`,
        resultsPath
      ],
      'Sorting query results');
  }


  /**
   * Returns the `DbInfo` for a database.
   * @param databasePath Path to the CodeQL database to obtain information from.
   */
  resolveDatabase(databasePath: string): Promise<DbInfo> {
    return this.runJsonCodeQlCliCommand(['resolve', 'database'], [databasePath],
      'Resolving database');
  }

  /**
   * Gets information necessary for upgrading a database.
   * @param dbScheme the path to the dbscheme of the database to be upgraded.
   * @param searchPath A list of directories to search for upgrade scripts.
   * @returns A list of database upgrade script directories
   */
  resolveUpgrades(dbScheme: string, searchPath: string[]): Promise<UpgradesInfo> {
    const args = ['--additional-packs', searchPath.join(path.delimiter), '--dbscheme', dbScheme];

    return this.runJsonCodeQlCliCommand<UpgradesInfo>(
      ['resolve', 'upgrades'],
      args,
      'Resolving database upgrade scripts',
    );
  }

  /**
   * Gets information about available qlpacks
   * @param additionalPacks A list of directories to search for qlpacks before searching in `searchPath`.
   * @param searchPath A list of directories to search for packs not found in `additionalPacks`. If undefined,
   *   the default CLI search path is used.
   * @returns A dictionary mapping qlpack name to the directory it comes from
   */
  resolveQlpacks(additionalPacks: string[], searchPath?: string[]): Promise<QlpacksInfo> {
    const args = ['--additional-packs', additionalPacks.join(path.delimiter)];
    if (searchPath?.length) {
      args.push('--search-path', path.join(...searchPath));
    }

    return this.runJsonCodeQlCliCommand<QlpacksInfo>(
      ['resolve', 'qlpacks'],
      args,
      'Resolving qlpack information',
    );
  }

  /**
   * Gets information about queries in a query suite.
   * @param suite The suite to resolve.
   * @param additionalPacks A list of directories to search for qlpacks before searching in `searchPath`.
   * @param searchPath A list of directories to search for packs not found in `additionalPacks`. If undefined,
   *   the default CLI search path is used.
   * @returns A list of query files found.
   */
  resolveQueriesInSuite(suite: string, additionalPacks: string[], searchPath?: string[]): Promise<string[]> {
    const args = ['--additional-packs', additionalPacks.join(path.delimiter)];
    if (searchPath !== undefined) {
      args.push('--search-path', path.join(...searchPath));
    }
    args.push(suite);
    return this.runJsonCodeQlCliCommand<string[]>(
      ['resolve', 'queries'],
      args,
      'Resolving queries',
    );
  }

  async generateDil(qloFile: string, outFile: string): Promise<void> {
    const extraArgs = (await this.getVersion()).compare(CodeQLCliServer.CLI_VERSION_WITH_DECOMPILE_KIND_DIL) >= 0
      ? ['--kind', 'dil', '-o', outFile, qloFile]
      : ['-o', outFile, qloFile];
    await this.runCodeQlCliCommand(
      ['query', 'decompile'],
      extraArgs,
      'Generating DIL',
    );
  }

  private async getVersion() {
    if (!this._version) {
      this._version = await this.refreshVersion();
    }
    return this._version;
  }

  private async refreshVersion() {
    const distribution = await this.distributionProvider.getDistribution();
    switch(distribution.kind) {
      case FindDistributionResultKind.CompatibleDistribution:
      // eslint-disable-next-line no-fallthrough
      case FindDistributionResultKind.IncompatibleDistribution:
        return distribution.version;

      default:
        // We should not get here because if no distributions are available, then
        // the cli class is never instantiated.
        throw new Error('No distribution found');
    }
  }
}

/**
 * Spawns a child server process using the CodeQL CLI
 * and attaches listeners to it.
 *
 * @param config The configuration containing the path to the CLI.
 * @param name Name of the server being started, to be shown in log and error messages.
 * @param command The `codeql` command to be run, provided as an array of command/subcommand names.
 * @param commandArgs The arguments to pass to the `codeql` command.
 * @param logger Logger to write startup messages.
 * @param stderrListener Listener for log messages from the server's stderr stream.
 * @param stdoutListener Optional listener for messages from the server's stdout stream.
 * @param progressReporter Used to output progress messages, e.g. to the status bar.
 * @returns The started child process.
 */
export function spawnServer(
  codeqlPath: string,
  name: string,
  command: string[],
  commandArgs: string[],
  logger: Logger,
  stderrListener: (data: any) => void,
  stdoutListener?: (data: any) => void,
  progressReporter?: ProgressReporter
): child_process.ChildProcessWithoutNullStreams {
  // Enable verbose logging.
  const args = command.concat(commandArgs).concat(LOGGING_FLAGS);

  // Start the server process.
  const base = codeqlPath;
  const argsString = args.join(' ');
  if (progressReporter !== undefined) {
    progressReporter.report({ message: `Starting ${name}` });
  }
  logger.log(`Starting ${name} using CodeQL CLI: ${base} ${argsString}`);
  const child = child_process.spawn(base, args);
  if (!child || !child.pid) {
    throw new Error(`Failed to start ${name} using command ${base} ${argsString}.`);
  }

  // Set up event listeners.
  child.on('close', (code) => logger.log(`Child process exited with code ${code}`));
  child.stderr!.on('data', stderrListener);
  if (stdoutListener !== undefined) {
    child.stdout!.on('data', stdoutListener);
  }

  if (progressReporter !== undefined) {
    progressReporter.report({ message: `Started ${name}` });
  }
  logger.log(`${name} started on PID: ${child.pid}`);
  return child;
}

/**
 * Runs a CodeQL CLI command without invoking the CLI server, returning the output as a string.
 * @param codeQlPath The path to the CLI.
 * @param command The `codeql` command to be run, provided as an array of command/subcommand names.
 * @param commandArgs The arguments to pass to the `codeql` command.
 * @param description Description of the action being run, to be shown in log and error messages.
 * @param logger Logger to write command log messages, e.g. to an output channel.
 * @param progressReporter Used to output progress messages, e.g. to the status bar.
 * @returns The contents of the command's stdout, if the command succeeded.
 */
export async function runCodeQlCliCommand(
  codeQlPath: string,
  command: string[],
  commandArgs: string[],
  description: string,
  logger: Logger,
  progressReporter?: ProgressReporter
): Promise<string> {
  // Add logging arguments first, in case commandArgs contains positional parameters.
  const args = command.concat(LOGGING_FLAGS).concat(commandArgs);
  const argsString = args.join(' ');
  try {
    if (progressReporter !== undefined) {
      progressReporter.report({ message: description });
    }
    logger.log(`${description} using CodeQL CLI: ${codeQlPath} ${argsString}...`);
    const result = await promisify(child_process.execFile)(codeQlPath, args);
    logger.log(result.stderr);
    logger.log('CLI command succeeded.');
    return result.stdout;
  } catch (err) {
    throw new Error(`${description} failed: ${err.stderr || err}`);
  }
}

/**
 * Buffer to hold state used when splitting a text stream into lines.
 */
class SplitBuffer {
  private readonly decoder = new StringDecoder('utf8');
  private readonly maxSeparatorLength: number;
  private buffer = '';
  private searchIndex = 0;

  constructor(private readonly separators: readonly string[]) {
    this.maxSeparatorLength = separators.map(s => s.length).reduce((a, b) => Math.max(a, b), 0);
  }

  /**
   * Append new text data to the buffer.
   * @param chunk The chunk of data to append.
   */
  public addChunk(chunk: Buffer): void {
    this.buffer += this.decoder.write(chunk);
  }

  /**
   * Signal that the end of the input stream has been reached.
   */
  public end(): void {
    this.buffer += this.decoder.end();
    this.buffer += this.separators[0];  // Append a separator to the end to ensure the last line is returned.
  }

  /**
   * Extract the next full line from the buffer, if one is available.
   * @returns The text of the next available full line (without the separator), or `undefined` if no
   * line is available.
   */
  public getNextLine(): string | undefined {
    while (this.searchIndex <= (this.buffer.length - this.maxSeparatorLength)) {
      for (const separator of this.separators) {
        if (this.buffer.startsWith(separator, this.searchIndex)) {
          const line = this.buffer.substr(0, this.searchIndex);
          this.buffer = this.buffer.substr(this.searchIndex + separator.length);
          this.searchIndex = 0;
          return line;
        }
      }
      this.searchIndex++;
    }

    return undefined;
  }
}

/**
 * Splits a text stream into lines based on a list of valid line separators.
 * @param stream The text stream to split. This stream will be fully consumed.
 * @param separators The list of strings that act as line separators.
 * @returns A sequence of lines (not including separators).
 */
async function* splitStreamAtSeparators(
  stream: Readable, separators: string[]
): AsyncGenerator<string, void, unknown> {

  const buffer = new SplitBuffer(separators);
  for await (const chunk of stream) {
    buffer.addChunk(chunk);
    let line: string | undefined;
    do {
      line = buffer.getNextLine();
      if (line !== undefined) {
        yield line;
      }
    } while (line !== undefined);
  }
  buffer.end();
  let line: string | undefined;
  do {
    line = buffer.getNextLine();
    if (line !== undefined) {
      yield line;
    }
  } while (line !== undefined);
}

/**
 *  Standard line endings for splitting human-readable text.
 */
const lineEndings = ['\r\n', '\r', '\n'];

/**
 * Log a text stream to a `Logger` interface.
 * @param stream The stream to log.
 * @param logger The logger that will consume the stream output.
 */
async function logStream(stream: Readable, logger: Logger): Promise<void> {
  for await (const line of await splitStreamAtSeparators(stream, lineEndings)) {
    logger.log(line);
  }
}
