/**
 * Types for messages exchanged during jsonrpc communication with the
 * the CodeQL query server.
 *
 * This file exists in the queryserver and in the vscode extension, and
 * should be kept in sync between them.
 *
 * A note about the namespaces below, which look like they are
 * essentially enums, namely Severity, ResultColumnKind, and
 * QueryResultType. By design, for the sake of extensibility, clients
 * receiving messages of this protocol are supposed to accept any
 * number for any of these types. We commit to the given meaning of
 * the numbers listed in constants in the namespaces, and we commit to
 * the fact that any unknown QueryResultType value counts as an error.
 */

import * as rpc from 'vscode-jsonrpc';

/**
 * A position within a QL file.
 */
export interface Position {
  /**
   * The one-based index of the start line
   */
  line: number;
  /**
   * The one-based offset of the start column within
   * the start line in UTF-16 code-units
   */
  column: number;
  /**
   * The one-based index of the end line line
   */
  endLine: number;

  /**
   * The one-based offset of the end column within
   * the end line in UTF-16 code-units
   */
  endColumn: number;
  /**
   * The path of the file.
   * If the file name is "Compiler Generated" the
   * the position is not a real position but
   * arises from compiler generated code.
   */
  fileName: string;
}

/**
 * A query that should be checked for any errors or warnings
 */
export interface CheckQueryParams {
  /**
   * The options for compilation, if missing then the default options.
   */
  compilationOptions?: CompilationOptions;
  /**
   * The ql program to check.
   */
  queryToCheck: QlProgram;
  /**
   * The way of compiling a query
   */
  target: CompilationTarget;
}

/**
 * A query that should compiled into a qlo
 */
export interface CompileQueryParams {
  /**
   * The options for compilation, if missing then the default options.
   */
  compilationOptions?: CompilationOptions;
  /**
   * The options for compilation that do not affect the result.
   */
  extraOptions?: ExtraOptions;
  /**
  * The ql program to check.
  */
  queryToCheck: QlProgram;
  /**
   * The way of compiling a query
   */
  target: CompilationTarget;
  /**
   * The path to write the qlo at.
   */
  resultPath?: string;
}

/**
 * A dil (datalog intermediate language) query that should compiled into a qlo
 */
export interface CompileDilParams {
  /**
 * The options for compilation, if missing then the default options.
 */
  compilationOptions?: DilCompilationOptions;
  /**
   * The options for compilation that do not affect the result.
   */
  extraOptions?: ExtraOptions;
  /**
   * The dil query to compile
   */
  dilQuery?: DILQuery;
  /**
   * The path to write the qlo at.
   */
  resultPath?: string;
}


/**
 * The options for QL compilation.
 */
export interface CompilationOptions {
  /**
   * Whether to ensure that elements that do not have a location or URL
   * get a default location.
   */
  computeNoLocationUrls: boolean;
  /**
   * Whether to fail if any warnings occur in the ql code.
   */
  failOnWarnings: boolean;
  /**
   * Whether to compile as fast as possible, at the expense
   * of optimization.
   */
  fastCompilation: boolean;
  /**
   * Whether to include dil within qlos.
   */
  includeDilInQlo: boolean;
  /**
   * Whether to only do the initial program checks on the subset of the program that
   * is used.
   */
  localChecking: boolean;
  /**
   * Whether to disable urls in the results.
   */
  noComputeGetUrl: boolean;
  /**
   * Whether to disable toString values in the results.
   */
  noComputeToString: boolean;
}

/**
 * Compilation options that do not affect the result of
 * query compilation
 */
export interface ExtraOptions {
  /**
   * The uris of any additional compilation caches
   * TODO: Document cache uri format
   */
  extraCompilationCache?: string;
  /**
   * The compilation timeout in seconds. If it is
   * zero then there is no timeout.
   */
  timeoutSecs: number;
}


/**
 * The DIL compilation options
 */
export interface DilCompilationOptions {
  /**
   * Whether to compile as fast as possible, at the expense
   * of optimization.
   */
  fastCompilation: boolean;
  /**
   * Whether to include dil within qlos.
   */
  includeDilInQlo: boolean;
}

