import { DisposableObject } from '../vscode-utils/disposable-object';
import {
  WebviewPanel,
  ExtensionContext,
  window as Window,
  ViewColumn,
  Uri,
} from 'vscode';
import * as path from 'path';

import { tmpDir } from '../run-queries';
import { CompletedQuery } from '../query-results';
import {
  FromCompareViewMessage,
  ToCompareViewMessage,
  QueryCompareResult,
} from '../interface-types';
import { Logger } from '../logging';
import { CodeQLCliServer } from '../cli';
import { DatabaseManager } from '../databases';
import { getHtmlForWebview, jumpToLocation } from '../interface-utils';
import { transformBqrsResultSet, RawResultSet, BQRSInfo } from '../bqrs-cli-types';
import resultsDiff from './resultsDiff';

interface ComparePair {
  from: CompletedQuery;
  to: CompletedQuery;
}

export class CompareInterfaceManager extends DisposableObject {
  private comparePair: ComparePair | undefined;
  private panel: WebviewPanel | undefined;
  private panelLoaded = false;
  private panelLoadedCallBacks: (() => void)[] = [];

  constructor(
    private ctx: ExtensionContext,
    private databaseManager: DatabaseManager,
    private cliServer: CodeQLCliServer,
    private logger: Logger,
    private showQueryResultsCallback: (
      item: CompletedQuery
    ) => Promise<void>
  ) {
    super();
  }

  async showResults(
    from: CompletedQuery,
    to: CompletedQuery,
    selectedResultSetName?: string
  ) {
    this.comparePair = { from, to };
    this.getPanel().reveal(undefined, true);

    await this.waitForPanelLoaded();
    const [
      commonResultSetNames,
      currentResultSetName,
      fromResultSet,
      toResultSet,
    ] = await this.findCommonResultSetNames(
      from,
      to,
      selectedResultSetName
    );
    if (currentResultSetName) {
      let rows: QueryCompareResult | undefined;
      let message: string | undefined;
      try {
        rows = this.compareResults(fromResultSet, toResultSet);
      } catch (e) {
        message = e.message;
      }

      await this.postMessage({
        t: 'setComparisons',
        stats: {
          fromQuery: {
            // since we split the description into several rows
            // only run interpolation if the label is user-defined
            // otherwise we will wind up with duplicated rows
            name: from.options.label
              ? from.interpolate(from.getLabel())
              : from.queryName,
            status: from.statusString,
            time: from.time,
          },
          toQuery: {
            name: to.options.label
              ? to.interpolate(to.getLabel())
              : to.queryName,
            status: to.statusString,
            time: to.time,
          },
        },
        columns: fromResultSet.schema.columns,
        commonResultSetNames,
        currentResultSetName: currentResultSetName,
        rows,
        message,
        datebaseUri: to.database.databaseUri,
      });
    }
  }

  getPanel(): WebviewPanel {
    if (this.panel == undefined) {
      const { ctx } = this;
      const panel = (this.panel = Window.createWebviewPanel(
        'compareView',
        'Compare CodeQL Query Results',
        { viewColumn: ViewColumn.Active, preserveFocus: true },
        {
          enableScripts: true,
          enableFindWidget: true,
          retainContextWhenHidden: true,
          localResourceRoots: [
            Uri.file(tmpDir.name),
            Uri.file(path.join(this.ctx.extensionPath, 'out')),
          ],
        }
      ));
      this.panel.onDidDispose(
        () => {
          this.panel = undefined;
          this.comparePair = undefined;
        },
        null,
        ctx.subscriptions
      );

      const scriptPathOnDisk = Uri.file(
        ctx.asAbsolutePath('out/compareView.js')
      );

      const stylesheetPathOnDisk = Uri.file(
        ctx.asAbsolutePath('out/resultsView.css')
      );

      panel.webview.html = getHtmlForWebview(
        panel.webview,
        scriptPathOnDisk,
        stylesheetPathOnDisk
      );
      panel.webview.onDidReceiveMessage(
        async (e) => this.handleMsgFromView(e),
        undefined,
        ctx.subscriptions
      );
    }
    return this.panel;
  }

  private waitForPanelLoaded(): Promise<void> {
    return new Promise((resolve) => {
      if (this.panelLoaded) {
        resolve();
      } else {
        this.panelLoadedCallBacks.push(resolve);
      }
    });
  }

  private async handleMsgFromView(
    msg: FromCompareViewMessage
  ): Promise<void> {
    switch (msg.t) {
      case 'compareViewLoaded':
        this.panelLoaded = true;
        this.panelLoadedCallBacks.forEach((cb) => cb());
        this.panelLoadedCallBacks = [];
        break;

      case 'changeCompare':
        this.changeTable(msg.newResultSetName);
        break;

      case 'viewSourceFile':
        await jumpToLocation(msg, this.databaseManager, this.logger);
        break;

      case 'openQuery':
        await this.openQuery(msg.kind);
        break;
    }
  }

  private postMessage(msg: ToCompareViewMessage): Thenable<boolean> {
    return this.getPanel().webview.postMessage(msg);
  }

  private async findCommonResultSetNames(
    from: CompletedQuery,
    to: CompletedQuery,
    selectedResultSetName: string | undefined
  ): Promise<[string[], string, RawResultSet, RawResultSet]> {
    const fromSchemas = await this.cliServer.bqrsInfo(
      from.query.resultsPaths.resultsPath
    );
    const toSchemas = await this.cliServer.bqrsInfo(
      to.query.resultsPaths.resultsPath
    );
    const fromSchemaNames = fromSchemas['result-sets'].map(
      (schema) => schema.name
    );
    const toSchemaNames = toSchemas['result-sets'].map(
      (schema) => schema.name
    );
    const commonResultSetNames = fromSchemaNames.filter((name) =>
      toSchemaNames.includes(name)
    );
    const currentResultSetName =
      selectedResultSetName || commonResultSetNames[0];
    const fromResultSet = await this.getResultSet(
      fromSchemas,
      currentResultSetName,
      from.query.resultsPaths.resultsPath
    );
    const toResultSet = await this.getResultSet(
      toSchemas,
      currentResultSetName,
      to.query.resultsPaths.resultsPath
    );
    return [
      commonResultSetNames,
      currentResultSetName,
      fromResultSet,
      toResultSet,
    ];
  }

  private async changeTable(newResultSetName: string) {
    if (!this.comparePair?.from || !this.comparePair.to) {
      return;
    }
    await this.showResults(
      this.comparePair.from,
      this.comparePair.to,
      newResultSetName
    );
  }

  private async getResultSet(
    bqrsInfo: BQRSInfo,
    resultSetName: string,
    resultsPath: string
  ): Promise<RawResultSet> {
    const schema = bqrsInfo['result-sets'].find(
      (schema) => schema.name === resultSetName
    );
    if (!schema) {
      throw new Error(`Schema ${resultSetName} not found.`);
    }
    const chunk = await this.cliServer.bqrsDecode(
      resultsPath,
      resultSetName
    );
    return transformBqrsResultSet(schema, chunk);
  }

  private compareResults(
    fromResults: RawResultSet,
    toResults: RawResultSet
  ): QueryCompareResult {
    // Only compare columns that have the same name
    return resultsDiff(fromResults, toResults);
  }

  private openQuery(kind: 'from' | 'to') {
    const toOpen =
      kind === 'from' ? this.comparePair?.from : this.comparePair?.to;
    if (toOpen) {
      this.showQueryResultsCallback(toOpen);
    }
  }
}
