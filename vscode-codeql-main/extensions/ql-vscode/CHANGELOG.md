# CodeQL for Visual Studio Code: Changelog

## [UNRELEASED]

## 1.3.5 - 27 October 2020

- Fix a bug where archived source folders for databases were not showing any contents.

## 1.3.4 - 22 October 2020

- Add friendly welcome message when the databases view is empty.
- Add open query, open results, and remove query commands in the query history view title bar.
- The maximum number of simultaneous queries launchable by the `CodeQL: Run Queries in Selected Files` command is now configurable by changing the `codeQL.runningQueries.maxQueries` setting.
- Allow simultaneously run queries to be canceled in a single-click.
- Prevent multiple upgrade dialogs from appearing when running simultaneous queries on upgradeable databases.
- Fix sorting of results. Some pages of results would have the wrong sort order and columns.
- Remember previous sort order when reloading query results.
- Fix proper escaping of backslashes in SARIF message strings.
- Allow setting `codeQL.runningQueries.numberOfThreads` and `codeQL.runningTests.numberOfThreads` to 0, (which is interpreted as 'use one thread per core on the machine').
- Clear the problems view of all CodeQL query results when a database is removed.
- Add a `View DIL` command on query history items. This opens a text editor containing the Datalog Intermediary Language representation of the compiled query.
- Remove feature flag for the AST Viewer. For more information on how to use the AST Viewer, [see the documentation](https://help.semmle.com/codeql/codeql-for-vscode/procedures/exploring-the-structure-of-your-source-code.html).
- The `codeQL.runningTests.numberOfThreads` setting is now used correctly when running tests.

## 1.3.3 - 16 September 2020

- Fix display of raw results entities with label but no url.
- Fix bug where sort order is forgotten when changing raw results page.
- Avoid showing a location link in results view when a result item has an empty location.

## 1.3.2 - 12 August 2020

- Fix error with choosing qlpack search path.
- Fix pagination when there are no results.
- Suppress database downloaded from URL message when action canceled.
- Fix QL test discovery to avoid showing duplicate tests in the test explorer.
- Enable pagination of query results
- Add experimental AST Viewer for Go and C++. To enable, add `"codeQL.experimentalAstViewer": true` to the user settings file.

## 1.3.1 - 7 July 2020

- Fix unzipping of large files.
- Ensure compare order is consistent when selecting two queries to compare. The first query selected is always the _from_ query and the query selected later is always the _to_ query.
- Ensure added databases have zipped source locations for databases added as archives or downloaded from the internet.
- Fix bug where it is not possible to add databases starting with `db-*`.
- Change styling of pagination section of the results page.
- Fix display of query text for stored quick queries.

## 1.3.0 - 22 June 2020

- Report error when selecting invalid database.
- Add descriptive message for database archive import failure.
- Respect VS Code's i18n locale setting when formatting dates and sorting strings.
- Allow the opening of large SARIF files externally from VS Code.
- Add new 'CodeQL: Compare Query' command that shows the differences between two queries.
- Allow multiple items in the query history view to be removed in one operation.
- Allow multiple items in the databases view to be removed in one operation.
- Allow multiple items in the databases view to be upgraded in one operation.
- Allow multiple items in the databases view to have their external folders opened.
- Allow all selected queries to be run in one command from the file explorer.

## 1.2.2 - 8 June 2020

- Fix auto-indentation rules.
- Add ability to download platform-specific releases of the CodeQL CLI if they are available.
- Fix handling of downloading prerelease versions of the CodeQL CLI.
- Add pagination for displaying non-interpreted results.

## 1.2.1 - 29 May 2020

- Better formatting and autoindentation when adding QLDoc comments to `.ql` and `.qll` files.
- Allow for more flexibility when opening a database in the workspace. A user can now choose the actual database folder, or the nested `db-*` folder.
- Add query history menu command for viewing corresponding SARIF file.
- Add ability for users to download databases directly from LGTM.com.

## 1.2.0 - 19 May 2020

- Enable 'Go to Definition' and 'Go to References' on source archive
  files in CodeQL databases. This is handled by a CodeQL query.
- Fix adding database archive files on Windows.
- Enable adding remote and local database archive files from the
  command palette.

## 1.1.5 - 15 May 2020

- Links in results are no longer underlined and monospaced.
- Add the ability to choose a database either from an archive, a folder, or from the internet.
- New icons for commands on the databases view.

## 1.1.4 - 13 May 2020

- Add the ability to download and install databases archives from the internet.

## 1.1.3 - 8 May 2020

- Add a suggestion in alerts view to view raw results, when there are
  raw results but no alerts.
- Add the ability to rename databases in the database view.
- Add the ability to open the directory in the filesystem
  of a database.

## 1.1.2 - 28 April 2020

- Implement syntax highlighting for the new `unique` aggregate.
- Implement XML syntax highlighting for `.qhelp` files.
- Add option to auto save queries before running them.
- Add new command in query history to view the query text of the
  selected query (note that this may be different from the current
  contents of the query file if the file has been edited).
- Add ability to sort CodeQL databases by name or by date added.

## 1.1.1 - 23 March 2020

- Fix quick evaluation in `.qll` files.
- Add new command in query history view to view the log file of a
  query.
- Request user acknowledgment before updating the CodeQL binaries.
- Warn when using the deprecated `codeql.cmd` launcher on Windows.

## 1.1.0 - 17 March 2020

- Add functionality for testing custom CodeQL queries by using the VS
  Code Test Explorer extension and `codeql test`. See the documentation for
  more details.
- Add a "Show log" button to all information, error, and warning
  popups that will display the CodeQL extension log.
- Display a message when a query times out.
- Show canceled queries in query history.
- Improve error messages when attempting to run non-query files.

## 1.0.6 - 28 February 2020

- Add command to restart query server.
- Enable support for future minor upgrades to the CodeQL CLI.

## 1.0.5 - 13 February 2020

- Add an icon next to any failed query runs in the query history
  view.
- Add the ability to sort alerts by alert message.

## 1.0.4 - 24 January 2020

- Disable word-based autocomplete by default.
- Add command `CodeQL: Quick Query` for easy query creation without
  having to choose a place in the filesystem to store the query file.

## 1.0.3 - 13 January 2020

- Reduce the frequency of CodeQL CLI update checks to help avoid hitting GitHub API limits of 60 requests per
  hour for unauthenticated IPs.
- Fix sorting of result sets with names containing special characters.

## 1.0.2 - 13 December 2019

- Fix rendering of negative numbers in results.
- Allow customization of query history labels from settings and from
  query history view context menu.
- Show number of results in results view.
- Add commands `CodeQL: Show Next Step on Path` and `CodeQL: Show Previous Step on Path` for navigating the steps on the currently
  shown path result.

## 1.0.1 - 21 November 2019

- Change `codeQL.cli.executablePath` to a per-machine setting, so it can no longer be set at the user or workspace level. This helps prevent arbitrary code execution when using a VS Code workspace from an untrusted source.
- Improve the highlighting of the selected query result within the source code.
- Improve the performance of switching between result tables in the CodeQL Query Results view.
- Fix the automatic upgrading of CodeQL databases when using upgrade scripts from the workspace.
- Allow removal of items from the CodeQL Query History view.

## 1.0.0 - 14 November 2019

Initial release of CodeQL for Visual Studio Code.
