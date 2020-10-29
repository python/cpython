import * as React from 'react';
import { ResultRow } from '../bqrs-cli-types';
import { zebraStripe } from './result-table-utils';
import RawTableValue from './RawTableValue';

interface Props {
  rowIndex: number;
  row: ResultRow;
  databaseUri: string;
  className?: string;
}

export default function RawTableRow(props: Props) {
  return (
    <tr key={props.rowIndex} {...zebraStripe(props.rowIndex, props.className || '')}>
      <td key={-1}>{props.rowIndex + 1}</td>

      {props.row.map((value, columnIndex) => (
        <td key={columnIndex}>
          <RawTableValue
            value={value}
            databaseUri={props.databaseUri}
          />
        </td>
      ))}
    </tr>
  );
}
