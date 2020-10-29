import * as React from 'react';

interface Props {
  availableResultSets: string[];
  currentResultSetName: string;
  updateResultSet: (newResultSet: string) => void;
}

export default function CompareSelector(props: Props) {
  return (
    <select
      value={props.currentResultSetName}
      onChange={(e) => props.updateResultSet(e.target.value)}
    >
      {props.availableResultSets.map((resultSet) => (
        <option key={resultSet} value={resultSet}>
          {resultSet}
        </option>
      ))}
    </select>
  );
}
