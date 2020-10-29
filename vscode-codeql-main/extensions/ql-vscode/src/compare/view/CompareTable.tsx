import * as React from 'react';

import { SetComparisonsMessage } from '../../interface-types';
import RawTableHeader from '../../view/RawTableHeader';
import { className } from '../../view/result-table-utils';
import { ResultRow } from '../../bqrs-cli-types';
import RawTableRow from '../../view/RawTableRow';
import { vscode } from '../../view/vscode-api';

interface Props {
  comparison: SetComparisonsMessage;
}

export default function CompareTable(props: Props) {
  const comparison = props.comparison;
  const rows = props.comparison.rows!;

  async function openQuery(kind: 'from' | 'to') {
    vscode.postMessage({
      t: 'openQuery',
      kind,
    });
  }

  function createRows(rows: ResultRow[], databaseUri: string) {
    return (
      <tbody>
        {rows.map((row, rowIndex) => (
          <RawTableRow
            key={rowIndex}
            rowIndex={rowIndex}
            row={row}
            databaseUri={databaseUri}
          />
        ))}
      </tbody>
    );
  }

  return (
    <table className='vscode-codeql__compare-body'>
      <thead>
        <tr>
          <td>
            <a
              onClick={() => openQuery('from')}
              className='vscode-codeql__compare-open'
            >
              {comparison.stats.fromQuery?.name}
            </a>
          </td>
          <td>
            <a
              onClick={() => openQuery('to')}
              className='vscode-codeql__compare-open'
            >
              {comparison.stats.toQuery?.name}
            </a>
          </td>
        </tr>
        <tr>
          <td>{comparison.stats.fromQuery?.time}</td>
          <td>{comparison.stats.toQuery?.time}</td>
        </tr>
        <tr>
          <th>{rows.from.length} rows removed</th>
          <th>{rows.to.length} rows added</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>
            <table className={className}>
              <RawTableHeader
                columns={comparison.columns}
                schemaName={comparison.currentResultSetName}
                preventSort={true}
              />
              {createRows(rows.from, comparison.datebaseUri)}
            </table>
          </td>
          <td>
            <table className={className}>
              <RawTableHeader
                columns={comparison.columns}
                schemaName={comparison.currentResultSetName}
                preventSort={true}
              />
              {createRows(rows.to, comparison.datebaseUri)}
            </table>
          </td>
        </tr>
      </tbody>
    </table>
  );
}
