import * as React from 'react';
import { UrlValue, ResolvableLocationValue } from '../bqrs-cli-types';
import { isStringLoc, tryGetResolvableLocation } from '../bqrs-utils';
import { RawResultsSortState, QueryMetadata, SortDirection } from '../interface-types';
import { assertNever } from '../helpers-pure';
import { ResultSet } from '../interface-types';
import { vscode } from './vscode-api';

export interface ResultTableProps {
  resultSet: ResultSet;
  databaseUri: string;
  metadata?: QueryMetadata;
  resultsPath: string | undefined;
  sortState?: RawResultsSortState;
  offset: number;

  /**
   * Holds if there are any raw results. When that is the case, we
   * want to direct users to pay attention to raw results if
   * interpreted results are empty.
   */
  nonemptyRawResults: boolean;

  /**
   * Callback to show raw results.
   */
  showRawResults: () => void;
}

export const className = 'vscode-codeql__result-table';
export const tableSelectionHeaderClassName = 'vscode-codeql__table-selection-header';
export const alertExtrasClassName = `${className}-alert-extras`;
export const toggleDiagnosticsClassName = `${className}-toggle-diagnostics`;
export const evenRowClassName = 'vscode-codeql__result-table-row--even';
export const oddRowClassName = 'vscode-codeql__result-table-row--odd';
export const pathRowClassName = 'vscode-codeql__result-table-row--path';
export const selectedRowClassName = 'vscode-codeql__result-table-row--selected';

export function jumpToLocationHandler(
  loc: ResolvableLocationValue,
  databaseUri: string,
  callback?: () => void
): (e: React.MouseEvent) => void {
  return (e) => {
    jumpToLocation(loc, databaseUri);
    e.preventDefault();
    e.stopPropagation();
    if (callback) callback();
  };
}

export function jumpToLocation(loc: ResolvableLocationValue, databaseUri: string): void {
  vscode.postMessage({
    t: 'viewSourceFile',
    loc,
    databaseUri
  });
}

/**
 * Render a location as a link which when clicked displays the original location.
 */
export function renderLocation(
  loc: UrlValue | undefined,
  label: string | undefined,
  databaseUri: string,
  title?: string,
  callback?: () => void
): JSX.Element {

  // If the label was empty, use a placeholder instead, so the link is still clickable.
  let displayLabel = label;
  if (!label) {
    displayLabel = '[empty string]';
  } else if (label.match(/^\s+$/)) {
    displayLabel = `[whitespace: "${label}"]`;
  }

  if (loc === undefined) {
    return <span>{displayLabel}</span>;
  } else if (isStringLoc(loc)) {
    return <a href={loc}>{loc}</a>;
  }

  const resolvableLoc = tryGetResolvableLocation(loc);
  if (resolvableLoc !== undefined) {
    return (
      <a href="#"
        className="vscode-codeql__result-table-location-link"
        title={title}
        onClick={jumpToLocationHandler(resolvableLoc, databaseUri, callback)}>
        {displayLabel}
      </a>
    );
  } else {
    return <span title={title}>{displayLabel}</span>;
  }
}

/**
 * Returns the attributes for a zebra-striped table row at position `index`.
 */
export function zebraStripe(index: number, ...otherClasses: string[]): { className: string } {
  return { className: [(index % 2) ? oddRowClassName : evenRowClassName, ...otherClasses].join(' ') };
}

/**
 * Returns the attributes for a zebra-striped table row at position `index`,
 * with highlighting if `isSelected` is true.
 */
export function selectableZebraStripe(isSelected: boolean, index: number, ...otherClasses: string[]): { className: string } {
  return isSelected
    ? { className: [selectedRowClassName, ...otherClasses].join(' ') }
    : zebraStripe(index, ...otherClasses);
}

/**
 * Returns the next sort direction when cycling through sort directions while clicking.
 * if `includeUndefined` is true, include `undefined` in the cycle.
 */
export function nextSortDirection(direction: SortDirection | undefined, includeUndefined?: boolean): SortDirection | undefined {
  switch (direction) {
    case SortDirection.asc:
      return SortDirection.desc;
    case SortDirection.desc:
      return includeUndefined ? undefined : SortDirection.asc;
    case undefined:
      return SortDirection.asc;
    default:
      return assertNever(direction);
  }
}
