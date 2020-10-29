import * as React from 'react';
import {
  DatabaseInfo,
  Interpretation,
  RawResultsSortState,
  QueryMetadata,
  ResultsPaths,
  InterpretedResultsSortState,
  RAW_RESULTS_PAGE_SIZE,
  ResultSet,
  ALERTS_TABLE_NAME,
  SELECT_TABLE_NAME,
  getDefaultResultSetName,
  ParsedResultSets,
  IntoResultsViewMsg
} from '../interface-types';
import { PathTable } from './alert-table';
import { RawTable } from './raw-results-table';
import { ResultTableProps, tableSelectionHeaderClassName, toggleDiagnosticsClassName, alertExtrasClassName } from './result-table-utils';
import { vscode } from './vscode-api';


/**
 * Properties for the `ResultTables` component.
 */
export interface ResultTablesProps {
  parsedResultSets: ParsedResultSets;
  rawResultSets: readonly ResultSet[];
  interpretation: Interpretation | undefined;
  database: DatabaseInfo;
  metadata?: QueryMetadata;
  resultsPath: string;
  origResultsPaths: ResultsPaths;
  sortStates: Map<string, RawResultsSortState>;
  interpretedSortState?: InterpretedResultsSortState;
  isLoadingNewResults: boolean;
}

/**
 * State for the `ResultTables` component.
 */
interface ResultTablesState {
  selectedTable: string; // name of selected result set
  selectedPage: string; // stringified selected page
  problemsViewSelected: boolean;
}

const UPDATING_RESULTS_TEXT_CLASS_NAME = 'vscode-codeql__result-tables-updating-text';

function getResultCount(resultSet: ResultSet): number {
  switch (resultSet.t) {
    case 'RawResultSet':
      return resultSet.schema.rows;
    case 'SarifResultSet':
      return resultSet.numTotalResults;
  }
}

function renderResultCountString(resultSet: ResultSet): JSX.Element {
  const resultCount = getResultCount(resultSet);
  return <span className="number-of-results">
    {resultCount} {resultCount === 1 ? 'result' : 'results'}
  </span>;
}

/**
 * Displays multiple `ResultTable` tables, where the table to be displayed is selected by a
 * dropdown.
 */
