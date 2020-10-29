import * as React from 'react';
import { ResultTableProps, className } from './result-table-utils';
import { RAW_RESULTS_LIMIT, RawResultsSortState } from '../interface-types';
import { RawTableResultSet } from '../interface-types';
import RawTableHeader from './RawTableHeader';
import RawTableRow from './RawTableRow';
import { ResultRow } from '../bqrs-cli-types';

export type RawTableProps = ResultTableProps & {
  resultSet: RawTableResultSet;
  sortState?: RawResultsSortState;
  offset: number;
};

export class RawTable extends React.Component<RawTableProps, {}> {
  constructor(props: RawTableProps) {
    super(props);
  }

  render(): React.ReactNode {
    const { resultSet, databaseUri } = this.props;

    let dataRows = resultSet.rows;
    let numTruncatedResults = 0;
    if (dataRows.length > RAW_RESULTS_LIMIT) {
      numTruncatedResults = dataRows.length - RAW_RESULTS_LIMIT;
      dataRows = dataRows.slice(0, RAW_RESULTS_LIMIT);
    }

    const tableRows = dataRows.map((row: ResultRow, rowIndex: number) =>
      <RawTableRow
        key={rowIndex}
        rowIndex={rowIndex + this.props.offset}
        row={row}
        databaseUri={databaseUri}
      />
    );

    if (numTruncatedResults > 0) {
      const colSpan = dataRows[0].length + 1; // one row for each data column, plus index column
      tableRows.push(<tr><td key={'message'} colSpan={colSpan} style={{ textAlign: 'center', fontStyle: 'italic' }}>
        Too many results to show at once. {numTruncatedResults} result(s) omitted.
      </td></tr>);
    }

    return <table className={className}>
      <RawTableHeader
        columns={resultSet.schema.columns}
        schemaName={resultSet.schema.name}
        sortState={this.props.sortState}
      />
      <tbody>
        {tableRows}
      </tbody>
    </table>;
  }
}
