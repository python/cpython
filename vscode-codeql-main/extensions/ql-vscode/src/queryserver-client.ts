import * as cp from 'child_process';
import * as path from 'path';
import { DisposableObject } from './vscode-utils/disposable-object';
import { Disposable } from 'vscode';
import { CancellationToken, createMessageConnection, MessageConnection, RequestType } from 'vscode-jsonrpc';
import * as cli from './cli';
import { QueryServerConfig } from './config';
import { Logger, ProgressReporter } from './logging';
import { completeQuery, EvaluationResult, progress, ProgressMessage, WithProgressId } from './messages';
import * as messages from './messages';

type ServerOpts = {
  logger: Logger;
}

/** A running query server process and its associated message connection. */
class ServerProcess implements Disposable {
  child: cp.ChildProcess;
  connection: MessageConnection;
  logger: Logger;

  constructor(child: cp.ChildProcess, connection: MessageConnection, logger: Logger) {
    this.child = child;
    this.connection = connection;
    this.logger = logger;
  }

  dispose(): void {
    this.logger.log('Stopping query server...');
    this.connection.dispose();
    this.child.stdin!.end();
    this.child.stderr!.destroy();
    // TODO kill the process if it doesn't terminate after a certain time limit.

    // On Windows, we usually have to terminate the process before closing its stdout.
    this.child.stdout!.destroy();
    this.logger.log('Stopped query server.');
  }
}

type WithProgressReporting = (task: (progress: ProgressReporter, token: CancellationToken) => Thenable<void>) => Thenable<void>;

/**
 * Client that manages a query server process.
 * The server process is started upon initialization and tracked during its lifetime.
 * The server process is disposed when the client is disposed, or if the client asks
 * to restart it (which disposes the existing process and starts a new one).
 */
export class QueryServerClient extends DisposableObject {
  serverProcess?: ServerProcess;
  evaluationResultCallbacks: { [key: number]: (res: EvaluationResult) => void };
  progressCallbacks: { [key: number]: ((res: ProgressMessage) => void) | undefined };
  nextCallback: number;
  nextProgress: number;
  withProgressReporting: WithProgressReporting;
  public activeQueryName: string | undefined;

  constructor(readonly config: QueryServerConfig, readonly cliServer: cli.CodeQLCliServer, readonly opts: ServerOpts, withProgressReporting: WithProgressReporting) {
    super();
    // When the query server configuration changes, restart the query server.
    if (config.onDidChangeConfiguration !== undefined) {
      this.push(config.onDidChangeConfiguration(async () => {
        this.logger.log('Restarting query server due to configuration changes...');
        await this.restartQueryServer();
      }, this));
    }
    this.withProgressReporting = withProgressReporting;
    this.nextCallback = 0;
    this.nextProgress = 0;
    this.progressCallbacks = {};
    this.evaluationResultCallbacks = {};
  }

  get logger(): Logger {
    return this.opts.logger;
  }

  /** Stops the query server by disposing of the current server process. */
  private stopQueryServer(): void {
    if (this.serverProcess !== undefined) {
      this.disposeAndStopTracking(this.serverProcess);
    } else {
      this.logger.log('No server process to be stopped.');
    }
  }

  /** Restarts the query server by disposing of the current server process and then starting a new one. */
  async restartQueryServer(): Promise<void> {
    this.stopQueryServer();
    await this.startQueryServer();
  }

  showLog(): void {
    this.logger.show();
  }

  /** Starts a new query server process, sending progress messages to the status bar. */
  async startQueryServer(): Promise<void> {
    // Use an arrow function to preserve the value of `this`.
    return this.withProgressReporting((progress, _) => this.startQueryServerImpl(progress));
  }

  /** Starts a new query server process, sending progress messages to the given reporter. */
  private async startQueryServerImpl(progressReporter: ProgressReporter): Promise<void> {
    const ramArgs = await this.cliServer.resolveRam(this.config.queryMemoryMb, progressReporter);
    const args = ['--threads', this.config.numThreads.toString()].concat(ramArgs);
    if (this.config.debug) {
      args.push('--debug', '--tuple-counting');
    }
    const child = cli.spawnServer(
      this.config.codeQlPath,
      'CodeQL query server',
      ['execute', 'query-server'],
      args,
      this.logger,
      data => this.logger.log(data.toString(), {
        trailingNewline: false,
        additionalLogLocation: this.activeQueryName
      }),
      undefined, // no listener for stdout
      progressReporter
    );
    progressReporter.report({ message: 'Connecting to CodeQL query server' });
    const connection = createMessageConnection(child.stdout, child.stdin);
    connection.onRequest(completeQuery, res => {
      if (!(res.runId in this.evaluationResultCallbacks)) {
        this.logger.log(`No callback associated with run id ${res.runId}, continuing without executing any callback`);
      }
      else {
        const baseLocation = this.logger.getBaseLocation();
        if (baseLocation && this.activeQueryName) {
          res.logFileLocation = path.join(baseLocation, this.activeQueryName);
        }
        this.evaluationResultCallbacks[res.runId](res);
      }
      return {};
    });
    connection.onNotification(progress, res => {
      const callback = this.progressCallbacks[res.id];
      if (callback) {
        callback(res);
      }
    });
    this.serverProcess = new ServerProcess(child, connection, this.opts.logger);
    // Ensure the server process is disposed together with this client.
    this.track(this.serverProcess);
    connection.listen();
    progressReporter.report({ message: 'Connected to CodeQL query server' });
    this.nextCallback = 0;
    this.nextProgress = 0;
    this.progressCallbacks = {};
    this.evaluationResultCallbacks = {};
  }

  registerCallback(callback: (res: EvaluationResult) => void): number {
    const id = this.nextCallback++;
    this.evaluationResultCallbacks[id] = callback;
    return id;
  }

  unRegisterCallback(id: number): void {
    delete this.evaluationResultCallbacks[id];
  }

  get serverProcessPid(): number {
    return this.serverProcess!.child.pid;
  }

  async sendRequest<P, R, E, RO>(type: RequestType<WithProgressId<P>, R, E, RO>, parameter: P, token?: CancellationToken, progress?: (res: ProgressMessage) => void): Promise<R> {
    const id = this.nextProgress++;
    this.progressCallbacks[id] = progress;

    this.updateActiveQuery(type.method, parameter);
    try {
      if (this.serverProcess === undefined) {
        throw new Error('No query server process found.');
      }
      return await this.serverProcess.connection.sendRequest(type, { body: parameter, progressId: id }, token);
    } finally {
      delete this.progressCallbacks[id];
    }
  }

  /**
   * Updates the active query every time there is a new request to compile.
   * The active query is used to specify the side log.
   *
   * This isn't ideal because in situations where there are queries running
   * in parallel, each query's log messages are interleaved. Fixing this
   * properly will require a change in the query server.
   */
  private updateActiveQuery(method: string, parameter: any): void {
    if (method === messages.compileQuery.method) {
      const queryPath = parameter?.queryToCheck?.queryPath || 'unknown';
      this.activeQueryName = `query-${path.basename(queryPath)}-${this.nextProgress}.log`;
    }
  }
}
