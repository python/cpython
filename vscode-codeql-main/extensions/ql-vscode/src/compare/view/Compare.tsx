import * as React from 'react';
import { useState, useEffect } from 'react';
import * as Rdom from 'react-dom';

import {
  ToCompareViewMessage,
  SetComparisonsMessage,
} from '../../interface-types';
import CompareSelector from './CompareSelector';
import { vscode } from '../../view/vscode-api';
import CompareTable from './CompareTable';

const emptyComparison: SetComparisonsMessage = {
  t: 'setComparisons',
  stats: {},
  rows: undefined,
  columns: [],
  commonResultSetNames: [],
  currentResultSetName: '',
  datebaseUri: '',
  message: 'Empty comparison'
};

export function Compare(_: {}): JSX.Element {
  const [comparison, setComparison] = useState<SetComparisonsMessage>(
    emptyComparison
  );

  const message = comparison.message || 'Empty comparison';
  const hasRows = comparison.rows && (comparison.rows.to.length || comparison.rows.from.length);

  useEffect(() => {
    window.addEventListener('message', (evt: MessageEvent) => {
      if (evt.origin === window.origin) {
        const msg: ToCompareViewMessage = evt.data;
        switch (msg.t) {
          case 'setComparisons':
            setComparison(msg);
        }
      } else {
        console.error(`Invalid event origin ${evt.origin}`);
      }
    });
  });
  if (!comparison) {
    return <div>Waiting for results to load.</div>;
  }

  try {
    return (
      <>
        <div className="vscode-codeql__compare-header">
          <div className="vscode-codeql__compare-header-item">
            Table to compare:
          </div>
          <CompareSelector
            availableResultSets={comparison.commonResultSetNames}
            currentResultSetName={comparison.currentResultSetName}
            updateResultSet={(newResultSetName: string) =>
              vscode.postMessage({ t: 'changeCompare', newResultSetName })
            }
          />
        </div>
        {hasRows ? (
          <CompareTable comparison={comparison}></CompareTable>
        ) : (
            <div className="vscode-codeql__compare-message">{message}</div>
          )}
      </>
    );
  } catch (err) {
    console.error(err);
    return <div>Error!</div>;
  }
}

Rdom.render(
  <Compare />,
  document.getElementById('root'),
  // Post a message to the extension when fully loaded.
  () => vscode.postMessage({ t: 'compareViewLoaded' })
);
