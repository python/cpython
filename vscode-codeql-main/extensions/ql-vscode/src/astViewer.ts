import {
  window,
  TreeDataProvider,
  EventEmitter,
  Event,
  ProviderResult,
  TreeItemCollapsibleState,
  TreeItem,
  TreeView,
  TextEditorSelectionChangeEvent,
  Location,
  Range
} from 'vscode';
import * as path from 'path';

import { DatabaseItem } from './databases';
import { UrlValue, BqrsId } from './bqrs-cli-types';
import { showLocation } from './interface-utils';
import { isStringLoc, isWholeFileLoc, isLineColumnLoc } from './bqrs-utils';
import { commandRunner } from './helpers';
import { DisposableObject } from './vscode-utils/disposable-object';

export interface AstItem {
  id: BqrsId;
  label?: string;
  location?: UrlValue;
  fileLocation?: Location;
  children: ChildAstItem[];
  order: number;
}

export interface ChildAstItem extends AstItem {
  parent: ChildAstItem | AstItem;
}

class AstViewerDataProvider  extends DisposableObject implements TreeDataProvider<AstItem> {

  public roots: AstItem[] = [];
  public db: DatabaseItem | undefined;

  private _onDidChangeTreeData =
    new EventEmitter<AstItem | undefined>();
  readonly onDidChangeTreeData: Event<AstItem | undefined> =
    this._onDidChangeTreeData.event;

  constructor() {
    super();
    this.push(
      commandRunner('codeQLAstViewer.gotoCode',
      async (item: AstItem) => {
        await showLocation(item.fileLocation);
      })
    );
  }

  refresh(): void {
    this._onDidChangeTreeData.fire();
  }
  getChildren(item?: AstItem): ProviderResult<AstItem[]> {
    const children = item ? item.children : this.roots;
    return children.sort((c1, c2) => (c1.order - c2.order));
  }

  getParent(item: ChildAstItem): ProviderResult<AstItem> {
    return item.parent;
  }

  getTreeItem(item: AstItem): TreeItem {
    const line = this.extractLineInfo(item?.location);

    const state = item.children.length
      ? TreeItemCollapsibleState.Collapsed
      : TreeItemCollapsibleState.None;
    const treeItem = new TreeItem(item.label || '', state);
    treeItem.description = line ? `Line ${line}` : '';
    treeItem.id = String(item.id);
    treeItem.tooltip = `${treeItem.description} ${treeItem.label}`;
    treeItem.command = {
      command: 'codeQLAstViewer.gotoCode',
      title: 'Go To Code',
      tooltip: `Go To ${item.location}`,
      arguments: [item]
    };
    return treeItem;
  }

  private extractLineInfo(loc?: UrlValue) {
    if (!loc) {
      return '';
    } else if (isStringLoc(loc)) {
      return loc;
    } else if (isWholeFileLoc(loc)) {
      return loc.uri;
    } else if (isLineColumnLoc(loc)) {
      return loc.startLine;
    } else {
      return '';
    }
  }
}

export class AstViewer extends DisposableObject {
  private treeView: TreeView<AstItem>;
  private treeDataProvider: AstViewerDataProvider;
  private currentFile: string | undefined;

  constructor() {
    super();

    this.treeDataProvider = new AstViewerDataProvider();
    this.treeView = window.createTreeView('codeQLAstViewer', {
      treeDataProvider: this.treeDataProvider,
      showCollapseAll: true
    });

    this.push(this.treeView);
    this.push(this.treeDataProvider);
    this.push(
      commandRunner('codeQLAstViewer.clear', async () => {
        this.clear();
      })
    );
    this.push(window.onDidChangeTextEditorSelection(this.updateTreeSelection, this));
  }

  updateRoots(roots: AstItem[], db: DatabaseItem, fileName: string) {
    this.treeDataProvider.roots = roots;
    this.treeDataProvider.db = db;
    this.treeDataProvider.refresh();
    this.treeView.message = `AST for ${path.basename(fileName)}`;
    this.treeView.reveal(roots[0], { focus: false });
    this.currentFile = fileName;
  }

  private updateTreeSelection(e: TextEditorSelectionChangeEvent) {
    function isInside(selectedRange: Range, astRange?: Range): boolean {
      return !!astRange?.contains(selectedRange);
    }

    // Recursively iterate all children until we find the node with the smallest
    // range that contains the selection.
    // Some nodes do not have a location, but their children might, so must
    // recurse though location-less AST nodes to see if children are correct.
    function findBest(selectedRange: Range, items?: AstItem[]): AstItem | undefined {
      if (!items || !items.length) {
        return;
      }
      for (const item of items) {
        let candidate: AstItem | undefined = undefined;
        if (isInside(selectedRange, item.fileLocation?.range)) {
          candidate = item;
        }
        // always iterate through children since the location of an AST node in code QL does not
        // always cover the complete text of the node.
        candidate = findBest(selectedRange, item.children) || candidate;
        if (candidate) {
          return candidate;
        }
      }
      return;
    }

    if (
      this.treeView.visible &&
      e.textEditor.document.uri.fsPath === this.currentFile &&
      e.selections.length === 1
    ) {
      const selection = e.selections[0];
      const range = selection.anchor.isBefore(selection.active)
        ? new Range(selection.anchor, selection.active)
        : new Range(selection.active, selection.anchor);

      const targetItem = findBest(range, this.treeDataProvider.roots);
      if (targetItem) {
        this.treeView.reveal(targetItem);
      }
    }
  }

  private clear() {
    this.treeDataProvider.roots = [];
    this.treeDataProvider.db = undefined;
    this.treeDataProvider.refresh();
    this.treeView.message = undefined;
    this.currentFile = undefined;
  }
}
