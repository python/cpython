import * as path from 'path';
import { DisposableObject } from './vscode-utils/disposable-object';
import {
  Event,
  EventEmitter,
  ProviderResult,
  TreeDataProvider,
  TreeItem,
  Uri,
  window,
  env
} from 'vscode';
import * as fs from 'fs-extra';

import * as cli from './cli';
import {
  DatabaseChangedEvent,
  DatabaseItem,
  DatabaseManager,
  getUpgradesDirectories,
} from './databases';
import {
  commandRunner,
  commandRunnerWithProgress,
  getOnDiskWorkspaceFolders,
  ProgressCallback,
  showAndLogErrorMessage
} from './helpers';
import { logger } from './logging';
import { clearCacheInDatabase } from './run-queries';
import * as qsClient from './queryserver-client';
import { upgradeDatabase } from './upgrades';
import {
  importArchiveDatabase,
  promptImportInternetDatabase,
  promptImportLgtmDatabase,
} from './databaseFetcher';
import { CancellationToken } from 'vscode-jsonrpc';

type ThemableIconPath = { light: string; dark: string } | string;

/**
 * Path to icons to display next to currently selected database.
 */
const SELECTED_DATABASE_ICON: ThemableIconPath = {
  light: 'media/light/check.svg',
  dark: 'media/dark/check.svg',
};

/**
 * Path to icon to display next to an invalid database.
 */
const INVALID_DATABASE_ICON: ThemableIconPath = 'media/red-x.svg';

function joinThemableIconPath(
  base: string,
  iconPath: ThemableIconPath
): ThemableIconPath {
  if (typeof iconPath == 'object')
    return {
      light: path.join(base, iconPath.light),
      dark: path.join(base, iconPath.dark),
    };
  else return path.join(base, iconPath);
}

enum SortOrder {
  NameAsc = 'NameAsc',
  NameDesc = 'NameDesc',
  DateAddedAsc = 'DateAddedAsc',
  DateAddedDesc = 'DateAddedDesc',
}

/**
 * Tree data provider for the databases view.
 */
class DatabaseTreeDataProvider extends DisposableObject
  implements TreeDataProvider<DatabaseItem> {
  private _sortOrder = SortOrder.NameAsc;

  private readonly _onDidChangeTreeData = new EventEmitter<DatabaseItem | undefined>();
  private currentDatabaseItem: DatabaseItem | undefined;

  constructor(
    private databaseManager: DatabaseManager,
    private readonly extensionPath: string
  ) {
    super();

    this.currentDatabaseItem = databaseManager.currentDatabaseItem;

    this.push(
      this.databaseManager.onDidChangeDatabaseItem(
        this.handleDidChangeDatabaseItem
      )
    );
    this.push(
      this.databaseManager.onDidChangeCurrentDatabaseItem(
        this.handleDidChangeCurrentDatabaseItem
      )
    );
  }

  public get onDidChangeTreeData(): Event<DatabaseItem | undefined> {
    return this._onDidChangeTreeData.event;
  }

  private handleDidChangeDatabaseItem = (event: DatabaseChangedEvent): void => {
    // Note that events from the databse manager are instances of DatabaseChangedEvent
    // and events fired by the UI are instances of DatabaseItem

    // When event.item is undefined, then the entire tree is refreshed.
    // When event.item is a db item, then only that item is refreshed.
    this._onDidChangeTreeData.fire(event.item);
  };

  private handleDidChangeCurrentDatabaseItem = (
    event: DatabaseChangedEvent
  ): void => {
    if (this.currentDatabaseItem) {
      this._onDidChangeTreeData.fire(this.currentDatabaseItem);
    }
    this.currentDatabaseItem = event.item;
    if (this.currentDatabaseItem) {
      this._onDidChangeTreeData.fire(this.currentDatabaseItem);
    }
  };

  public getTreeItem(element: DatabaseItem): TreeItem {
    const item = new TreeItem(element.name);
    if (element === this.currentDatabaseItem) {
      item.iconPath = joinThemableIconPath(
        this.extensionPath,
        SELECTED_DATABASE_ICON
      );
    } else if (element.error !== undefined) {
      item.iconPath = joinThemableIconPath(
        this.extensionPath,
        INVALID_DATABASE_ICON
      );
    }
    item.tooltip = element.databaseUri.fsPath;
    return item;
  }

  public getChildren(element?: DatabaseItem): ProviderResult<DatabaseItem[]> {
    if (element === undefined) {
      return this.databaseManager.databaseItems.slice(0).sort((db1, db2) => {
        switch (this.sortOrder) {
          case SortOrder.NameAsc:
            return db1.name.localeCompare(db2.name, env.language);
          case SortOrder.NameDesc:
            return db2.name.localeCompare(db1.name, env.language);
          case SortOrder.DateAddedAsc:
            return (db1.dateAdded || 0) - (db2.dateAdded || 0);
          case SortOrder.DateAddedDesc:
            return (db2.dateAdded || 0) - (db1.dateAdded || 0);
        }
      });
    } else {
      return [];
    }
  }

  public getParent(_element: DatabaseItem): ProviderResult<DatabaseItem> {
    return null;
  }

  public getCurrent(): DatabaseItem | undefined {
    return this.currentDatabaseItem;
  }

  public get sortOrder() {
    return this._sortOrder;
  }

  public set sortOrder(newSortOrder: SortOrder) {
    this._sortOrder = newSortOrder;
    this._onDidChangeTreeData.fire();
  }
}

