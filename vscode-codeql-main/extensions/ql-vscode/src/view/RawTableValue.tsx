import * as React from 'react';

import { renderLocation } from './result-table-utils';
import { ColumnValue } from '../bqrs-cli-types';

interface Props {
  value: ColumnValue;
  databaseUri: string;
}

export default function RawTableValue(props: Props): JSX.Element {
  const v = props.value;
  if (
    typeof v === 'string'
    || typeof v === 'number'
    || typeof v === 'boolean'
  ) {
    return <span>{v}</span>;
  }

  return renderLocation(v.url, v.label, props.databaseUri);
}
