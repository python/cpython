import * as sarif from 'sarif';

/**
 * Identifies one of the results in a result set by its index in the result list.
 */
export interface Result {
  resultIndex: number;
}

/**
 * Identifies one of the paths associated with a result.
 */
export interface Path extends Result {
  pathIndex: number;
}

/**
 * Identifies one of the nodes in a path.
 */
export interface PathNode extends Path {
  pathNodeIndex: number;
}

/** Alias for `undefined` but more readable in some cases */
export const none: PathNode | undefined = undefined;

/**
 * Looks up a specific result in a result set.
 */
export function getResult(sarif: sarif.Log, key: Result): sarif.Result | undefined {
  if (sarif.runs.length === 0) return undefined;
  if (sarif.runs[0].results === undefined) return undefined;
  const results = sarif.runs[0].results;
  return results[key.resultIndex];
}

/**
 * Looks up a specific path in a result set.
 */
export function getPath(sarif: sarif.Log, key: Path): sarif.ThreadFlow | undefined {
  const result = getResult(sarif, key);
  if (result === undefined) return undefined;
  let index = -1;
  if (result.codeFlows === undefined) return undefined;
  for (const codeFlows of result.codeFlows) {
    for (const threadFlow of codeFlows.threadFlows) {
      ++index;
      if (index == key.pathIndex)
        return threadFlow;
    }
  }
  return undefined;
}

/**
 * Looks up a specific path node in a result set.
 */
export function getPathNode(sarif: sarif.Log, key: PathNode): sarif.Location | undefined {
  const path = getPath(sarif, key);
  if (path === undefined) return undefined;
  return path.locations[key.pathNodeIndex];
}

/**
 * Returns true if the two keys are both `undefined` or contain the same set of indices.
 */
export function equals(key1: PathNode | undefined, key2: PathNode | undefined): boolean {
  if (key1 === key2) return true;
  if (key1 === undefined || key2 === undefined) return false;
  return key1.resultIndex === key2.resultIndex && key1.pathIndex === key2.pathIndex && key1.pathNodeIndex === key2.pathNodeIndex;
}

/**
 * Returns true if the two keys contain the same set of indices and neither are `undefined`.
 */
export function equalsNotUndefined(key1: PathNode | undefined, key2: PathNode | undefined): boolean {
  if (key1 === undefined || key2 === undefined) return false;
  return key1.resultIndex === key2.resultIndex && key1.pathIndex === key2.pathIndex && key1.pathNodeIndex === key2.pathNodeIndex;
}

/**
 * Returns the list of paths in the given SARIF result.
 *
 * Path nodes indices are relative to this flattened list.
 */
export function getAllPaths(result: sarif.Result): sarif.ThreadFlow[] {
  if (result.codeFlows === undefined) return [];
  const paths = [];
  for (const codeFlow of result.codeFlows) {
    for (const threadFlow of codeFlow.threadFlows) {
      paths.push(threadFlow);
    }
  }
  return paths;
}