/** Gets the first element in the given list, if any, or undefined if the list is empty or undefined. */
function getFirst(list: Uri[] | undefined): Uri | undefined {
  if (list === undefined || list.length === 0) {
    return undefined;
  } else {
    return list[0];
  }
}

/**
 * Displays file selection dialog. Expects the user to choose a
 * database directory, which should be the parent directory of a
 * directory of the form `db-[language]`, for example, `db-cpp`.
 *
 * XXX: no validation is done other than checking the directory name
 * to make sure it really is a database directory.
 */
async function chooseDatabaseDir(byFolder: boolean): Promise<Uri | undefined> {
  const chosen = await window.showOpenDialog({
    openLabel: byFolder ? 'Choose Database folder' : 'Choose Database archive',
    canSelectFiles: !byFolder,
    canSelectFolders: byFolder,
    canSelectMany: false,
    filters: byFolder ? {} : { Archives: ['zip'] },
  });
  return getFirst(chosen);
}

export class DatabaseUI extends DisposableObject {
  private treeDataProvider: DatabaseTreeDataProvider;

  public constructor(
    private cliserver: cli.CodeQLCliServer,
    private databaseManager: DatabaseManager,
    private readonly queryServer: qsClient.QueryServerClient | undefined,
    private readonly storagePath: string,
    readonly extensionPath: string
  ) {
    super();

    this.treeDataProvider = this.push(
      new DatabaseTreeDataProvider(databaseManager, extensionPath)
    );
    this.push(
      window.createTreeView('codeQLDatabases', {
        treeDataProvider: this.treeDataProvider,
        canSelectMany: true,
      })
    );

    logger.log('Registering database panel commands.');
    this.push(
      commandRunnerWithProgress(
        'codeQL.setCurrentDatabase',
        this.handleSetCurrentDatabase,
        {
          title: 'Importing database from archive',
        }
      )
    );
    this.push(
      commandRunnerWithProgress(
        'codeQL.upgradeCurrentDatabase',
        this.handleUpgradeCurrentDatabase,
        {
          title: 'Upgrading current database',
          cancellable: true,
        }
      )
    );
    this.push(
      commandRunnerWithProgress(
        'codeQL.clearCache',
        this.handleClearCache,
        {
          title: 'Clearing Cache',
        })
    );

    this.push(
      commandRunnerWithProgress(
        'codeQLDatabases.chooseDatabaseFolder',
        this.handleChooseDatabaseFolder,
        {
          title: 'Adding database from folder',
        }
      )
    );
    this.push(
      commandRunnerWithProgress(
        'codeQLDatabases.chooseDatabaseArchive',
        this.handleChooseDatabaseArchive,
        {
          title: 'Adding database from archive',
        }
      )
    );
    this.push(
      commandRunnerWithProgress(
        'codeQLDatabases.chooseDatabaseInternet',
        this.handleChooseDatabaseInternet,
        {
          title: 'Adding database from URL',
        }
      )
    );
    this.push(
      commandRunnerWithProgress(
        'codeQLDatabases.chooseDatabaseLgtm',
        this.handleChooseDatabaseLgtm,
        {
          title: 'Adding database from LGTM',
        })
    );
    this.push(
      commandRunner(
        'codeQLDatabases.setCurrentDatabase',
        this.handleMakeCurrentDatabase
      )
    );
    this.push(
      commandRunner(
        'codeQLDatabases.sortByName',
        this.handleSortByName
      )
    );
    this.push(
      commandRunner(
        'codeQLDatabases.sortByDateAdded',
        this.handleSortByDateAdded
      )
    );
    this.push(
      commandRunner(
        'codeQLDatabases.removeDatabase',
        this.handleRemoveDatabase
      )
    );
    this.push(
      commandRunnerWithProgress(
        'codeQLDatabases.upgradeDatabase',
        this.handleUpgradeDatabase,
        {
          title: 'Upgrading database',
          cancellable: true,
        }
      )
    );
    this.push(
      commandRunner(
        'codeQLDatabases.renameDatabase',
        this.handleRenameDatabase
      )
    );
    this.push(
      commandRunner(
        'codeQLDatabases.openDatabaseFolder',
        this.handleOpenFolder
      )
    );
  }