/**
 * A full ql program
 */
export interface QlProgram {
  /**
   * The path to the dbscheme
   */
  dbschemePath: string;
  /**
   *The ql library search path
   */
  libraryPath: string[];
  /**
   * The path to the query
   */
  queryPath: string;
  /**
   * If set then the contents of the source files.
   * Otherwise they will be searched for on disk.
   */
  sourceContents?: QlFileSet;
}

/**
 * A representation of files in query with all imports
 * pre-resolved.
 */
export interface QlFileSet {
  /**
   * The files imported by the given file
   */
  imports: { [key: string]: string[] };
  /**
   * An id of each file
   */
  nodeNumbering: { [key: string]: number };
  /**
   * The code for each file
   */
  qlCode: { [key: string]: string };
  /**
   * The resolution of an import in each directory.
   */
  resolvedDirImports: { [key: string]: { [key: string]: string } };
}

/**
 * An uncompiled dil query
 */
export interface DILQuery {
  /**
   * The path to the dbscheme
   */
  dbschemePath: string;
  /**
   * The path to the dil file
   */
  dilPath: string;
  /**
   * The dil source
   */
  dilSource: string;
}

/**
 * The way of compiling the query, as a normal query
 * or a subset of it. Note that precisely one of the two options should be set.
 */
export interface CompilationTarget {
  /**
   * Compile as a normal query
   */
  query?: {};
  /**
   * Compile as a quick evaluation
   */
  quickEval?: QuickEvalOptions;
}

/**
 * Options for quick evaluation
 */
export interface QuickEvalOptions {
  quickEvalPos?: Position;
}

/**
 * The result of checking a query.
 */
export interface CheckQueryResult {
  /**
   * Whether the query came from a compilation cache
   */
  fromCache: boolean;
  /**
   * The errors or warnings that occurred during compilation
   */
  messages: CompilationMessage[];
  /**
   * The types of the query predicates of the query
   */
  resultPatterns: ResultPattern[];
}


/**
 * A compilation message (either an error or a warning)
 */
export interface CompilationMessage {
  /**
   * The text of the message
   */
  message: string;
  /**
   * The source position associated with the message
   */
  position: Position;
  /**
   * The severity of the message
   */
  severity: Severity;
}

export type Severity = number;
/**
 * Severity of different messages. This namespace is intentionally not
 * an enum, see "for the sake of extensibility" comment above.
 */
// eslint-disable-next-line @typescript-eslint/no-namespace
export namespace Severity {
  /**
   * The message is a compilation error.
   */
  export const ERROR = 0;
  /**
   * The message is a compilation warning.
   */
  export const WARNING = 1;
}

/**
 * The type of a query predicate
 */
export interface ResultPattern {
  /**
   * The types of the columns of the query predicate
   */
  columns: ResultColumn[];
  /**
   * The name of the query predicate.
   * #select" is used as the name of a select clause.
   */
  name: string;
}

/**
 * The name and type of a single column
 */
export interface ResultColumn {
  /**
   * The kind of the column. See `ResultColumnKind`
   * for the current possible meanings
   */
  kind: ResultColumnKind;
  /**
   * The name of the column.
   * This may be compiler generated for complex select expressions.
   */
  name: string;
}

export type ResultColumnKind = number;
/**
 * The kind of a result column. This namespace is intentionally not an enum, see "for the sake of
 * extensibility" comment above.
 */
// eslint-disable-next-line @typescript-eslint/no-namespace
export namespace ResultColumnKind {
  /**
   * A column of type `float`
   */
  export const FLOAT = 0;
  /**
   * A column of type `int`
   */
  export const INTEGER = 1;
  /**
   * A column of type `string`
   */
  export const STRING = 2;
  /**
   * A column of type `boolean`
   */
  export const BOOLEAN = 3;
  /**
 * A column of type `date`
 */
  export const DATE = 4;
  /**
   * A column of a non-primitive type
   */
  export const ENTITY = 5;
}