export class ResultTables
  extends React.Component<ResultTablesProps, ResultTablesState> {

  private getResultSets(): ResultSet[] {
    const resultSets: ResultSet[] =
      // eslint-disable-next-line @typescript-eslint/ban-ts-ignore
      // @ts-ignore 2783
      this.props.rawResultSets.map((rs) => ({ t: 'RawResultSet', ...rs }));

    if (this.props.interpretation != undefined) {
      resultSets.push({
        t: 'SarifResultSet',
        // FIXME: The values of version, columns, tupleCount are
        // unused stubs because a SarifResultSet schema isn't used the
        // same way as a RawResultSet. Probably should pull `name` field
        // out.
        schema: {
          name: ALERTS_TABLE_NAME,
          rows: 1,
          columns: []
        },
        name: ALERTS_TABLE_NAME,
        ...this.props.interpretation,
      });
    }
    return resultSets;
  }

  private getResultSetNames(resultSets: ResultSet[]): string[] {
    return this.props.parsedResultSets.resultSetNames.concat([ALERTS_TABLE_NAME]);
  }

  constructor(props: ResultTablesProps) {
    super(props);
    const selectedTable = props.parsedResultSets.selectedTable || getDefaultResultSet(this.getResultSets());
    const selectedPage = (props.parsedResultSets.pageNumber + 1) + '';
    this.state = {
      selectedTable,
      selectedPage,
      problemsViewSelected: false
    };
  }

  untoggleProblemsView() {
    this.setState({
      problemsViewSelected: false
    });
  }

  private onTableSelectionChange = (event: React.ChangeEvent<HTMLSelectElement>): void => {
    const selectedTable = event.target.value;
    vscode.postMessage({
      t: 'changePage',
      pageNumber: 0,
      selectedTable
    });
  }

  private alertTableExtras(): JSX.Element | undefined {
    const { database, resultsPath, metadata, origResultsPaths } = this.props;
    const handleCheckboxChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
      if (e.target.checked === this.state.problemsViewSelected) {
        // no change
        return;
      }
      this.setState({
        problemsViewSelected: e.target.checked
      });
      if (resultsPath !== undefined) {
        vscode.postMessage({
          t: 'toggleDiagnostics',
          origResultsPaths: origResultsPaths,
          databaseUri: database.databaseUri,
          visible: e.target.checked,
          metadata: metadata
        });
      }
    };

    return (
      <div className={alertExtrasClassName}>
        <div className={toggleDiagnosticsClassName}>
          <input
            type="checkbox"
            id="toggle-diagnostics"
            name="toggle-diagnostics"
            onChange={handleCheckboxChanged}
            checked={this.state.problemsViewSelected}
          />
          <label htmlFor="toggle-diagnostics">Show results in Problems view</label>
        </div>
      </div>
    );
  }

  getOffset(): number {
    const { parsedResultSets } = this.props;
    return parsedResultSets.pageNumber * RAW_RESULTS_PAGE_SIZE;
  }

  renderPageButtons(): JSX.Element {
    const { parsedResultSets } = this.props;
    const selectedTable = this.state.selectedTable;

    // FIXME: The extension, not the view, should be in charge of deciding whether to initially show
    // a raw or alerts page. We have to conditionally recompute the number of pages here, because
    // on initial load of query results, resultSets.numPages will have the number of *raw* pages available,
    // not interpreted pages, because the extension doesn't know the view will default to showing alerts
    // instead.
    const numPages = Math.max(selectedTable === ALERTS_TABLE_NAME ?
      parsedResultSets.numInterpretedPages : parsedResultSets.numPages, 1);

    const onChange = (e: React.ChangeEvent<HTMLInputElement>) => {
      this.setState({ selectedPage: e.target.value });
    };
    const choosePage = (input: string) => {
      const pageNumber = parseInt(input);
      if (pageNumber !== undefined && !isNaN(pageNumber)) {
        const actualPageNumber = Math.max(0, Math.min(pageNumber - 1, numPages - 1));
        vscode.postMessage({
          t: 'changePage',
          pageNumber: actualPageNumber,
          selectedTable,
        });
      }
    };

    const prevPage = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
      vscode.postMessage({
        t: 'changePage',
        pageNumber: Math.max(parsedResultSets.pageNumber - 1, 0),
        selectedTable,
      });
    };
    const nextPage = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
      vscode.postMessage({
        t: 'changePage',
        pageNumber: Math.min(parsedResultSets.pageNumber + 1, numPages - 1),
        selectedTable,
      });
    };

    return <span className="vscode-codeql__table-selection-header">
      <button onClick={prevPage} >&#xab;</button>
      <input
        type="number"
        size={3}
        value={this.state.selectedPage}
        onChange={onChange}
        onBlur={e => choosePage(e.target.value)}
        onKeyDown={e => {
          if (e.keyCode === 13) {
            choosePage((e.target as HTMLInputElement).value);
          }
        }
        }
      />
      <span>
        / {numPages}
      </span>
      <button value=">" onClick={nextPage} >&#xbb;</button>
    </span>;
  }

  render(): React.ReactNode {
    const { selectedTable } = this.state;
    const resultSets = this.getResultSets();
    const resultSetNames = this.getResultSetNames(resultSets);

    const resultSet = resultSets.find(resultSet => resultSet.schema.name == selectedTable);
    const nonemptyRawResults = resultSets.some(resultSet => resultSet.t == 'RawResultSet' && resultSet.rows.length > 0);
    const numberOfResults = resultSet && renderResultCountString(resultSet);

    const resultSetOptions =
      resultSetNames.map(name => <option key={name} value={name}>{name}</option>);

    return <div>
      {this.renderPageButtons()}
      <div className={tableSelectionHeaderClassName}>
        <select value={selectedTable} onChange={this.onTableSelectionChange}>
          {resultSetOptions}
        </select>
        {numberOfResults}
        {selectedTable === ALERTS_TABLE_NAME ? this.alertTableExtras() : undefined}
        {
          this.props.isLoadingNewResults ?
            <span className={UPDATING_RESULTS_TEXT_CLASS_NAME}>Updating results…</span>
            : null
        }
      </div>
      {
        resultSet &&
        <ResultTable key={resultSet.schema.name} resultSet={resultSet}
          databaseUri={this.props.database.databaseUri}
          resultsPath={this.props.resultsPath}
          sortState={this.props.sortStates.get(resultSet.schema.name)}
          nonemptyRawResults={nonemptyRawResults}
          showRawResults={() => { this.setState({ selectedTable: SELECT_TABLE_NAME }); }}
          offset={this.getOffset()} />
      }
    </div>;
  }

  handleMessage(msg: IntoResultsViewMsg): void {
    switch (msg.t) {
      case 'untoggleShowProblems':
        this.setState({
          problemsViewSelected: false
        });
        break;

      default:
        // noop
    }
  }

  // TODO: Duplicated from results.tsx consider a way to
  // avoid this duplication
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

class ResultTable extends React.Component<ResultTableProps, {}> {

  constructor(props: ResultTableProps) {
    super(props);
  }

  render(): React.ReactNode {
    const { resultSet } = this.props;
    switch (resultSet.t) {
      case 'RawResultSet': return <RawTable
        {...this.props} resultSet={resultSet} />;
      case 'SarifResultSet': return <PathTable
        {...this.props} resultSet={resultSet} />;
    }
  }
}

function getDefaultResultSet(resultSets: readonly ResultSet[]): string {
  return getDefaultResultSetName(
    resultSets.map((resultSet) => resultSet.schema.name)
  );
}