  private handleMakeCurrentDatabase = async (
    databaseItem: DatabaseItem
  ): Promise<void> => {
    await this.databaseManager.setCurrentDatabaseItem(databaseItem);
  };

  handleChooseDatabaseFolder = async (
    progress: ProgressCallback,
    token: CancellationToken
  ): Promise<DatabaseItem | undefined> => {
    try {
      return await this.chooseAndSetDatabase(true, progress, token);
    } catch (e) {
      showAndLogErrorMessage(e.message);
      return undefined;
    }
  };

  handleChooseDatabaseArchive = async (
    progress: ProgressCallback,
    token: CancellationToken
  ): Promise<DatabaseItem | undefined> => {
    try {
      return await this.chooseAndSetDatabase(false, progress, token);
    } catch (e) {
      showAndLogErrorMessage(e.message);
      return undefined;
    }
  };

  handleChooseDatabaseInternet = async (
    progress: ProgressCallback,
    token: CancellationToken
  ): Promise<
    DatabaseItem | undefined
  > => {
    return await promptImportInternetDatabase(
      this.databaseManager,
      this.storagePath,
      progress,
      token
    );
  };

  handleChooseDatabaseLgtm = async (
    progress: ProgressCallback,
    token: CancellationToken
  ): Promise<DatabaseItem | undefined> => {
    return await promptImportLgtmDatabase(
      this.databaseManager,
      this.storagePath,
      progress,
      token
    );
  };

  async tryUpgradeCurrentDatabase(
    progress: ProgressCallback,
    token: CancellationToken
  ) {
    await this.handleUpgradeCurrentDatabase(progress, token);
  }

  private handleSortByName = async () => {
    if (this.treeDataProvider.sortOrder === SortOrder.NameAsc) {
      this.treeDataProvider.sortOrder = SortOrder.NameDesc;
    } else {
      this.treeDataProvider.sortOrder = SortOrder.NameAsc;
    }
  };

  private handleSortByDateAdded = async () => {
    if (this.treeDataProvider.sortOrder === SortOrder.DateAddedAsc) {
      this.treeDataProvider.sortOrder = SortOrder.DateAddedDesc;
    } else {
      this.treeDataProvider.sortOrder = SortOrder.DateAddedAsc;
    }
  };

  private handleUpgradeCurrentDatabase = async (
    progress: ProgressCallback,
    token: CancellationToken,
  ): Promise<void> => {
    await this.handleUpgradeDatabase(
      progress, token,
      this.databaseManager.currentDatabaseItem,
      []
    );
  };

  private handleUpgradeDatabase = async (
    progress: ProgressCallback,
    token: CancellationToken,
    databaseItem: DatabaseItem | undefined,
    multiSelect: DatabaseItem[] | undefined,
  ): Promise<void> => {
    if (multiSelect?.length) {
      await Promise.all(
        multiSelect.map((dbItem) => this.handleUpgradeDatabase(progress, token, dbItem, []))
      );
    }
    if (this.queryServer === undefined) {
      throw new Error(
        'Received request to upgrade database, but there is no running query server.'
      );
    }
    if (databaseItem === undefined) {
      throw new Error(
        'Received request to upgrade database, but no database was provided.'
      );
    }
    if (databaseItem.contents === undefined) {
      throw new Error(
        'Received request to upgrade database, but database contents could not be found.'
      );
    }
    if (databaseItem.contents.dbSchemeUri === undefined) {
      throw new Error(
        'Received request to upgrade database, but database has no schema.'
      );
    }

    // Search for upgrade scripts in any workspace folders available
    const searchPath: string[] = getOnDiskWorkspaceFolders();

    const upgradeInfo = await this.cliserver.resolveUpgrades(
      databaseItem.contents.dbSchemeUri.fsPath,
      searchPath
    );

    const { scripts, finalDbscheme } = upgradeInfo;

    if (finalDbscheme === undefined) {
      throw new Error('Could not determine target dbscheme to upgrade to.');
    }
    const targetDbSchemeUri = Uri.file(finalDbscheme);

    await upgradeDatabase(
      this.queryServer,
      databaseItem,
      targetDbSchemeUri,
      getUpgradesDirectories(scripts),
      progress,
      token
    );
  };

  private handleClearCache = async (
    progress: ProgressCallback,
    token: CancellationToken,
  ): Promise<void> => {
    if (
      this.queryServer !== undefined &&
      this.databaseManager.currentDatabaseItem !== undefined
    ) {
      await clearCacheInDatabase(
        this.queryServer,
        this.databaseManager.currentDatabaseItem,
        progress,
        token
      );
    }
  };

