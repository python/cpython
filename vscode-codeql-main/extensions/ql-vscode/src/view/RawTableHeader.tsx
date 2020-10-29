import * as React from 'react';

import { vscode } from './vscode-api';
import { RawResultsSortState, SortDirection } from '../interface-types';
import { nextSortDirection } from './result-table-utils';
import { Column } from '../bqrs-cli-types';

interface Props {
  readonly columns: readonly Column[];
  readonly schemaName: string;
  readonly sortState?: RawResultsSortState;
  readonly preventSort?: boolean;
}

function toggleSortStateForColumn(
  index: number,
  schemaName: string,
  sortState: RawResultsSortState | undefined,
  preventSort: boolean
): void {
  if (preventSort) {
    return;
  }

  const prevDirection =
    sortState && sortState.columnIndex === index
      ? sortState.sortDirection
      : undefined;
  const nextDirection = nextSortDirection(prevDirection);
  const nextSortState =
    nextDirection === undefined
      ? undefined
      : {
        columnIndex: index,
        sortDirection: nextDirection,
      };
  vscode.postMessage({
    t: 'changeSort',
    resultSetName: schemaName,
    sortState: nextSortState,
  });
}

export default function RawTableHeader(props: Props) {
  return (
    <thead>
      <tr>
        {[
          (
            <th key={-1}>
              <b>#</b>
            </th>
          ),
          ...props.columns.map((col, index) => {
            const displayName = col.name || `[${index}]`;
            const sortDirection =
              props.sortState && index === props.sortState.columnIndex
                ? props.sortState.sortDirection
                : undefined;
            return (
              <th
                className={
                  'sort-' +
                  (sortDirection !== undefined
                    ? SortDirection[sortDirection]
                    : 'none')
                }
                key={index}
                onClick={() =>
                  toggleSortStateForColumn(
                    index,
                    props.schemaName,
                    props.sortState,
                    !!props.preventSort
                  )
                }
              >
                <b>{displayName}</b>
              </th>
            );
          }),
        ]}
      </tr>
    </thead>
  );
}
