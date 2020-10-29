import * as fs from 'fs-extra';
import * as glob from 'glob-promise';
import * as path from 'path';
import * as vscode from 'vscode';
import * as cli from './cli';
import { ExtensionContext } from 'vscode';
import { showAndLogErrorMessage, showAndLogWarningMessage, showAndLogInformationMessage } from './helpers';
import { zipArchiveScheme, encodeArchiveBasePath, decodeSourceArchiveUri, encodeSourceArchiveUri } from './archive-filesystem-provider';
import { DisposableObject } from './vscode-utils/disposable-object';
import { QueryServerConfig } from './config';
import { Logger, logger } from './logging';

/**
 * databases.ts
 * ------------
 * Managing state of what the current database is, and what other
 * databases have been recently selected.
 *
 * The source of truth of the current state resides inside the
 * `DatabaseManager` class below.
 */

/**
 * The name of the key in the workspaceState dictionary in which we
 * persist the current database across sessions.
 */
const CURRENT_DB = 'currentDatabase';

/**
 * The name of the key in the workspaceState dictionary in which we
 * persist the list of databases across sessions.
 */
const DB_LIST = 'databaseList';

export interface DatabaseOptions {
  displayName?: string;
  ignoreSourceArchive?: boolean;
  dateAdded?: number | undefined;
}

interface FullDatabaseOptions extends DatabaseOptions {
  ignoreSourceArchive: boolean;
  dateAdded: number | undefined;
}

interface PersistedDatabaseItem {
  uri: string;
  options?: DatabaseOptions;
}

/**
 * The layout of the database.
 */
export enum DatabaseKind {
  /** A CodeQL database */
  Database,
  /** A raw QL dataset */
  RawDataset
}

export interface DatabaseContents {
  /** The layout of the database */
  kind: DatabaseKind;
  /**
   * The name of the database.
   */
  name: string;
  /** The URI of the QL dataset within the database. */
  datasetUri: vscode.Uri;
  /** The URI of the source archive within the database, if one exists. */
  sourceArchiveUri?: vscode.Uri;
  /** The URI of the CodeQL database scheme within the database, if exactly one exists. */
  dbSchemeUri?: vscode.Uri;
}

/**
 * An error thrown when we cannot find a valid database in a putative
 * database directory.
 */
class InvalidDatabaseError extends Error {
}


async function findDataset(parentDirectory: string): Promise<vscode.Uri> {
  /*
   * Look directly in the root
   */
  let dbRelativePaths = await glob('db-*/', {
    cwd: parentDirectory
  });

  if (dbRelativePaths.length === 0) {
    /*
     * Check If they are in the old location
     */
    dbRelativePaths = await glob('working/db-*/', {
      cwd: parentDirectory
    });
  }
  if (dbRelativePaths.length === 0) {
    throw new InvalidDatabaseError(`'${parentDirectory}' does not contain a dataset directory.`);
  }

  const dbAbsolutePath = path.join(parentDirectory, dbRelativePaths[0]);
  if (dbRelativePaths.length > 1) {
    showAndLogWarningMessage(`Found multiple dataset directories in database, using '${dbAbsolutePath}'.`);
  }

  return vscode.Uri.file(dbAbsolutePath);
}

async function findSourceArchive(
  databasePath: string, silent = false
): Promise<vscode.Uri | undefined> {

  const relativePaths = ['src', 'output/src_archive'];

  for (const relativePath of relativePaths) {
    const basePath = path.join(databasePath, relativePath);
    const zipPath = basePath + '.zip';

    if (await fs.pathExists(basePath)) {
      return vscode.Uri.file(basePath);
    } else if (await fs.pathExists(zipPath)) {
      return encodeArchiveBasePath(zipPath);
    }
  }
  if (!silent) {
    showAndLogInformationMessage(
      `Could not find source archive for database '${databasePath}'. Assuming paths are absolute.`
    );
  }
  return undefined;
}

