import * as React from 'react';
import * as Rdom from 'react-dom';
import { assertNever } from '../helpers-pure';
import {
  DatabaseInfo,
  Interpretation,
  IntoResultsViewMsg,
  SortedResultSetInfo,
  RawResultsSortState,
  NavigatePathMsg,
  QueryMetadata,
  ResultsPaths,
  ALERTS_TABLE_NAME,
  ParsedResultSets
} from '../interface-types';
import { EventHandlers as EventHandlerList } from './event-handler-list';
import { ResultTables } from './result-tables';
import { ResultSet } from '../interface-types';
import { vscode } from './vscode-api';

/**
 * results.tsx
 * -----------
 *
 * Displaying query results.
 */

interface ResultsInfo {
  parsedResultSets: ParsedResultSets;
  resultsPath: string;
  origResultsPaths: ResultsPaths;
  database: DatabaseInfo;
  interpretation: Interpretation | undefined;
  sortedResultsMap: Map<string, SortedResultSetInfo>;
  /**
   * See {@link SetStateMsg.shouldKeepOldResultsWhileRendering}.
   */
  shouldKeepOldResultsWhileRendering: boolean;
  metadata?: QueryMetadata;
}

interface Results {
  resultSets: readonly ResultSet[];
  sortStates: Map<string, RawResultsSortState>;
  database: DatabaseInfo;
}

interface ResultsState {
  // We use `null` instead of `undefined` here because in React, `undefined` is
  // used to mean "did not change" when updating the state of a component.
  resultsInfo: ResultsInfo | null;
  results: Results | null;
  errorMessage: string;
}

interface ResultsViewState {
  displayedResults: ResultsState;
  nextResultsInfo: ResultsInfo | null;
  isExpectingResultsUpdate: boolean;
}

export type NavigationEvent = NavigatePathMsg;

/**
 * Event handlers to be notified of navigation events coming from outside the webview.
 */
export const onNavigation = new EventHandlerList<NavigationEvent>();

/**
 * A minimal state container for displaying results.
 */
class App extends React.Component<{}, ResultsViewState> {
  constructor(props: any) {
    super(props);
    this.state = {
      displayedResults: {
        resultsInfo: null,
        results: null,
        errorMessage: '',
      },
      nextResultsInfo: null,
      isExpectingResultsUpdate: true,
    };
  }

  handleMessage(msg: IntoResultsViewMsg): void {
    switch (msg.t) {
      case 'setState':
        this.updateStateWithNewResultsInfo({
          resultsPath: msg.resultsPath,
          parsedResultSets: msg.parsedResultSets,
          origResultsPaths: msg.origResultsPaths,
          sortedResultsMap: new Map(Object.entries(msg.sortedResultsMap)),
          database: msg.database,
          interpretation: msg.interpretation,
          shouldKeepOldResultsWhileRendering:
            msg.shouldKeepOldResultsWhileRendering,
          metadata: msg.metadata,
        });

        this.loadResults();
        break;
      case 'showInterpretedPage':
        this.updateStateWithNewResultsInfo({
          resultsPath: '', // FIXME: Not used for interpreted, refactor so this is not needed
          parsedResultSets: {
            numPages: msg.numPages,
            numInterpretedPages: msg.numPages,
            resultSetNames: msg.resultSetNames,
            pageNumber: msg.pageNumber,
            resultSet: {
              t: 'SarifResultSet',
              name: ALERTS_TABLE_NAME,
              schema: {
                name: ALERTS_TABLE_NAME,
                rows: 1,
                columns: []
              },
              ...msg.interpretation,
            },
            selectedTable: ALERTS_TABLE_NAME,
          },
          origResultsPaths: undefined as any, // FIXME: Not used for interpreted, refactor so this is not needed
          sortedResultsMap: new Map(), // FIXME: Not used for interpreted, refactor so this is not needed
          database: msg.database,
          interpretation: msg.interpretation,
          shouldKeepOldResultsWhileRendering: true,
          metadata: msg.metadata,
        });
        this.loadResults();
        break;
      case 'resultsUpdating':
        this.setState({
          isExpectingResultsUpdate: true,
        });
        break;
      case 'navigatePath':
        onNavigation.fire(msg);
        break;

      case 'untoggleShowProblems':
        // noop
        break;

      default:
        assertNever(msg);
    }
  }

