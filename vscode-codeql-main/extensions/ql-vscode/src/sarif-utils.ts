import * as Sarif from 'sarif';
import { ResolvableLocationValue } from './bqrs-cli-types';

export interface SarifLink {
  dest: number;
  text: string;
}

// The type of a result that has no associated location.
// hint is a string intended for display to the user
// that explains why there is no location.
interface NoLocation {
  hint: string;
}

type ParsedSarifLocation =
  | (ResolvableLocationValue & {

    userVisibleFile: string;
  })
  // Resolvable locations have a `uri` field, but it will sometimes include
  // a source location prefix, which contains build-specific information the user
  // doesn't really need to see. We ensure that `userVisibleFile` will not contain
  // that, and is appropriate for display in the UI.
  | NoLocation;

export type SarifMessageComponent = string | SarifLink

/**
 * Unescape "[", "]" and "\\" like in sarif plain text messages
 */
export function unescapeSarifText(message: string): string {
  return message
    .replace(/\\\[/g, '[')
    .replace(/\\\]/g, ']')
    .replace(/\\\\/g, '\\');
}

export function parseSarifPlainTextMessage(message: string): SarifMessageComponent[] {
  const results: SarifMessageComponent[] = [];

  // We want something like "[linkText](4)", except that "[" and "]" may be escaped. The lookbehind asserts
  // that the initial [ is not escaped. Then we parse a link text with "[" and "]" escaped. Then we parse the numerical target.
  // Technically we could have any uri in the target but we don't output that yet.
  // The possibility of escaping outside the link is not mentioned in the sarif spec but we always output sartif this way.
  const linkRegex = /(?<=(?<!\\)(\\\\)*)\[(?<linkText>([^\\\]\[]|\\\\|\\\]|\\\[)*)\]\((?<linkTarget>[0-9]+)\)/g;
  let result: RegExpExecArray | null;
  let curIndex = 0;
  while ((result = linkRegex.exec(message)) !== null) {
    results.push(unescapeSarifText(message.substring(curIndex, result.index)));
    const linkText = result.groups!['linkText'];
    const linkTarget = +result.groups!['linkTarget'];
    results.push({ dest: linkTarget, text: unescapeSarifText(linkText) });
    curIndex = result.index + result[0].length;
  }
  results.push(unescapeSarifText(message.substring(curIndex, message.length)));
  return results;
}


/**
 * Computes a path normalized to reflect conventional normalization
 * of windows paths into zip archive paths.
 * @param sourceLocationPrefix The source location prefix of a database. May be
 * unix style `/foo/bar/baz` or windows-style `C:\foo\bar\baz`.
 * @param sarifRelativeUri A uri relative to sourceLocationPrefix.
 *
 * @returns A URI string that is valid for the `.file` field of a `FivePartLocation`:
 * directory separators are normalized, but drive letters `C:` may appear.
 */
export function getPathRelativeToSourceLocationPrefix(
  sourceLocationPrefix: string,
  sarifRelativeUri: string
) {
  const normalizedSourceLocationPrefix = sourceLocationPrefix.replace(/\\/g, '/');
  return `file:${normalizedSourceLocationPrefix}/${sarifRelativeUri}`;
}

export function parseSarifLocation(
  loc: Sarif.Location,
  sourceLocationPrefix: string
): ParsedSarifLocation {
  const physicalLocation = loc.physicalLocation;
  if (physicalLocation === undefined)
    return { hint: 'no physical location' };
  if (physicalLocation.artifactLocation === undefined)
    return { hint: 'no artifact location' };
  if (physicalLocation.artifactLocation.uri === undefined)
    return { hint: 'artifact location has no uri' };

  // This is not necessarily really an absolute uri; it could either be a
  // file uri or a relative uri.
  const uri = physicalLocation.artifactLocation.uri;

  const fileUriRegex = /^file:/;
  const hasFilePrefix = uri.match(fileUriRegex);
  const effectiveLocation = hasFilePrefix
    ? uri
    : getPathRelativeToSourceLocationPrefix(sourceLocationPrefix, uri);
  const userVisibleFile = decodeURIComponent(hasFilePrefix
    ? uri.replace(fileUriRegex, '')
    : uri);

  if (physicalLocation.region === undefined) {
    // If the region property is absent, the physicalLocation object refers to the entire file.
    // Source: https://docs.oasis-open.org/sarif/sarif/v2.1.0/cs01/sarif-v2.1.0-cs01.html#_Toc16012638.
    return {
      uri: effectiveLocation,
      userVisibleFile
    } as ParsedSarifLocation;
  } else {
    const region = physicalLocation.region;
    // We assume that the SARIF we're given always has startLine
    // This is not mandated by the SARIF spec, but should be true of
    // SARIF output by our own tools.
    const startLine = region.startLine!;

    // These defaults are from SARIF 2.1.0 spec, section 3.30.2, "Text Regions"
    // https://docs.oasis-open.org/sarif/sarif/v2.1.0/cs01/sarif-v2.1.0-cs01.html#_Ref493492556
    const endLine = region.endLine === undefined ? startLine : region.endLine;
    const startColumn = region.startColumn === undefined ? 1 : region.startColumn;

    // We also assume that our tools will always supply `endColumn` field, which is
    // fortunate, since the SARIF spec says that it defaults to the end of the line, whose
    // length we don't know at this point in the code.
    //
    // It is off by one with respect to the way vscode counts columns in selections.
    const endColumn = region.endColumn! - 1;

    return {
      uri: effectiveLocation,
      userVisibleFile,
      startLine,
      startColumn,
      endLine,
      endColumn,
    };
  }
}

export function isNoLocation(loc: ParsedSarifLocation): loc is NoLocation {
  return 'hint' in loc;
}
