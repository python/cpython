import * as vscode from 'vscode';

import { decodeSourceArchiveUri, encodeArchiveBasePath, zipArchiveScheme } from '../archive-filesystem-provider';
import { CodeQLCliServer } from '../cli';
import { DatabaseManager } from '../databases';
import { CachedOperation, ProgressCallback, withProgress } from '../helpers';
import * as messages from '../messages';
import { QueryServerClient } from '../queryserver-client';
import { compileAndRunQueryAgainstDatabase, QueryWithResults } from '../run-queries';
import AstBuilder from './astBuilder';
import {
  KeyType,
} from './keyType';
import { FullLocationLink, getLocationsForUriString, TEMPLATE_NAME } from './locationFinder';
import { qlpackOfDatabase, resolveQueries } from './queryResolver';

/**
 * Run templated CodeQL queries to find definitions and references in
 * source-language files. We may eventually want to find a way to
 * generalize this to other custom queries, e.g. showing dataflow to
 * or from a selected identifier.
 */

export class TemplateQueryDefinitionProvider implements vscode.DefinitionProvider {
  private cache: CachedOperation<vscode.LocationLink[]>;

  constructor(
    private cli: CodeQLCliServer,
    private qs: QueryServerClient,
    private dbm: DatabaseManager,
  ) {
    this.cache = new CachedOperation<vscode.LocationLink[]>(this.getDefinitions.bind(this));
  }

  async provideDefinition(document: vscode.TextDocument, position: vscode.Position, _token: vscode.CancellationToken): Promise<vscode.LocationLink[]> {
    const fileLinks = await this.cache.get(document.uri.toString());
    const locLinks: vscode.LocationLink[] = [];
    for (const link of fileLinks) {
      if (link.originSelectionRange!.contains(position)) {
        locLinks.push(link);
      }
    }
    return locLinks;
  }

  private async getDefinitions(uriString: string): Promise<vscode.LocationLink[]> {
    return withProgress({
      location: vscode.ProgressLocation.Notification,
      cancellable: true,
      title: 'Finding definitions'
    }, async (progress, token) => {
      return getLocationsForUriString(
        this.cli,
        this.qs,
        this.dbm,
        uriString,
        KeyType.DefinitionQuery,
        progress,
        token,
        (src, _dest) => src === uriString
      );
    });
  }
}

export class TemplateQueryReferenceProvider implements vscode.ReferenceProvider {
  private cache: CachedOperation<FullLocationLink[]>;

  constructor(
    private cli: CodeQLCliServer,
    private qs: QueryServerClient,
    private dbm: DatabaseManager,
  ) {
    this.cache = new CachedOperation<FullLocationLink[]>(this.getReferences.bind(this));
  }

  async provideReferences(
    document: vscode.TextDocument,
    position: vscode.Position,
    _context: vscode.ReferenceContext,
    _token: vscode.CancellationToken
  ): Promise<vscode.Location[]> {
    const fileLinks = await this.cache.get(document.uri.toString());
    const locLinks: vscode.Location[] = [];
    for (const link of fileLinks) {
      if (link.targetRange!.contains(position)) {
        locLinks.push({ range: link.originSelectionRange!, uri: link.originUri });
      }
    }
    return locLinks;
  }

  private async getReferences(uriString: string): Promise<FullLocationLink[]> {
    return withProgress({
      location: vscode.ProgressLocation.Notification,
      cancellable: true,
      title: 'Finding references'
    }, async (progress, token) => {
      return getLocationsForUriString(
        this.cli,
        this.qs,
        this.dbm,
        uriString,
        KeyType.DefinitionQuery,
        progress,
        token,
        (src, _dest) => src === uriString
      );
    });
  }
}

export class TemplatePrintAstProvider {
  private cache: CachedOperation<QueryWithResults | undefined>;

  constructor(
    private cli: CodeQLCliServer,
    private qs: QueryServerClient,
    private dbm: DatabaseManager,

    // Note: progress and token are only used if a cached value is not available
    private progress: ProgressCallback,
    private token: vscode.CancellationToken
  ) {
    this.cache = new CachedOperation<QueryWithResults | undefined>(this.getAst.bind(this));
  }

  async provideAst(document?: vscode.TextDocument): Promise<AstBuilder | undefined> {
    if (!document) {
      return;
    }
    const queryResults = await this.cache.get(document.uri.toString());
    if (!queryResults) {
      return;
    }

    return new AstBuilder(
      queryResults, this.cli,
      this.dbm.findDatabaseItem(vscode.Uri.parse(queryResults.database.databaseUri!))!,
      document.fileName
    );
  }

  private async getAst(uriString: string): Promise<QueryWithResults> {
    const uri = vscode.Uri.parse(uriString, true);
    if (uri.scheme !== zipArchiveScheme) {
      throw new Error('AST Viewing is only available for databases with zipped source archives.');
    }

    const zippedArchive = decodeSourceArchiveUri(uri);
    const sourceArchiveUri = encodeArchiveBasePath(zippedArchive.sourceArchiveZipPath);
    const db = this.dbm.findDatabaseItemBySourceArchive(sourceArchiveUri);

    if (!db) {
      throw new Error('Can\'t infer database from the provided source.');
    }

    const qlpack = await qlpackOfDatabase(this.cli, db);
    if (!qlpack) {
      throw new Error('Can\'t infer qlpack from database source archive');
    }
    const queries = await resolveQueries(this.cli, qlpack, KeyType.PrintAstQuery);
    if (queries.length > 1) {
      throw new Error('Found multiple Print AST queries. Can\'t continue');
    }
    if (queries.length === 0) {
      throw new Error('Did not find any Print AST queries. Can\'t continue');
    }

    const query = queries[0];
    const templates: messages.TemplateDefinitions = {
      [TEMPLATE_NAME]: {
        values: {
          tuples: [[{
            stringValue: zippedArchive.pathWithinSourceArchive
          }]]
        }
      }
    };

    return await compileAndRunQueryAgainstDatabase(
      this.cli,
      this.qs,
      db,
      false,
      vscode.Uri.file(query),
      this.progress,
      this.token,
      templates
    );
  }
}