  private updateStateWithNewResultsInfo(resultsInfo: ResultsInfo): void {
    this.setState((prevState) => {
      const stateWithDisplayedResults = (
        displayedResults: ResultsState
      ): ResultsViewState => ({
        displayedResults,
        isExpectingResultsUpdate: prevState.isExpectingResultsUpdate,
        nextResultsInfo: resultsInfo,
      });

      if (!prevState.isExpectingResultsUpdate && resultsInfo === null) {
        // No results to display
        return stateWithDisplayedResults({
          resultsInfo: null,
          results: null,
          errorMessage: 'No results to display',
        });
      }
      if (!resultsInfo || !resultsInfo.shouldKeepOldResultsWhileRendering) {
        // Display loading message
        return stateWithDisplayedResults({
          resultsInfo: null,
          results: null,
          errorMessage: 'Loading results…',
        });
      }
      return stateWithDisplayedResults(prevState.displayedResults);
    });
  }

  private getResultSets(
    resultsInfo: ResultsInfo
  ): readonly ResultSet[] {
    const parsedResultSets = resultsInfo.parsedResultSets;
    const resultSet = parsedResultSets.resultSet;
    if (!resultSet.t) {
      throw new Error(
        'Missing result set type. Should be either "SarifResultSet" or "RawResultSet".'
      );
    }
    return [resultSet];
  }

  private async loadResults(): Promise<void> {
    const resultsInfo = this.state.nextResultsInfo;
    if (resultsInfo === null) {
      return;
    }

    let results: Results | null = null;
    let statusText = '';
    try {
      const resultSets = this.getResultSets(resultsInfo);
      results = {
        resultSets,
        database: resultsInfo.database,
        sortStates: this.getSortStates(resultsInfo),
      };
    } catch (e) {
      let errorMessage: string;
      if (e instanceof Error) {
        errorMessage = e.message;
      } else {
        errorMessage = 'Unknown error';
      }

      statusText = `Error loading results: ${errorMessage}`;
    }

    this.setState((prevState) => {
      // Only set state if this results info is still current.
      if (resultsInfo !== prevState.nextResultsInfo) {
        return null;
      }
      return {
        displayedResults: {
          resultsInfo,
          results,
          errorMessage: statusText,
        },
        nextResultsInfo: null,
        isExpectingResultsUpdate: false,
      };
    });
  }

  private getSortStates(
    resultsInfo: ResultsInfo
  ): Map<string, RawResultsSortState> {
    const entries = Array.from(resultsInfo.sortedResultsMap.entries());
    return new Map(
      entries.map(([key, sortedResultSetInfo]) => [
        key,
        sortedResultSetInfo.sortState,
      ])
    );
  }

  render(): JSX.Element {
    const displayedResults = this.state.displayedResults;
    if (
      displayedResults.results !== null &&
      displayedResults.resultsInfo !== null
    ) {
      const parsedResultSets = displayedResults.resultsInfo.parsedResultSets;
      const key = (parsedResultSets.selectedTable || '') + parsedResultSets.pageNumber;
      return (
        <ResultTables
          key={key}
          parsedResultSets={parsedResultSets}
          rawResultSets={displayedResults.results.resultSets}
          interpretation={
            displayedResults.resultsInfo
              ? displayedResults.resultsInfo.interpretation
              : undefined
          }
          database={displayedResults.results.database}
          origResultsPaths={displayedResults.resultsInfo.origResultsPaths}
          resultsPath={displayedResults.resultsInfo.resultsPath}
          metadata={
            displayedResults.resultsInfo
              ? displayedResults.resultsInfo.metadata
              : undefined
          }
          sortStates={displayedResults.results.sortStates}
          interpretedSortState={
            displayedResults.resultsInfo.interpretation?.sortState
          }
          isLoadingNewResults={
            this.state.isExpectingResultsUpdate ||
            this.state.nextResultsInfo !== null
          }
        />
      );
    } else {
      return <span>{displayedResults.errorMessage}</span>;
    }
  }

  componentDidMount(): void {
    this.vscodeMessageHandler = this.vscodeMessageHandler.bind(this);
    window.addEventListener('message', this.vscodeMessageHandler);
  }

  componentWillUnmount(): void {
    if (this.vscodeMessageHandler) {
      window.removeEventListener('message', this.vscodeMessageHandler);
    }
  }

  private vscodeMessageHandler(evt: MessageEvent) {
    evt.origin === window.origin
      ? this.handleMessage(evt.data as IntoResultsViewMsg)
      : console.error(`Invalid event origin ${evt.origin}`);
  }
}

Rdom.render(<App />, document.getElementById('root'));

vscode.postMessage({ t: 'resultViewLoaded' });
