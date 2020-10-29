import * as path from 'path';
import * as React from 'react';
import * as Sarif from 'sarif';
import * as Keys from '../result-keys';
import * as octicons from './octicons';
import { className, renderLocation, ResultTableProps, zebraStripe, selectableZebraStripe, jumpToLocation, nextSortDirection } from './result-table-utils';
import { onNavigation, NavigationEvent } from './results';
import { PathTableResultSet } from '../interface-types';
import { parseSarifPlainTextMessage, parseSarifLocation, isNoLocation } from '../sarif-utils';
import { InterpretedResultsSortColumn, SortDirection, InterpretedResultsSortState } from '../interface-types';
import { vscode } from './vscode-api';
import { isWholeFileLoc, isLineColumnLoc } from '../bqrs-utils';

export type PathTableProps = ResultTableProps & { resultSet: PathTableResultSet };
export interface PathTableState {
  expanded: { [k: string]: boolean };
  selectedPathNode: undefined | Keys.PathNode;
}

export class PathTable extends React.Component<PathTableProps, PathTableState> {
  constructor(props: PathTableProps) {
    super(props);
    this.state = { expanded: {}, selectedPathNode: undefined };
    this.handleNavigationEvent = this.handleNavigationEvent.bind(this);
  }

  /**
   * Given a list of `indices`, toggle the first, and if we 'open' the
   * first item, open all the rest as well. This mimics vscode's file
   * explorer tree view behavior.
   */
  toggle(e: React.MouseEvent, indices: number[]) {
    this.setState(previousState => {
      if (previousState.expanded[indices[0]]) {
        return { expanded: { ...previousState.expanded, [indices[0]]: false } };
      }
      else {
        const expanded = { ...previousState.expanded };
        for (const index of indices) {
          expanded[index] = true;
        }
        return { expanded };
      }
    });
    e.stopPropagation();
    e.preventDefault();
  }

  sortClass(column: InterpretedResultsSortColumn): string {
    const sortState = this.props.resultSet.sortState;
    if (sortState !== undefined && sortState.sortBy === column) {
      return sortState.sortDirection === SortDirection.asc ? 'sort-asc' : 'sort-desc';
    }
    else {
      return 'sort-none';
    }
  }

  getNextSortState(column: InterpretedResultsSortColumn): InterpretedResultsSortState | undefined {
    const oldSortState = this.props.resultSet.sortState;
    const prevDirection = oldSortState && oldSortState.sortBy === column ? oldSortState.sortDirection : undefined;
    const nextDirection = nextSortDirection(prevDirection, true);
    return nextDirection === undefined ? undefined :
      { sortBy: column, sortDirection: nextDirection };
  }

  toggleSortStateForColumn(column: InterpretedResultsSortColumn): void {
    vscode.postMessage({
      t: 'changeInterpretedSort',
      sortState: this.getNextSortState(column),
    });
  }

  renderNoResults(): JSX.Element {
    if (this.props.nonemptyRawResults) {
      return <span>No Alerts. See <a href='#' onClick={this.props.showRawResults}>raw results</a>.</span>;
    } else {
      return <span>No Alerts</span>;
    }
  }

