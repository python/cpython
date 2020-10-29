import { window as Window, OutputChannel, Progress, Disposable } from 'vscode';
import { DisposableObject } from './vscode-utils/disposable-object';
import * as fs from 'fs-extra';
import * as path from 'path';

interface LogOptions {
  /** If false, don't output a trailing newline for the log entry. Default true. */
  trailingNewline?: boolean;

  /** If specified, add this log entry to the log file at the specified location. */
  additionalLogLocation?: string;
}

export interface Logger {
  /** Writes the given log message, optionally followed by a newline. */
  log(message: string, options?: LogOptions): Promise<void>;
  /**
   * Reveal this channel in the UI.
   *
   * @param preserveFocus When `true` the channel will not take focus.
   */
  show(preserveFocus?: boolean): void;

  /**
   * Remove the log at the specified location
   * @param location log to remove
   */
  removeAdditionalLogLocation(location: string | undefined): void;

  /**
   * The base location location where all side log files are stored.
   */
  getBaseLocation(): string | undefined;
}

export type ProgressReporter = Progress<{ message: string }>;

/** A logger that writes messages to an output channel in the Output tab. */
export class OutputChannelLogger extends DisposableObject implements Logger {
  public readonly outputChannel: OutputChannel;
  private readonly additionalLocations = new Map<string, AdditionalLogLocation>();
  private additionalLogLocationPath: string | undefined;

  constructor(private title: string) {
    super();
    this.outputChannel = Window.createOutputChannel(title);
    this.push(this.outputChannel);
  }

  init(storagePath: string): void {
    this.additionalLogLocationPath = path.join(storagePath, this.title);

    // clear out any old state from previous runs
    fs.remove(this.additionalLogLocationPath);
  }

  /**
   * This function is asynchronous and will only resolve once the message is written
   * to the side log (if required). It is not necessary to await the results of this
   * function if you don't need to guarantee that the log writing is complete before
   * continuing.
   */
  async log(message: string, options = {} as LogOptions): Promise<void> {
    if (options.trailingNewline === undefined) {
      options.trailingNewline = true;
    }

    if (options.trailingNewline) {
      this.outputChannel.appendLine(message);
    } else {
      this.outputChannel.append(message);
    }

    if (this.additionalLogLocationPath && options.additionalLogLocation) {
      const logPath = path.join(this.additionalLogLocationPath, options.additionalLogLocation);
      let additional = this.additionalLocations.get(logPath);
      if (!additional) {
        const msg = `| Log being saved to ${logPath} |`;
        const separator = new Array(msg.length).fill('-').join('');
        this.outputChannel.appendLine(separator);
        this.outputChannel.appendLine(msg);
        this.outputChannel.appendLine(separator);
        additional = new AdditionalLogLocation(logPath);
        this.additionalLocations.set(logPath, additional);
        this.track(additional);
      }

      await additional.log(message, options);
    }
  }

  show(preserveFocus?: boolean): void {
    this.outputChannel.show(preserveFocus);
  }

  removeAdditionalLogLocation(location: string | undefined): void {
    if (this.additionalLogLocationPath && location) {
      const logPath = location.startsWith(this.additionalLogLocationPath)
        ? location
        : path.join(this.additionalLogLocationPath, location);
      const additional = this.additionalLocations.get(logPath);
      if (additional) {
        this.disposeAndStopTracking(additional);
        this.additionalLocations.delete(logPath);
      }
    }
  }

  getBaseLocation() {
    return this.additionalLogLocationPath;
  }
}

class AdditionalLogLocation extends Disposable {
  constructor(private location: string) {
    super(() => { /**/ });
  }

  async log(message: string, options = {} as LogOptions): Promise<void> {
    if (options.trailingNewline === undefined) {
      options.trailingNewline = true;
    }
    await fs.ensureFile(this.location);

    await fs.appendFile(this.location, message + (options.trailingNewline ? '\n' : ''), {
      encoding: 'utf8'
    });
  }

  async dispose(): Promise<void> {
    await fs.remove(this.location);
  }
}

/** The global logger for the extension. */
export const logger = new OutputChannelLogger('CodeQL Extension Log');

/** The logger for messages from the query server. */
export const queryServerLogger = new OutputChannelLogger('CodeQL Query Server');

/** The logger for messages from the language server. */
export const ideServerLogger = new OutputChannelLogger(
  'CodeQL Language Server'
);

/** The logger for messages from tests. */
export const testLogger = new OutputChannelLogger('CodeQL Tests');