/**
 * Parameters for compiling an upgrade.
 */
export interface CompileUpgradeParams {
  /**
   * The parameters for how to compile the upgrades
   */
  upgrade: UpgradeParams;
  /**
   * A directory to store parts of the compiled upgrade
   */
  upgradeTempDir: string;
}

/**
 * Parameters describing an upgrade
 */
export interface UpgradeParams {
  /**
   * The location of non built-in upgrades
   */
  additionalUpgrades: string[];
  /**
   * The path to the current dbscheme to start the upgrade
   */
  fromDbscheme: string;
  /**
   * The path to the target dbscheme to try an upgrade to
   */
  toDbscheme: string;
}

/**
 * The result of checking an upgrade
 */
export interface CheckUpgradeResult {
  /**
   * A description of the steps to take to upgrade this dataset.
   * Note that the list may be partial.
   */
  checkedUpgrades?: UpgradesDescription;
  /**
   * Any errors that occurred when checking the scripts.
   */
  upgradeError?: string;
}


/**
 * The result of compiling an upgrade
 */
export interface CompileUpgradeResult {
  /**
   * The compiled upgrade.
   */
  compiledUpgrades?: CompiledUpgrades;
  /**
   * Any errors that occurred when checking the scripts.
   */
  error?: string;
}
/**
 * A description of a upgrade process
 */
export interface UpgradesDescription {
  /**
   * The initial sha of the dbscheme to upgrade from
   */
  initialSha: string;
  /**
   * A list of description of the steps in the upgrade process.
   * Note that this may only upgrade partially
   */
  scripts: UpgradeDescription[];
  /**
   * The sha of the target dataset.
   */
  targetSha: string;
}

/**
 * The description of a single step
 */
export interface UpgradeDescription {
  /**
   * The compatibility of the upgrade
   */
  compatibility: string;
  /**
   * A description of the upgrade
   */
  description: string;
  /**
   * The dbscheme sha after this upgrade has run.
   */
  newSha: string;
}

/**
 * A compiled upgrade.
 */
export interface CompiledUpgrades {
  /**
  * The initial sha of the dbscheme to upgrade from
  */
  initialSha: string;
  /**
   * The path to the new dataset statistics
   */
  newStatsPath: string;
  /**
   * The steps in the upgrade path
   */
  scripts: CompiledUpgradeScript[];
  /**
   * The sha of the target dataset.
   */
  targetSha: string;
}

/**
 * A compiled step to upgrade the dataset.
 */
export interface CompiledUpgradeScript {
  /**
   * A description of the spec
   */
  description: UpgradeDescription;
  /**
   * The path to the dbscheme that this upgrade step
   * upgrades to.
   */
  newDbschemePath: string;
  /**
   * The actions required to run this step.
   */
  specs: UpgradeAction[];
}

/**
 * An action used to upgrade a query.
 * Only one of the options should be set
 */
export interface UpgradeAction {
  deleted?: DeleteSpec;
  runQuery?: QloSpec;
}

/**
 * Delete a relation
 */
export interface DeleteSpec {
  /**
   * The name of the relation to delete
   */
  relationToDelete: string;
}

/**
 * Run a qlo to provide a relation
 */
export interface QloSpec {
  /**
   * The name of the relation to create/replace
   */
  newRelation: string;
  /**
   * The Uri of the qlo to run
   */
  qloUri: string;
}

/**
 * Parameters to clear the cache
 */
export interface ClearCacheParams {
  /**
   * The dataset for which we want to clear the cache
   */
  db: Dataset;
  /**
   * Whether the cache should actually be cleared.
   */
  dryRun: boolean;
}
/**
 * Parameters for trimming the cache of a dataset
 */
export interface TrimCacheParams {
  /**
   * The dataset that we want to trim the cache of.
   */
  db: Dataset;
}


/**
 * A ql dataset
 */
export interface Dataset {
  /**
   * The path to the dataset
   */
  dbDir: string;
  /**
   * The name of the working set (normally "default")
   */
  workingSet: string;
}