async function resolveDatabase(
  databasePath: string
): Promise<DatabaseContents> {

  const name = path.basename(databasePath);

  // Look for dataset and source archive.
  const datasetUri = await findDataset(databasePath);
  const sourceArchiveUri = await findSourceArchive(databasePath);

  return {
    kind: DatabaseKind.Database,
    name,
    datasetUri,
    sourceArchiveUri
  };

}

/** Gets the relative paths of all `.dbscheme` files in the given directory. */
async function getDbSchemeFiles(dbDirectory: string): Promise<string[]> {
  return await glob('*.dbscheme', { cwd: dbDirectory });
}

async function resolveDatabaseContents(uri: vscode.Uri): Promise<DatabaseContents> {
  if (uri.scheme !== 'file') {
    throw new Error(`Database URI scheme '${uri.scheme}' not supported; only 'file' URIs are supported.`);
  }
  const databasePath = uri.fsPath;
  if (!await fs.pathExists(databasePath)) {
    throw new InvalidDatabaseError(`Database '${databasePath}' does not exist.`);
  }

  const contents = await resolveDatabase(databasePath);

  if (contents === undefined) {
    throw new InvalidDatabaseError(`'${databasePath}' is not a valid database.`);
  }

  // Look for a single dbscheme file within the database.
  // This should be found in the dataset directory, regardless of the form of database.
  const dbPath = contents.datasetUri.fsPath;
  const dbSchemeFiles = await getDbSchemeFiles(dbPath);
  if (dbSchemeFiles.length === 0) {
    throw new InvalidDatabaseError(`Database '${databasePath}' does not contain a CodeQL dbscheme under '${dbPath}'.`);
  }
  else if (dbSchemeFiles.length > 1) {
    throw new InvalidDatabaseError(`Database '${databasePath}' contains multiple CodeQL dbschemes under '${dbPath}'.`);
  } else {
    contents.dbSchemeUri = vscode.Uri.file(path.resolve(dbPath, dbSchemeFiles[0]));
  }
  return contents;
}

/** An item in the list of available databases */
export interface DatabaseItem {
  /** The URI of the database */
  readonly databaseUri: vscode.Uri;
  /** The name of the database to be displayed in the UI */
  name: string;
  /** The URI of the database's source archive, or `undefined` if no source archive is to be used. */
  readonly sourceArchive: vscode.Uri | undefined;
  /**
   * The contents of the database.
   * Will be `undefined` if the database is invalid. Can be updated by calling `refresh()`.
   */
  readonly contents: DatabaseContents | undefined;

  /**
   * The date this database was added as a unix timestamp. Or undefined if we don't know.
   */
  readonly dateAdded: number | undefined;

  /** If the database is invalid, describes why. */
  readonly error: Error | undefined;
  /**
   * Resolves the contents of the database.
   *
   * @remarks
   * The contents include the database directory, source archive, and metadata about the database.
   * If the database is invalid, `this.error` is updated with the error object that describes why
   * the database is invalid. This error is also thrown.
   */
  refresh(): Promise<void>;
  /**
   * Resolves a filename to its URI in the source archive.
   *
   * @param file Filename within the source archive. May be `undefined` to return a dummy file path.
   */
  resolveSourceFile(file: string | undefined): vscode.Uri;

  /**
   * Holds if the database item has a `.dbinfo` or `codeql-database.yml` file.
   */
  hasMetadataFile(): Promise<boolean>;

  /**
   * Returns `sourceLocationPrefix` of exported database.
   */
  getSourceLocationPrefix(server: cli.CodeQLCliServer): Promise<string>;

  /**
   * Returns dataset folder of exported database.
   */
  getDatasetFolder(server: cli.CodeQLCliServer): Promise<string>;

  /**
   * Returns the root uri of the virtual filesystem for this database's source archive,
   * as displayed in the filesystem explorer.
   */
  getSourceArchiveExplorerUri(): vscode.Uri | undefined;

  /**
   * Holds if `uri` belongs to this database's source archive.
   */
  belongsToSourceArchiveExplorerUri(uri: vscode.Uri): boolean;
}

export enum DatabaseEventKind {
  Add = 'Add',
  Remove = 'Remove',

  // Fired when databases are refreshed from persisted state
  Refresh = 'Refresh',