  private handleSetCurrentDatabase = async (
    progress: ProgressCallback,
    token: CancellationToken,
    uri: Uri,
  ): Promise<void> => {
    try {
      // Assume user has selected an archive if the file has a .zip extension
      if (uri.path.endsWith('.zip')) {
        await importArchiveDatabase(
          uri.toString(true),
          this.databaseManager,
          this.storagePath,
          progress,
          token
        );
      } else {
        await this.setCurrentDatabase(uri);
      }
    } catch (e) {
      // rethrow and let this be handled by default error handling.
      throw new Error(
        `Could not set database to ${path.basename(uri.fsPath)}. Reason: ${
        e.message
        }`
      );
    }
  };

  private handleRemoveDatabase = async (
    databaseItem: DatabaseItem,
    multiSelect: DatabaseItem[] | undefined
  ): Promise<void> => {
    if (multiSelect?.length) {
      multiSelect.forEach((dbItem) =>
        this.databaseManager.removeDatabaseItem(dbItem)
      );
    } else {
      this.databaseManager.removeDatabaseItem(databaseItem);
    }
  };

  private handleRenameDatabase = async (
    databaseItem: DatabaseItem,
    multiSelect: DatabaseItem[] | undefined
  ): Promise<void> => {
    this.assertSingleDatabase(multiSelect);

    const newName = await window.showInputBox({
      prompt: 'Choose new database name',
      value: databaseItem.name,
    });

    if (newName) {
      this.databaseManager.renameDatabaseItem(databaseItem, newName);
    }
  };

  private handleOpenFolder = async (
    databaseItem: DatabaseItem,
    multiSelect: DatabaseItem[] | undefined
  ): Promise<void> => {
    if (multiSelect?.length) {
      await Promise.all(
        multiSelect.map((dbItem) => env.openExternal(dbItem.databaseUri))
      );
    } else {
      await env.openExternal(databaseItem.databaseUri);
    }
  };

  /**
   * Return the current database directory. If we don't already have a
   * current database, ask the user for one, and return that, or
   * undefined if they cancel.
   */
  public async getDatabaseItem(
    progress: ProgressCallback,
    token: CancellationToken
  ): Promise<DatabaseItem | undefined> {
    if (this.databaseManager.currentDatabaseItem === undefined) {
      await this.chooseAndSetDatabase(false, progress, token);
    }

    return this.databaseManager.currentDatabaseItem;
  }

  private async setCurrentDatabase(
    uri: Uri
  ): Promise<DatabaseItem | undefined> {
    let databaseItem = this.databaseManager.findDatabaseItem(uri);
    if (databaseItem === undefined) {
      databaseItem = await this.databaseManager.openDatabase(uri);
    }
    await this.databaseManager.setCurrentDatabaseItem(databaseItem);

    return databaseItem;
  }

  /**
   * Ask the user for a database directory. Returns the chosen database, or `undefined` if the
   * operation was canceled.
   */
  private async chooseAndSetDatabase(
    byFolder: boolean,
    progress: ProgressCallback,
    token: CancellationToken,
  ): Promise<DatabaseItem | undefined> {
    const uri = await chooseDatabaseDir(byFolder);

    if (!uri) {
      return undefined;
    }

    if (byFolder) {
      const fixedUri = await this.fixDbUri(uri);
      // we are selecting a database folder
      return await this.setCurrentDatabase(fixedUri);
    } else {
      // we are selecting a database archive. Must unzip into a workspace-controlled area
      // before importing.
      return await importArchiveDatabase(
        uri.toString(true),
        this.databaseManager,
        this.storagePath,
        progress,
        token
      );
    }
  }

  /**
   * Perform some heuristics to ensure a proper database location is chosen.
   *
   * 1. If the selected URI to add is a file, choose the containing directory
   * 2. If the selected URI is a directory matching db-*, choose the containing directory
   * 3. choose the current directory
   *
   * @param uri a URI that is a datbase folder or inside it
   *
   * @return the actual database folder found by using the heuristics above.
   */
  private async fixDbUri(uri: Uri): Promise<Uri> {
    let dbPath = uri.fsPath;
    if ((await fs.stat(dbPath)).isFile()) {
      dbPath = path.dirname(dbPath);
    }

    if (isLikelyDbFolder(dbPath)) {
      dbPath = path.dirname(dbPath);
    }
    return Uri.file(dbPath);
  }

  private assertSingleDatabase(
    multiSelect: DatabaseItem[] = [],
    message = 'Please select a single database.'
  ) {
    if (multiSelect.length > 1) {
      throw new Error(message);
    }
  }
}

// TODO: Get the list of supported languages from a list that will be auto-updated.
const dbRegeEx = /^db-(javascript|go|cpp|java|python|csharp)$/;
function isLikelyDbFolder(dbPath: string) {
  return path.basename(dbPath).match(dbRegeEx);
}
