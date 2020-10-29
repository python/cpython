import { UrlValue, ResolvableLocationValue, LineColumnLocation, WholeFileLocation } from './bqrs-cli-types';

/**
 * The CodeQL filesystem libraries use this pattern in `getURL()` predicates
 * to describe the location of an entire filesystem resource.
 * Such locations appear as `StringLocation`s instead of `FivePartLocation`s.
 *
 * Folder resources also get similar URLs, but with the `folder` scheme.
 * They are deliberately ignored here, since there is no suitable location to show the user.
 */
const FILE_LOCATION_REGEX = /file:\/\/(.+):([0-9]+):([0-9]+):([0-9]+):([0-9]+)/;
/**
 * Gets a resolvable source file location for the specified `LocationValue`, if possible.
 * @param loc The location to test.
 */
export function tryGetResolvableLocation(
  loc: UrlValue | undefined
): ResolvableLocationValue | undefined {
  let resolvedLoc;
  if (loc === undefined) {
    resolvedLoc = undefined;
  } else if (isWholeFileLoc(loc) || isLineColumnLoc(loc)) {
    resolvedLoc = loc as ResolvableLocationValue;
  } else if (isStringLoc(loc)) {
    resolvedLoc = tryGetLocationFromString(loc);
  } else {
    resolvedLoc = undefined;
  }

  return resolvedLoc;
}

export function tryGetLocationFromString(
  loc: string
): ResolvableLocationValue | undefined {
  const matches = FILE_LOCATION_REGEX.exec(loc);
  if (matches && matches.length > 1 && matches[1]) {
    if (isWholeFileMatch(matches)) {
      return {
        uri: matches[1],
      } as WholeFileLocation;
    } else {
      return {
        uri: matches[1],
        startLine: Number(matches[2]),
        startColumn: Number(matches[3]),
        endLine: Number(matches[4]),
        endColumn: Number(matches[5]),
      };
    }
  } else {
    return undefined;
  }
}

function isWholeFileMatch(matches: RegExpExecArray): boolean {
  return (
    matches[2] === '0' &&
    matches[3] === '0' &&
    matches[4] === '0' &&
    matches[5] === '0'
  );
}

/**
 * Checks whether the file path is empty. If so, we do not want to render this location
 * as a link.
 *
 * @param uri A file uri
 */
export function isEmptyPath(uriStr: string) {
  return !uriStr || uriStr === 'file:/';
}

export function isLineColumnLoc(loc: UrlValue): loc is LineColumnLocation {
  return typeof loc !== 'string'
    && !isEmptyPath(loc.uri)
    && 'startLine' in loc
    && 'startColumn' in loc
    && 'endLine' in loc
    && 'endColumn' in loc
    && loc.endColumn > 0;
}

export function isWholeFileLoc(loc: UrlValue): loc is WholeFileLocation {
  return typeof loc !== 'string' && !isEmptyPath(loc.uri) && !isLineColumnLoc(loc);
}

export function isStringLoc(loc: UrlValue): loc is string {
  return typeof loc === 'string';
}
