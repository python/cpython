import { DisposableObject } from './vscode-utils/disposable-object';
import { workspace, Event, EventEmitter, ConfigurationChangeEvent, ConfigurationTarget } from 'vscode';
import { DistributionManager } from './distribution';
import { logger } from './logging';

/** Helper class to look up a labelled (and possibly nested) setting. */
class Setting {
  name: string;
  parent?: Setting;

  constructor(name: string, parent?: Setting) {
    this.name = name;
    this.parent = parent;
  }

  get qualifiedName(): string {
    if (this.parent === undefined) {
      return this.name;
    } else {
      return `${this.parent.qualifiedName}.${this.name}`;
    }
  }

  getValue<T>(): T {
    if (this.parent === undefined) {
      throw new Error('Cannot get the value of a root setting.');
    }
    return workspace.getConfiguration(this.parent.qualifiedName).get<T>(this.name)!;
  }

  updateValue<T>(value: T, target: ConfigurationTarget): Thenable<void> {
    if (this.parent === undefined) {
      throw new Error('Cannot update the value of a root setting.');
    }
    return workspace.getConfiguration(this.parent.qualifiedName).update(this.name, value, target);
  }

}

const ROOT_SETTING = new Setting('codeQL');

// Distribution configuration

const DISTRIBUTION_SETTING = new Setting('cli', ROOT_SETTING);
const CUSTOM_CODEQL_PATH_SETTING = new Setting('executablePath', DISTRIBUTION_SETTING);
const INCLUDE_PRERELEASE_SETTING = new Setting('includePrerelease', DISTRIBUTION_SETTING);
const PERSONAL_ACCESS_TOKEN_SETTING = new Setting('personalAccessToken', DISTRIBUTION_SETTING);
const QUERY_HISTORY_SETTING = new Setting('queryHistory', ROOT_SETTING);
const QUERY_HISTORY_FORMAT_SETTING = new Setting('format', QUERY_HISTORY_SETTING);

/** When these settings change, the distribution should be updated. */
const DISTRIBUTION_CHANGE_SETTINGS = [CUSTOM_CODEQL_PATH_SETTING, INCLUDE_PRERELEASE_SETTING, PERSONAL_ACCESS_TOKEN_SETTING];

export interface DistributionConfig {
  customCodeQlPath?: string;
  includePrerelease: boolean;
  personalAccessToken?: string;
  ownerName?: string;
  repositoryName?: string;
  onDidChangeConfiguration?: Event<void>;
}

// Query server configuration

const RUNNING_QUERIES_SETTING = new Setting('runningQueries', ROOT_SETTING);
const NUMBER_OF_THREADS_SETTING = new Setting('numberOfThreads', RUNNING_QUERIES_SETTING);
const TIMEOUT_SETTING = new Setting('timeout', RUNNING_QUERIES_SETTING);
const MEMORY_SETTING = new Setting('memory', RUNNING_QUERIES_SETTING);
const DEBUG_SETTING = new Setting('debug', RUNNING_QUERIES_SETTING);
const RUNNING_TESTS_SETTING = new Setting('runningTests', ROOT_SETTING);

export const NUMBER_OF_TEST_THREADS_SETTING = new Setting('numberOfThreads', RUNNING_TESTS_SETTING);
export const MAX_QUERIES = new Setting('maxQueries', RUNNING_QUERIES_SETTING);
export const AUTOSAVE_SETTING = new Setting('autoSave', RUNNING_QUERIES_SETTING);

/** When these settings change, the running query server should be restarted. */
const QUERY_SERVER_RESTARTING_SETTINGS = [NUMBER_OF_THREADS_SETTING, MEMORY_SETTING, DEBUG_SETTING];

export interface QueryServerConfig {
  codeQlPath: string;
  debug: boolean;
  numThreads: number;
  queryMemoryMb?: number;
  timeoutSecs: number;
  onDidChangeConfiguration?: Event<void>;
}

/** When these settings change, the query history should be refreshed. */
const QUERY_HISTORY_SETTINGS = [QUERY_HISTORY_FORMAT_SETTING];

export interface QueryHistoryConfig {
  format: string;
  onDidChangeConfiguration: Event<void>;
}

const CLI_SETTINGS = [NUMBER_OF_TEST_THREADS_SETTING];

export interface CliConfig {
  numberTestThreads: number;
  onDidChangeConfiguration?: Event<void>;
}


abstract class ConfigListener extends DisposableObject {
  protected readonly _onDidChangeConfiguration = this.push(new EventEmitter<void>());