  render(): JSX.Element {
    const { databaseUri, resultSet } = this.props;

    const header = <thead>
      <tr>
        <th colSpan={2}></th>
        <th className={this.sortClass('alert-message') + ' vscode-codeql__alert-message-cell'} colSpan={3} onClick={() => this.toggleSortStateForColumn('alert-message')}>Message</th>
      </tr>
    </thead>;

    const rows: JSX.Element[] = [];
    const { numTruncatedResults, sourceLocationPrefix } = resultSet;

    function renderRelatedLocations(msg: string, relatedLocations: Sarif.Location[]): JSX.Element[] {
      const relatedLocationsById: { [k: string]: Sarif.Location } = {};
      for (const loc of relatedLocations) {
        relatedLocationsById[loc.id!] = loc;
      }

      // match things like `[link-text](related-location-id)`
      const parts = parseSarifPlainTextMessage(msg);

      return parts.map((part, i) => {
        if (typeof part === 'string') {
          return <span key={i}>{part}</span>;
        } else {
          const renderedLocation = renderSarifLocationWithText(part.text, relatedLocationsById[part.dest],
            undefined);
          return <span key={i}>{renderedLocation}</span>;
        }
      });
    }

    function renderNonLocation(msg: string | undefined, locationHint: string): JSX.Element | undefined {
      if (msg == undefined)
        return undefined;
      return <span title={locationHint}>{msg}</span>;
    }

    const updateSelectionCallback = (pathNodeKey: Keys.PathNode | undefined) => {
      return () => {
        this.setState(previousState => ({
          ...previousState,
          selectedPathNode: pathNodeKey
        }));
      };
    };

    function renderSarifLocationWithText(text: string | undefined, loc: Sarif.Location, pathNodeKey: Keys.PathNode | undefined): JSX.Element | undefined {
      const parsedLoc = parseSarifLocation(loc, sourceLocationPrefix);
      if ('hint' in parsedLoc) {
        return renderNonLocation(text, parsedLoc.hint);
      } else if (isWholeFileLoc(parsedLoc) || isLineColumnLoc(parsedLoc)) {
        return renderLocation(
          parsedLoc,
          text,
          databaseUri,
          undefined,
          updateSelectionCallback(pathNodeKey)
        );
      } else {
        return undefined;
      }
    }

    /**
     * Render sarif location as a link with the text being simply a
     * human-readable form of the location itself.
     */
    function renderSarifLocation(
      loc: Sarif.Location,
      pathNodeKey: Keys.PathNode | undefined
    ): JSX.Element | undefined {
      const parsedLoc = parseSarifLocation(loc, sourceLocationPrefix);
      if ('hint' in parsedLoc) {
        return renderNonLocation('[no location]', parsedLoc.hint);
      } else if (isWholeFileLoc(parsedLoc)) {
        const shortLocation = `${path.basename(parsedLoc.userVisibleFile)}`;
        const longLocation = `${parsedLoc.userVisibleFile}`;
        return renderLocation(
          parsedLoc,
          shortLocation,
          databaseUri,
          longLocation,
          updateSelectionCallback(pathNodeKey)
        );
      } else if (isLineColumnLoc(parsedLoc)) {
        const shortLocation = `${path.basename(parsedLoc.userVisibleFile)}:${parsedLoc.startLine}:${parsedLoc.startColumn}`;
        const longLocation = `${parsedLoc.userVisibleFile}`;
        return renderLocation(
          parsedLoc,
          shortLocation,
          databaseUri,
          longLocation,
          updateSelectionCallback(pathNodeKey)
        );
      } else {
        return undefined;
      }
    }

    const toggler: (indices: number[]) => (e: React.MouseEvent) => void = (indices) => {
      return (e) => this.toggle(e, indices);
    };

    if (!resultSet.sarif.runs?.[0]?.results?.length) {
      return this.renderNoResults();
    }

    let expansionIndex = 0;

    resultSet.sarif.runs[0].results.forEach((result, resultIndex) => {
      const text = result.message.text || '[no text]';
      const msg: JSX.Element[] =
        result.relatedLocations === undefined ?
          [<span key="0">{text}</span>] :
          renderRelatedLocations(text, result.relatedLocations);

      const currentResultExpanded = this.state.expanded[expansionIndex];
      const indicator = currentResultExpanded ? octicons.chevronDown : octicons.chevronRight;
      const location = result.locations !== undefined && result.locations.length > 0 &&
        renderSarifLocation(result.locations[0], Keys.none);
      const locationCells = <td className="vscode-codeql__location-cell">{location}</td>;

      if (result.codeFlows === undefined) {
        rows.push(
          <tr key={resultIndex} {...zebraStripe(resultIndex)}>
            <td className="vscode-codeql__icon-cell">{octicons.info}</td>
            <td colSpan={3}>{msg}</td>
            {locationCells}
          </tr>
        );
      }
      else {
        const paths: Sarif.ThreadFlow[] = Keys.getAllPaths(result);

        const indices = paths.length == 1 ?
          [expansionIndex, expansionIndex + 1] : /* if there's exactly one path, auto-expand
                                                  * the path when expanding the result */
          [expansionIndex];

        rows.push(
          <tr {...zebraStripe(resultIndex)} key={resultIndex}>
            <td className="vscode-codeql__icon-cell vscode-codeql__dropdown-cell" onMouseDown={toggler(indices)}>
              {indicator}
            </td>
            <td className="vscode-codeql__icon-cell">
              {octicons.listUnordered}
            </td>
            <td colSpan={2}>
              {msg}
            </td>
            {locationCells}
          </tr >
        );
        expansionIndex++;

        paths.forEach((path, pathIndex) => {
          const pathKey = { resultIndex, pathIndex };
          const currentPathExpanded = this.state.expanded[expansionIndex];
          if (currentResultExpanded) {
            const indicator = currentPathExpanded ? octicons.chevronDown : octicons.chevronRight;
            rows.push(
              <tr {...zebraStripe(resultIndex)} key={`${resultIndex}-${pathIndex}`}>
                <td className="vscode-codeql__icon-cell"><span className="vscode-codeql__vertical-rule"></span></td>
                <td className="vscode-codeql__icon-cell vscode-codeql__dropdown-cell" onMouseDown={toggler([expansionIndex])}>{indicator}</td>
                <td className="vscode-codeql__text-center" colSpan={3}>
                  Path
                </td>
              </tr>
            );
          }
          expansionIndex++;

          if (currentResultExpanded && currentPathExpanded) {
            const pathNodes = path.locations;
            for (let pathNodeIndex = 0; pathNodeIndex < pathNodes.length; ++pathNodeIndex) {
              const pathNodeKey: Keys.PathNode = { ...pathKey, pathNodeIndex };
              const step = pathNodes[pathNodeIndex];
              const msg = step.location !== undefined && step.location.message !== undefined ?
                renderSarifLocationWithText(step.location.message.text, step.location, pathNodeKey) :
                '[no location]';
              const additionalMsg = step.location !== undefined ?
                renderSarifLocation(step.location, pathNodeKey) :
                '';
              const isSelected = Keys.equalsNotUndefined(this.state.selectedPathNode, pathNodeKey);
              const stepIndex = pathNodeIndex + 1; // Convert to 1-based
              const zebraIndex = resultIndex + stepIndex;
              rows.push(
                <tr className={isSelected ? 'vscode-codeql__selected-path-node' : undefined} key={`${resultIndex}-${pathIndex}-${pathNodeIndex}`}>
                  <td className="vscode-codeql__icon-cell"><span className="vscode-codeql__vertical-rule"></span></td>
                  <td className="vscode-codeql__icon-cell"><span className="vscode-codeql__vertical-rule"></span></td>
                  <td {...selectableZebraStripe(isSelected, zebraIndex, 'vscode-codeql__path-index-cell')}>{stepIndex}</td>
                  <td {...selectableZebraStripe(isSelected, zebraIndex)}>{msg} </td>
                  <td {...selectableZebraStripe(isSelected, zebraIndex, 'vscode-codeql__location-cell')}>{additionalMsg}</td>
                </tr>);
            }
          }
        });

      }
    });

    if (numTruncatedResults > 0) {
      rows.push(
        <tr key="truncatd-message">
          <td colSpan={5} style={{ textAlign: 'center', fontStyle: 'italic' }}>
            Too many results to show at once. {numTruncatedResults} result(s) omitted.
          </td>
        </tr>
      );
    }

    return <table className={className}>
      {header}
      <tbody>{rows}</tbody>
    </table>;
  }

  private handleNavigationEvent(event: NavigationEvent) {
    this.setState(prevState => {
      const { selectedPathNode } = prevState;
      if (selectedPathNode === undefined) return prevState;

      const path = Keys.getPath(this.props.resultSet.sarif, selectedPathNode);
      if (path === undefined) return prevState;

      const nextIndex = selectedPathNode.pathNodeIndex + event.direction;
      if (nextIndex < 0 || nextIndex >= path.locations.length) return prevState;

      const sarifLoc = path.locations[nextIndex].location;
      if (sarifLoc === undefined) {
        return prevState;
      }

      const loc = parseSarifLocation(sarifLoc, this.props.resultSet.sourceLocationPrefix);
      if (isNoLocation(loc)) {
        return prevState;
      }

      jumpToLocation(loc, this.props.databaseUri);
      const newSelection = { ...selectedPathNode, pathNodeIndex: nextIndex };
      return { ...prevState, selectedPathNode: newSelection };
    });
  }

  componentDidMount() {
    onNavigation.addListener(this.handleNavigationEvent);
  }

  componentWillUnmount() {
    onNavigation.removeListener(this.handleNavigationEvent);
  }
}
