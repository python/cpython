import { EventEmitter, Event, Uri, WorkspaceFolder, RelativePattern } from 'vscode';
import { MultiFileSystemWatcher } from './vscode-utils/multi-file-system-watcher';
import { CodeQLCliServer, QlpacksInfo } from './cli';
import { Discovery } from './discovery';

export interface QLPack {
  name: string;
  uri: Uri;
}

/**
 * Service to discover all available QL packs in a workspace folder.
 */
export class QLPackDiscovery extends Discovery<QlpacksInfo> {
  private readonly _onDidChangeQLPacks = this.push(new EventEmitter<void>());
  private readonly watcher = this.push(new MultiFileSystemWatcher());
  private _qlPacks: readonly QLPack[] = [];

  constructor(
    private readonly workspaceFolder: WorkspaceFolder,
    private readonly cliServer: CodeQLCliServer
  ) {
    super('QL Pack Discovery');

    // Watch for any changes to `qlpack.yml` files in this workspace folder.
    // TODO: The CLI server should tell us what paths to watch for.
    this.watcher.addWatch(new RelativePattern(this.workspaceFolder, '**/qlpack.yml'));
    this.watcher.addWatch(new RelativePattern(this.workspaceFolder, '**/.codeqlmanifest.json'));
    this.push(this.watcher.onDidChange(this.handleQLPackFileChanged, this));
  }

  public get onDidChangeQLPacks(): Event<void> { return this._onDidChangeQLPacks.event; }

  public get qlPacks(): readonly QLPack[] { return this._qlPacks; }

  private handleQLPackFileChanged(_uri: Uri): void {
    this.refresh();
  }

  protected discover(): Promise<QlpacksInfo> {
    // Only look for QL packs in this workspace folder.
    return this.cliServer.resolveQlpacks([this.workspaceFolder.uri.fsPath], []);
  }

  protected update(results: QlpacksInfo): void {
    const qlPacks: QLPack[] = [];
    for (const id in results) {
      qlPacks.push(...results[id].map(fsPath => {
        return {
          name: id,
          uri: Uri.file(fsPath)
        };
      }));
    }
    this._qlPacks = qlPacks;
    this._onDidChangeQLPacks.fire();
  }
}