/**
 * The result of trimming or clearing the cache.
 */
export interface ClearCacheResult {
  /**
   * A user friendly message saying what was or would be
   * deleted.
   */
  deletionMessage: string;
}

/**
 * Parameters for running a set of queries
 */
export interface EvaluateQueriesParams {
  /**
   * The dataset to run on
   */
  db: Dataset;
  /**
   * An identifier used in callbacks to identify this run.
   */
  evaluateId: number;
  /**
   * The queries to run
   */
  queries: QueryToRun[];
  /**
   * Whether the evaluator should stop on a non fatal-error
   */
  stopOnError: boolean;
  /**
   * Whether the evaluator should assume this is the final
   * run on this dataset before it's cache would be deleted.
   */
  useSequenceHint: boolean;
}

export type TemplateDefinitions = { [key: string]: TemplateSource }

/**
 * A single query that should be run
 */
export interface QueryToRun {
  /**
   * The id of this query within the run
   */
  id: number;
  /**
   * A uri pointing to the qlo to run.
   */
  qlo: string;
  /**
   * The path where we should save this queries results
   */
  resultsPath: string;
  /**
   * The per stage timeout (0 for no timeout)
   */
  timeoutSecs: number;
  /**
   * Values to set for each template
   */
  templateValues?: TemplateDefinitions;
  /**
   * Whether templates without values in the templateValues
   * map should be set to the empty set or give an error.
   */
  allowUnknownTemplates: boolean;
}

/**
 * The source of templates. Only one
 */
export interface TemplateSource {
  /**
   * Do basic interpretation of query results and
   * use the interpreted results in the query.
   * This should only be used to support legacy filter
   * queries.
   */
  interpretedInput?: ProblemResults;
  /**
   * Use the explicitly listed values
   */
  values?: RelationValues;
}

/**
 * A relation as a list of tuples
 */
export interface RelationValues {
  tuples: Value[][];
}

/**
 * A single primitive value for templates.
 * Only one case should be set.
 */
export interface Value {
  booleanValue?: boolean;
  dateValue?: string;
  doubleValue?: number;
  intValue?: number;
  stringValue?: string;
}

/**
 * A relation made by interpreting the results of a problem or metric query
 * to be used as the input to a filter query.
 */
export interface ProblemResults {
  /**
   * The path to the original query.
   */
  queryPath: string;
  /**
   * The way of obtaining the queries
   */
  results: ResultSet;
  /**
   * Whether the results are for a defect filter or a metric filter.
   */
  type: ResultType;
}

/**
 * The type of results that are going to be sent into the filter query.
 */
export enum ResultType {
  METRIC = 0,
  DEFECT = 1,
}

/**
 * The way of obtaining the results
 */
export interface ResultSet {
  /**
   * Via an earlier query in the evaluation run
   */
  precedingQuery?: number;
  /**
   * Directly from an existing results set.
   */
  resultsFile?: string;
}

/**
 * The type returned when the evaluation is complete
 */
export type EvaluationComplete = {};

/**
 * The result of a single query
 */
export interface EvaluationResult {
  /**
   * The id of the run that this query was in
   */
  runId: number;
  /**
   * The id of the query within the run
   */
  queryId: number;
  /**
   * The type of the result. See QueryResultType for
   * possible meanings. Any other result should be interpreted as an error.
   */
  resultType: QueryResultType;
  /**
   * The wall clock time it took to evaluate the query.
   * The time is from when we initially tried to evaluate the query
   * to when we get the results. Hence with parallel evaluation the times may
   * look odd.
   */
  evaluationTime: number;
  /**
   * An error message if an error happened
   */
  message?: string;

  /**
   * Full path to file with all log messages emitted while this query was active, if one exists
   */
  logFileLocation?: string;
}

export type QueryResultType = number;
/**
 * The result of running a query. This namespace is intentionally not
 * an enum, see "for the sake of extensibility" comment above.
 */