  // Fired when the current database changes
  Change = 'Change',

  Rename = 'Rename'
}

export interface DatabaseChangedEvent {
  kind: DatabaseEventKind;
  item: DatabaseItem | undefined;
}

// Exported for testing
export class DatabaseItemImpl implements DatabaseItem {
  private _error: Error | undefined = undefined;
  private _contents: DatabaseContents | undefined;
  /** A cache of database info */
  private _dbinfo: cli.DbInfo | undefined;

  public constructor(
    public readonly databaseUri: vscode.Uri,
    contents: DatabaseContents | undefined,
    private options: FullDatabaseOptions,
    private readonly onChanged: (event: DatabaseChangedEvent) => void
  ) {
    this._contents = contents;
  }

  public get name(): string {
    if (this.options.displayName) {
      return this.options.displayName;
    }
    else if (this._contents) {
      return this._contents.name;
    }
    else {
      return path.basename(this.databaseUri.fsPath);
    }
  }

  public set name(newName: string) {
    this.options.displayName = newName;
  }

  public get sourceArchive(): vscode.Uri | undefined {
    if (this.options.ignoreSourceArchive || (this._contents === undefined)) {
      return undefined;
    } else {
      return this._contents.sourceArchiveUri;
    }
  }

  public get contents(): DatabaseContents | undefined {
    return this._contents;
  }

  public get dateAdded(): number | undefined {
    return this.options.dateAdded;
  }

  public get error(): Error | undefined {
    return this._error;
  }

  public async refresh(): Promise<void> {
    try {
      try {
        this._contents = await resolveDatabaseContents(this.databaseUri);
        this._error = undefined;
      }
      catch (e) {
        this._contents = undefined;
        this._error = e;
        throw e;
      }
    }
    finally {
      this.onChanged({
        kind: DatabaseEventKind.Refresh,
        item: this
      });
    }
  }