  constructor() {
    super();
    this.updateConfiguration();
    this.push(workspace.onDidChangeConfiguration(this.handleDidChangeConfiguration, this));
  }

  /**
   * Calls `updateConfiguration` if any of the `relevantSettings` have changed.
   */
  protected handleDidChangeConfigurationForRelevantSettings(relevantSettings: Setting[], e: ConfigurationChangeEvent): void {
    // Check whether any options that affect query running were changed.
    for (const option of relevantSettings) {
      // TODO: compare old and new values, only update if there was actually a change?
      if (e.affectsConfiguration(option.qualifiedName)) {
        this.updateConfiguration();
        break; // only need to do this once, if any of the settings have changed
      }
    }
  }

  protected abstract handleDidChangeConfiguration(e: ConfigurationChangeEvent): void;
  private updateConfiguration(): void {
    this._onDidChangeConfiguration.fire();
  }

  public get onDidChangeConfiguration(): Event<void> {
    return this._onDidChangeConfiguration.event;
  }
}

export class DistributionConfigListener extends ConfigListener implements DistributionConfig {
  public get customCodeQlPath(): string | undefined {
    return CUSTOM_CODEQL_PATH_SETTING.getValue() || undefined;
  }

  public get includePrerelease(): boolean {
    return INCLUDE_PRERELEASE_SETTING.getValue();
  }

  public get personalAccessToken(): string | undefined {
    return PERSONAL_ACCESS_TOKEN_SETTING.getValue() || undefined;
  }

  protected handleDidChangeConfiguration(e: ConfigurationChangeEvent): void {
    this.handleDidChangeConfigurationForRelevantSettings(DISTRIBUTION_CHANGE_SETTINGS, e);
  }
}

export class QueryServerConfigListener extends ConfigListener implements QueryServerConfig {
  public constructor(private _codeQlPath = '') {
    super();
  }

  public static async createQueryServerConfigListener(distributionManager: DistributionManager): Promise<QueryServerConfigListener> {
    const codeQlPath = await distributionManager.getCodeQlPathWithoutVersionCheck();
    const config = new QueryServerConfigListener(codeQlPath!);
    if (distributionManager.onDidChangeDistribution) {
      config.push(distributionManager.onDidChangeDistribution(async () => {
        const codeQlPath = await distributionManager.getCodeQlPathWithoutVersionCheck();
        config._codeQlPath = codeQlPath!;
        config._onDidChangeConfiguration.fire();
      }));
    }
    return config;
  }

  public get codeQlPath(): string {
    return this._codeQlPath;
  }

  public get numThreads(): number {
    return NUMBER_OF_THREADS_SETTING.getValue<number>();
  }

  /** Gets the configured query timeout, in seconds. This looks up the setting at the time of access. */
  public get timeoutSecs(): number {
    return TIMEOUT_SETTING.getValue<number | null>() || 0;
  }

  public get queryMemoryMb(): number | undefined {
    const memory = MEMORY_SETTING.getValue<number | null>();
    if (memory === null) {
      return undefined;
    }
    if (memory == 0 || typeof (memory) !== 'number') {
      logger.log(`Ignoring value '${memory}' for setting ${MEMORY_SETTING.qualifiedName}`);
      return undefined;
    }
    return memory;
  }

  public get debug(): boolean {
    return DEBUG_SETTING.getValue<boolean>();
  }

  protected handleDidChangeConfiguration(e: ConfigurationChangeEvent): void {
    this.handleDidChangeConfigurationForRelevantSettings(QUERY_SERVER_RESTARTING_SETTINGS, e);
  }
}

export class QueryHistoryConfigListener extends ConfigListener implements QueryHistoryConfig {
  protected handleDidChangeConfiguration(e: ConfigurationChangeEvent): void {
    this.handleDidChangeConfigurationForRelevantSettings(QUERY_HISTORY_SETTINGS, e);
  }

  public get format(): string {
    return QUERY_HISTORY_FORMAT_SETTING.getValue<string>();
  }
}

export class CliConfigListener extends ConfigListener implements CliConfig {

  public get numberTestThreads(): number {
    return NUMBER_OF_TEST_THREADS_SETTING.getValue();
  }

  protected handleDidChangeConfiguration(e: ConfigurationChangeEvent): void {
    this.handleDidChangeConfigurationForRelevantSettings(CLI_SETTINGS, e);
  }
}

// Enable experimental features

/**
 * Any settings below are deliberately not in package.json so that
 * they do not appear in the settings ui in vscode itself. If users
 * want to enable experimental features, they can add them directly in
 * their vscode settings json file.
 */