// eslint-disable-next-line @typescript-eslint/no-namespace
export namespace QueryResultType {
  /**
   * The query ran successfully
   */
  export const SUCCESS = 0;
  /**
   * The query failed due to an reason
   * that isn't listed
   */
  export const OTHER_ERROR = 1;
  /**
    * The query failed due to running out of
    * memory
    */
  export const OOM = 2;
  /**
   * The query failed due to exceeding the timeout
   */
  export const TIMEOUT = 3;
  /**
   * The query failed because it was cancelled.
   */
  export const CANCELLATION = 4;
}

/**
 * Parameters for running an upgrade
 */
export interface RunUpgradeParams {
  /**
   * The dataset to upgrade
   */
  db: Dataset;
  /**
   * The per stage timeout in seconds. Use 0
   * for no timeout.
   */
  timeoutSecs: number;
  /**
   * The upgrade to run
   */
  toRun: CompiledUpgrades;
}


/**
 * The result of running an upgrade
 */
export interface RunUpgradeResult {
  /**
   * The type of the result. See QueryResultType for
   * possible meanings. Any other result should be interpreted as an error.
   */
  resultType: QueryResultType;
  /**
   * The error message if an error occurred
   */
  error?: string;
  /**
   * The new dbscheme sha.
   */
  finalSha: string;
}


/**
 * Type for any action that could have progress messages.
 */
export interface WithProgressId<T> {
  /**
   * The main body
   */
  body: T;
  /**
   * The id used to report progress updates
   */
  progressId: number;
}

export interface ProgressMessage {
  /**
   * The id of the operation that is running
   */
  id: number;
  /**
   * The current step
   */
  step: number;
  /**
   * The maximum step. This *should* be constant for a single job.
   */
  maxStep: number;
  /**
   * The current progress message
   */
  message: string;
}

/**
 * Check a Ql query for errors without compiling it
 */
export const checkQuery = new rpc.RequestType<WithProgressId<CheckQueryParams>, CheckQueryResult, void, void>('compilation/checkQuery');
/**
 * Compile a Ql query into a qlo
 */
export const compileQuery = new rpc.RequestType<WithProgressId<CompileQueryParams>, CheckQueryResult, void, void>('compilation/compileQuery');
/**
 * Compile a dil query into a qlo
 */
export const compileDilQuery = new rpc.RequestType<WithProgressId<CompileDilParams>, CheckQueryResult, void, void>('compilation/compileDilQuery');


/**
 * Check if there is a valid upgrade path between two dbschemes.
 */
export const checkUpgrade = new rpc.RequestType<WithProgressId<UpgradeParams>, CheckUpgradeResult, void, void>('compilation/checkUpgrade');
/**
 * Compile an upgrade script to upgrade a dataset.
 */
export const compileUpgrade = new rpc.RequestType<WithProgressId<CompileUpgradeParams>, CompileUpgradeResult, void, void>('compilation/compileUpgrade');


/**
 * Clear the cache of a dataset
 */
export const clearCache = new rpc.RequestType<WithProgressId<ClearCacheParams>, ClearCacheResult, void, void>('evaluation/clearCache');
/**
 * Trim the cache of a dataset
 */
export const trimCache = new rpc.RequestType<WithProgressId<TrimCacheParams>, ClearCacheResult, void, void>('evaluation/trimCache');

/**
 * Run some queries on a dataset
 */
export const runQueries = new rpc.RequestType<WithProgressId<EvaluateQueriesParams>, EvaluationComplete, void, void>('evaluation/runQueries');

/**
 * Run upgrades on a dataset
 */
export const runUpgrade = new rpc.RequestType<WithProgressId<RunUpgradeParams>, RunUpgradeResult, void, void>('evaluation/runUpgrade');

/**
 * Request returned to the client to notify completion of a query.
 * The full runQueries job is completed when all queries are acknowledged.
 */
export const completeQuery = new rpc.RequestType<EvaluationResult, Record<string, any>, void, void>('evaluation/queryCompleted');

/**
 * A notification that the progress has been changed.
 */
export const progress = new rpc.NotificationType<ProgressMessage, void>('ql/progressUpdated');
