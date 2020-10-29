import { env, TreeItem } from 'vscode';

import { QueryWithResults, tmpDir, QueryInfo } from './run-queries';
import * as messages from './messages';
import * as cli from './cli';
import * as sarif from 'sarif';
import * as fs from 'fs-extra';
import * as path from 'path';
import { RawResultsSortState, SortedResultSetInfo, DatabaseInfo, QueryMetadata, InterpretedResultsSortState, ResultsPaths } from './interface-types';
import { QueryHistoryConfig } from './config';
import { QueryHistoryItemOptions } from './query-history';

export class CompletedQuery implements QueryWithResults {
  readonly time: string;
  readonly query: QueryInfo;
  readonly result: messages.EvaluationResult;
  readonly database: DatabaseInfo;
  readonly logFileLocation?: string;
  options: QueryHistoryItemOptions;
  treeItem?: TreeItem;
  dispose: () => void;

  /**
   * Map from result set name to SortedResultSetInfo.
   */
  sortedResultsInfo: Map<string, SortedResultSetInfo>;

  /**
   * How we're currently sorting alerts. This is not mere interface
   * state due to truncation; on re-sort, we want to read in the file
   * again, sort it, and only ship off a reasonable number of results
   * to the webview. Undefined means to use whatever order is in the
   * sarif file.
   */
  interpretedResultsSortState: InterpretedResultsSortState | undefined;

  constructor(
    evaluation: QueryWithResults,
    public config: QueryHistoryConfig,
  ) {
    this.query = evaluation.query;
    this.result = evaluation.result;
    this.database = evaluation.database;
    this.logFileLocation = evaluation.logFileLocation;
    this.options = evaluation.options;
    this.dispose = evaluation.dispose;

    this.time = new Date().toLocaleString(env.language);
    this.sortedResultsInfo = new Map();
  }

  get databaseName(): string {
    return this.database.name;
  }
  get queryName(): string {
    return getQueryName(this.query);
  }

  get statusString(): string {
    switch (this.result.resultType) {
      case messages.QueryResultType.CANCELLATION:
        return `cancelled after ${this.result.evaluationTime / 1000} seconds`;
      case messages.QueryResultType.OOM:
        return 'out of memory';
      case messages.QueryResultType.SUCCESS:
        return `finished in ${this.result.evaluationTime / 1000} seconds`;
      case messages.QueryResultType.TIMEOUT:
        return `timed out after ${this.result.evaluationTime / 1000} seconds`;
      case messages.QueryResultType.OTHER_ERROR:
      default:
        return this.result.message ? `failed: ${this.result.message}` : 'failed';
    }
  }

  getResultsPath(selectedTable: string, useSorted = true): string {
    if (!useSorted) {
      return this.query.resultsPaths.resultsPath;
    }
    return this.sortedResultsInfo.get(selectedTable)?.resultsPath
      || this.query.resultsPaths.resultsPath;
  }

  interpolate(template: string): string {
    const { databaseName, queryName, time, statusString } = this;
    const replacements: { [k: string]: string } = {
      t: time,
      q: queryName,
      d: databaseName,
      s: statusString,
      '%': '%',
    };
    return template.replace(/%(.)/g, (match, key) => {
      const replacement = replacements[key];
      return replacement !== undefined ? replacement : match;
    });
  }

  getLabel(): string {
      return this.options?.label
        || this.config.format;
  }

  get didRunSuccessfully(): boolean {
    return this.result.resultType === messages.QueryResultType.SUCCESS;
  }

  toString(): string {
    return this.interpolate(this.getLabel());
  }

  async updateSortState(
    server: cli.CodeQLCliServer,
    resultSetName: string,
    sortState?: RawResultsSortState
  ): Promise<void> {
    if (sortState === undefined) {
      this.sortedResultsInfo.delete(resultSetName);
      return;
    }

    const sortedResultSetInfo: SortedResultSetInfo = {
      resultsPath: path.join(tmpDir.name, `sortedResults${this.query.queryID}-${resultSetName}.bqrs`),
      sortState
    };

    await server.sortBqrs(
      this.query.resultsPaths.resultsPath,
      sortedResultSetInfo.resultsPath,
      resultSetName,
      [sortState.columnIndex],
      [sortState.sortDirection]
    );
    this.sortedResultsInfo.set(resultSetName, sortedResultSetInfo);
  }

  async updateInterpretedSortState(sortState?: InterpretedResultsSortState): Promise<void> {
    this.interpretedResultsSortState = sortState;
  }
}


/**
 * Gets a human-readable name for an evaluated query.
 * Uses metadata if it exists, and defaults to the query file name.
 */
export function getQueryName(query: QueryInfo) {
  // Queries run through quick evaluation are not usually the entire query file.
  // Label them differently and include the line numbers.
  if (query.quickEvalPosition !== undefined) {
    const { line, endLine, fileName } = query.quickEvalPosition;
    const lineInfo = line === endLine ? `${line}` : `${line}-${endLine}`;
    return `Quick evaluation of ${path.basename(fileName)}:${lineInfo}`;
  } else if (query.metadata?.name) {
    return query.metadata.name;
  } else {
    return path.basename(query.program.queryPath);
  }
}


/**
 * Call cli command to interpret results.
 */
export async function interpretResults(
  server: cli.CodeQLCliServer,
  metadata: QueryMetadata | undefined,
  resultsPaths: ResultsPaths,
  sourceInfo?: cli.SourceInfo
): Promise<sarif.Log> {
  const { resultsPath, interpretedResultsPath } = resultsPaths;
  if (await fs.pathExists(interpretedResultsPath)) {
    return JSON.parse(await fs.readFile(interpretedResultsPath, 'utf8'));
  }
  if (metadata === undefined) {
    throw new Error('Can\'t interpret results without query metadata');
  }
  let { kind, id } = metadata;
  if (kind === undefined) {
    throw new Error('Can\'t interpret results without query metadata including kind');
  }
  if (id === undefined) {
    // Interpretation per se doesn't really require an id, but the
    // SARIF format does, so in the absence of one, we use a dummy id.
    id = 'dummy-id';
  }
  return await server.interpretBqrs({ kind, id }, resultsPath, interpretedResultsPath, sourceInfo);
}