  public resolveSourceFile(uri: string | undefined): vscode.Uri {
    const sourceArchive = this.sourceArchive;
    // Sometimes, we are passed a path, sometimes a file URI.
    // We need to convert this to a file path that is probably inside of a zip file.
    const file = uri?.replace(/file:/, '');
    if (!sourceArchive) {
      if (file) {
        // Treat it as an absolute path.
        return vscode.Uri.file(file);
      } else {
        return this.databaseUri;
      }
    }

    if (file) {
      const absoluteFilePath = file.replace(':', '_');
      // Strip any leading slashes from the file path, and replace `:` with `_`.
      const relativeFilePath = absoluteFilePath.replace(/^\/*/, '').replace(':', '_');
      if (sourceArchive.scheme === zipArchiveScheme) {
        const zipRef = decodeSourceArchiveUri(sourceArchive);
        return encodeSourceArchiveUri({
          pathWithinSourceArchive: zipRef.pathWithinSourceArchive + '/' + absoluteFilePath,
          sourceArchiveZipPath: zipRef.sourceArchiveZipPath,
        });

      } else {
        let newPath = sourceArchive.path;
        if (!newPath.endsWith('/')) {
          // Ensure a trailing slash.
          newPath += '/';
        }
        newPath += relativeFilePath;

        return sourceArchive.with({ path: newPath });
      }

    } else {
      return sourceArchive;
    }
  }

  /**
   * Gets the state of this database, to be persisted in the workspace state.
   */
  public getPersistedState(): PersistedDatabaseItem {
    return {
      uri: this.databaseUri.toString(true),
      options: this.options
    };
  }

  /**
   * Holds if the database item refers to an exported snapshot
   */
  public async hasMetadataFile(): Promise<boolean> {
    return (await Promise.all([
      fs.pathExists(path.join(this.databaseUri.fsPath, '.dbinfo')),
      fs.pathExists(path.join(this.databaseUri.fsPath, 'codeql-database.yml'))
    ])).some(x => x);
  }

  /**
   * Returns information about a database.
   */
  private async getDbInfo(server: cli.CodeQLCliServer): Promise<cli.DbInfo> {
    if (this._dbinfo === undefined) {
      this._dbinfo = await server.resolveDatabase(this.databaseUri.fsPath);
    }
    return this._dbinfo;
  }

  /**
   * Returns `sourceLocationPrefix` of database. Requires that the database
   * has a `.dbinfo` file, which is the source of the prefix.
   */
  public async getSourceLocationPrefix(server: cli.CodeQLCliServer): Promise<string> {
    const dbInfo = await this.getDbInfo(server);
    return dbInfo.sourceLocationPrefix;
  }

  /**
   * Returns path to dataset folder of database.
   */
  public async getDatasetFolder(server: cli.CodeQLCliServer): Promise<string> {
    const dbInfo = await this.getDbInfo(server);
    return dbInfo.datasetFolder;
  }

  /**
   * Returns the root uri of the virtual filesystem for this database's source archive.
   */
  public getSourceArchiveExplorerUri(): vscode.Uri | undefined {
    const sourceArchive = this.sourceArchive;
    if (sourceArchive === undefined || !sourceArchive.fsPath.endsWith('.zip'))
      return undefined;
    return encodeArchiveBasePath(sourceArchive.fsPath);
  }

  /**
   * Holds if `uri` belongs to this database's source archive.
   */
  public belongsToSourceArchiveExplorerUri(uri: vscode.Uri): boolean {
    if (this.sourceArchive === undefined)
      return false;
    return uri.scheme === zipArchiveScheme &&
      decodeSourceArchiveUri(uri).sourceArchiveZipPath === this.sourceArchive.fsPath;
  }
}

/**
 * A promise that resolves to an event's result value when the event
 * `event` fires. If waiting for the event takes too long (by default
 * >1000ms) log a warning, and resolve to undefined.
 */
function eventFired<T>(event: vscode.Event<T>, timeoutMs = 1000): Promise<T | undefined> {
  return new Promise((res, _rej) => {
    const timeout = setTimeout(() => {
      logger.log(`Waiting for event ${event} timed out after ${timeoutMs}ms`);
      res(undefined);
      dispose();
    }, timeoutMs);
    const disposable = event(e => {
      res(e);
      dispose();
    });
    function dispose() {
      clearTimeout(timeout);
      disposable.dispose();
    }
  });
}

export class DatabaseManager extends DisposableObject {
  private readonly _onDidChangeDatabaseItem = this.push(new vscode.EventEmitter<DatabaseChangedEvent>());

  readonly onDidChangeDatabaseItem = this._onDidChangeDatabaseItem.event;

  private readonly _onDidChangeCurrentDatabaseItem = this.push(new vscode.EventEmitter<DatabaseChangedEvent>());
  readonly onDidChangeCurrentDatabaseItem = this._onDidChangeCurrentDatabaseItem.event;

  private readonly _databaseItems: DatabaseItemImpl[] = [];
  private _currentDatabaseItem: DatabaseItem | undefined = undefined;

  constructor(
    private ctx: ExtensionContext,
    public config: QueryServerConfig,
    public logger: Logger
  ) {
    super();

    this.loadPersistedState();  // Let this run async.
  }

  public async openDatabase(
    uri: vscode.Uri, options?: DatabaseOptions
  ): Promise<DatabaseItem> {

    const contents = await resolveDatabaseContents(uri);
    const realOptions = options || {};
    // Ignore the source archive for QLTest databases by default.
    const isQLTestDatabase = path.extname(uri.fsPath) === '.testproj';
    const fullOptions: FullDatabaseOptions = {
      ignoreSourceArchive: (realOptions.ignoreSourceArchive !== undefined) ?
        realOptions.ignoreSourceArchive : isQLTestDatabase,
      displayName: realOptions.displayName,
      dateAdded: realOptions.dateAdded || Date.now()
    };
    const databaseItem = new DatabaseItemImpl(uri, contents, fullOptions, (event) => {
      this._onDidChangeDatabaseItem.fire(event);
    });
    await this.addDatabaseItem(databaseItem);
    await this.addDatabaseSourceArchiveFolder(databaseItem);

    return databaseItem;
  }

  private async addDatabaseSourceArchiveFolder(item: DatabaseItem) {
    // The folder may already be in workspace state from a previous
    // session. If not, add it.
    const index = this.getDatabaseWorkspaceFolderIndex(item);
    if (index === -1) {
      // Add that filesystem as a folder to the current workspace.
      //
      // It's important that we add workspace folders to the end,
      // rather than beginning of the list, because the first
      // workspace folder is special; if it gets updated, the entire
      // extension host is restarted. (cf.
      // https://github.com/microsoft/vscode/blob/e0d2ed907d1b22808c56127678fb436d604586a7/src/vs/workbench/contrib/relauncher/browser/relauncher.contribution.ts#L209-L214)
      //
      // This is undesirable, as we might be adding and removing many
      // workspace folders as the user adds and removes databases.
      const end = (vscode.workspace.workspaceFolders || []).length;
      const uri = item.getSourceArchiveExplorerUri();
      if (uri === undefined) {
        logger.log(`Couldn't obtain file explorer uri for ${item.name}`);
      }
      else {
        logger.log(`Adding workspace folder for ${item.name} source archive at index ${end}`);
        if ((vscode.workspace.workspaceFolders || []).length < 2) {
          // Adding this workspace folder makes the workspace
          // multi-root, which may surprise the user. Let them know
          // we're doing this.
          vscode.window.showInformationMessage(`Adding workspace folder for source archive of database ${item.name}.`);
        }
        vscode.workspace.updateWorkspaceFolders(end, 0, {
          name: `[${item.name} source archive]`,
          uri,
        });
        // vscode api documentation says we must to wait for this event
        // between multiple `updateWorkspaceFolders` calls.
        await eventFired(vscode.workspace.onDidChangeWorkspaceFolders);
      }
    }
  }

  private async createDatabaseItemFromPersistedState(
    state: PersistedDatabaseItem
  ): Promise<DatabaseItem> {

    let displayName: string | undefined = undefined;
    let ignoreSourceArchive = false;
    let dateAdded = undefined;
    if (state.options) {
      if (typeof state.options.displayName === 'string') {
        displayName = state.options.displayName;
      }
      if (typeof state.options.ignoreSourceArchive === 'boolean') {
        ignoreSourceArchive = state.options.ignoreSourceArchive;
      }
      if (typeof state.options.dateAdded === 'number') {
        dateAdded = state.options.dateAdded;
      }
    }
    const fullOptions: FullDatabaseOptions = {
      ignoreSourceArchive,
      displayName,
      dateAdded
    };
    const item = new DatabaseItemImpl(vscode.Uri.parse(state.uri), undefined, fullOptions,
      (event) => {
        this._onDidChangeDatabaseItem.fire(event);
      });
    await this.addDatabaseItem(item);

    return item;
  }

  private async loadPersistedState(): Promise<void> {
    const currentDatabaseUri = this.ctx.workspaceState.get<string>(CURRENT_DB);
    const databases = this.ctx.workspaceState.get<PersistedDatabaseItem[]>(DB_LIST, []);

    try {
      for (const database of databases) {
        const databaseItem = await this.createDatabaseItemFromPersistedState(database);
        try {
          await databaseItem.refresh();
          if (currentDatabaseUri === database.uri) {
            this.setCurrentDatabaseItem(databaseItem, true);
          }
        }
        catch (e) {
          // When loading from persisted state, leave invalid databases in the list. They will be
          // marked as invalid, and cannot be set as the current database.
        }
      }
    } catch (e) {
      // database list had an unexpected type - nothing to be done?
      showAndLogErrorMessage(`Database list loading failed: ${e.message}`);
    }
  }

  public get databaseItems(): readonly DatabaseItem[] {
    return this._databaseItems;
  }

  public get currentDatabaseItem(): DatabaseItem | undefined {
    return this._currentDatabaseItem;
  }

  public async setCurrentDatabaseItem(item: DatabaseItem | undefined,
    skipRefresh = false): Promise<void> {

    if (!skipRefresh && (item !== undefined)) {
      await item.refresh();  // Will throw on invalid database.
    }
    if (this._currentDatabaseItem !== item) {
      this._currentDatabaseItem = item;
      this.updatePersistedCurrentDatabaseItem();
      this._onDidChangeCurrentDatabaseItem.fire({
        item,
        kind: DatabaseEventKind.Change
      });
    }
  }

  /**
   * Returns the index of the workspace folder that corresponds to the source archive of `item`
   * if there is one, and -1 otherwise.
   */
  private getDatabaseWorkspaceFolderIndex(item: DatabaseItem): number {
    return (vscode.workspace.workspaceFolders || [])
      .findIndex(folder => item.belongsToSourceArchiveExplorerUri(folder.uri));
  }

  public findDatabaseItem(uri: vscode.Uri): DatabaseItem | undefined {
    const uriString = uri.toString(true);
    return this._databaseItems.find(item => item.databaseUri.toString(true) === uriString);
  }

  public findDatabaseItemBySourceArchive(uri: vscode.Uri): DatabaseItem | undefined {
    const uriString = uri.toString(true);
    return this._databaseItems.find(item => item.sourceArchive && item.sourceArchive.toString(true) === uriString);
  }

  private async addDatabaseItem(item: DatabaseItemImpl) {
    this._databaseItems.push(item);
    this.updatePersistedDatabaseList();
    // note that we use undefined as the item in order to reset the entire tree
    this._onDidChangeDatabaseItem.fire({
      item: undefined,
      kind: DatabaseEventKind.Add
    });
  }

  public async renameDatabaseItem(item: DatabaseItem, newName: string) {
    item.name = newName;
    this.updatePersistedDatabaseList();
    this._onDidChangeDatabaseItem.fire({
      item,
      kind: DatabaseEventKind.Rename
    });
  }

  public removeDatabaseItem(item: DatabaseItem) {
    if (this._currentDatabaseItem == item)
      this._currentDatabaseItem = undefined;
    const index = this.databaseItems.findIndex(searchItem => searchItem === item);
    if (index >= 0) {
      this._databaseItems.splice(index, 1);
    }
    this.updatePersistedDatabaseList();

    // Delete folder from workspace, if it is still there
    const folderIndex = (vscode.workspace.workspaceFolders || []).findIndex(folder => item.belongsToSourceArchiveExplorerUri(folder.uri));
    if (index >= 0) {
      logger.log(`Removing workspace folder at index ${folderIndex}`);
      vscode.workspace.updateWorkspaceFolders(folderIndex, 1);
    }

    // Delete folder from file system only if it is controlled by the extension
    if (this.isExtensionControlledLocation(item.databaseUri)) {
      logger.log('Deleting database from filesystem.');
      fs.remove(item.databaseUri.path).then(
        () => logger.log(`Deleted '${item.databaseUri.path}'`),
        e => logger.log(`Failed to delete '${item.databaseUri.path}'. Reason: ${e.message}`));
    }

    // note that we use undefined as the item in order to reset the entire tree
    this._onDidChangeDatabaseItem.fire({
      item: undefined,
      kind: DatabaseEventKind.Remove
    });
  }

  private updatePersistedCurrentDatabaseItem(): void {
    this.ctx.workspaceState.update(CURRENT_DB, this._currentDatabaseItem ?
      this._currentDatabaseItem.databaseUri.toString(true) : undefined);
  }

  private updatePersistedDatabaseList(): void {
    this.ctx.workspaceState.update(DB_LIST, this._databaseItems.map(item => item.getPersistedState()));
  }

  private isExtensionControlledLocation(uri: vscode.Uri) {
    const storagePath = this.ctx.storagePath || this.ctx.globalStoragePath;
    return uri.path.startsWith(storagePath);
  }
}

/**
 * Get the set of directories containing upgrades, given a list of
 * scripts returned by the cli's upgrade resolution.
 */
export function getUpgradesDirectories(scripts: string[]): vscode.Uri[] {
  const parentDirs = scripts.map(dir => path.dirname(dir));
  const uniqueParentDirs = new Set(parentDirs);
  return Array.from(uniqueParentDirs).map(filePath => vscode.Uri.file(filePath));
}
