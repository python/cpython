import {
  CancellationToken,
  commands,
  Disposable,
  ExtensionContext,
  extensions,
  languages,
  ProgressLocation,
  ProgressOptions,
  Uri,
  window as Window,
  env,
  window
} from 'vscode';
import { LanguageClient } from 'vscode-languageclient';
import * as path from 'path';
import { testExplorerExtensionId, TestHub } from 'vscode-test-adapter-api';

import { AstViewer } from './astViewer';
import * as archiveFilesystemProvider from './archive-filesystem-provider';
import { CodeQLCliServer } from './cli';
import {
  CliConfigListener,
  DistributionConfigListener,
  MAX_QUERIES,
  QueryHistoryConfigListener,
  QueryServerConfigListener
} from './config';
import * as languageSupport from './languageSupport';
import { DatabaseManager } from './databases';
import { DatabaseUI } from './databases-ui';
import {
  TemplateQueryDefinitionProvider,
  TemplateQueryReferenceProvider,
  TemplatePrintAstProvider
} from './contextual/templateProvider';
import {
  DEFAULT_DISTRIBUTION_VERSION_RANGE,
  DistributionKind,
  DistributionManager,
  DistributionUpdateCheckResultKind,
  FindDistributionResult,
  FindDistributionResultKind,
  GithubApiError,
  GithubRateLimitedError
} from './distribution';
import * as helpers from './helpers';
import { assertNever } from './helpers-pure';
import { spawnIdeServer } from './ide-server';
import { InterfaceManager } from './interface';
import { WebviewReveal } from './interface-utils';
import { ideServerLogger, logger, queryServerLogger } from './logging';
import { QueryHistoryManager } from './query-history';
import { CompletedQuery } from './query-results';
import * as qsClient from './queryserver-client';
import { displayQuickQuery } from './quick-query';
import { compileAndRunQueryAgainstDatabase, tmpDirDisposal } from './run-queries';
import { QLTestAdapterFactory } from './test-adapter';
import { TestUIService } from './test-ui';
import { CompareInterfaceManager } from './compare/compare-interface';
import { gatherQlFiles } from './files';

/**
 * extension.ts
 * ------------
 *
 * A vscode extension for CodeQL query development.
 */

/**
 * Holds when we have proceeded past the initial phase of extension activation in which
 * we are trying to ensure that a valid CodeQL distribution exists, and we're actually setting
 * up the bulk of the extension.
 */
let beganMainExtensionActivation = false;

/**
 * A list of vscode-registered-command disposables that contain
 * temporary stub handlers for commands that exist package.json (hence
 * are already connected to onscreen ui elements) but which will not
 * have any useful effect if we haven't located a CodeQL distribution.
 */
const errorStubs: Disposable[] = [];

/**
 * Holds when we are installing or checking for updates to the distribution.
 */
let isInstallingOrUpdatingDistribution = false;

/**
 * If the user tries to execute vscode commands after extension activation is failed, give
 * a sensible error message.
 *
 * @param excludedCommands List of commands for which we should not register error stubs.
 */
function registerErrorStubs(excludedCommands: string[], stubGenerator: (command: string) => () => Promise<void>): void {
  // Remove existing stubs
  errorStubs.forEach(stub => stub.dispose());

  const extensionId = 'GitHub.vscode-codeql'; // TODO: Is there a better way of obtaining this?
  const extension = extensions.getExtension(extensionId);
  if (extension === undefined) {
    throw new Error(`Can't find extension ${extensionId}`);
  }

  const stubbedCommands: string[]
    = extension.packageJSON.contributes.commands.map((entry: { command: string }) => entry.command);

  stubbedCommands.forEach(command => {
    if (excludedCommands.indexOf(command) === -1) {
      errorStubs.push(helpers.commandRunner(command, stubGenerator(command)));
    }
  });
}

export async function activate(ctx: ExtensionContext): Promise<void> {
  logger.log('Starting CodeQL extension');

  initializeLogging(ctx);
  languageSupport.install();

  const distributionConfigListener = new DistributionConfigListener();
  ctx.subscriptions.push(distributionConfigListener);
  const codeQlVersionRange = DEFAULT_DISTRIBUTION_VERSION_RANGE;
  const distributionManager = new DistributionManager(ctx, distributionConfigListener, codeQlVersionRange);

  const shouldUpdateOnNextActivationKey = 'shouldUpdateOnNextActivation';

  registerErrorStubs([checkForUpdatesCommand], command => (async () => {
    helpers.showAndLogErrorMessage(`Can't execute ${command}: waiting to finish loading CodeQL CLI.`);
  }));

  interface DistributionUpdateConfig {
    isUserInitiated: boolean;
    shouldDisplayMessageWhenNoUpdates: boolean;
    allowAutoUpdating: boolean;
  }

  async function installOrUpdateDistributionWithProgressTitle(progressTitle: string, config: DistributionUpdateConfig): Promise<void> {
    const minSecondsSinceLastUpdateCheck = config.isUserInitiated ? 0 : 86400;
    const noUpdatesLoggingFunc = config.shouldDisplayMessageWhenNoUpdates ?
      helpers.showAndLogInformationMessage : async (message: string) => logger.log(message);
    const result = await distributionManager.checkForUpdatesToExtensionManagedDistribution(minSecondsSinceLastUpdateCheck);

    // We do want to auto update if there is no distribution at all
    const allowAutoUpdating = config.allowAutoUpdating || !await distributionManager.hasDistribution();

    switch (result.kind) {
      case DistributionUpdateCheckResultKind.AlreadyCheckedRecentlyResult:
        logger.log('Didn\'t perform CodeQL CLI update check since a check was already performed within the previous ' +
          `${minSecondsSinceLastUpdateCheck} seconds.`);
        break;
      case DistributionUpdateCheckResultKind.AlreadyUpToDate:
        await noUpdatesLoggingFunc('CodeQL CLI already up to date.');
        break;
      case DistributionUpdateCheckResultKind.InvalidLocation:
        await noUpdatesLoggingFunc('CodeQL CLI is installed externally so could not be updated.');
        break;
      case DistributionUpdateCheckResultKind.UpdateAvailable:
        if (beganMainExtensionActivation || !allowAutoUpdating) {
          const updateAvailableMessage = `Version "${result.updatedRelease.name}" of the CodeQL CLI is now available. ` +
            'Do you wish to upgrade?';
          await ctx.globalState.update(shouldUpdateOnNextActivationKey, true);
          if (await helpers.showInformationMessageWithAction(updateAvailableMessage, 'Restart and Upgrade')) {
            await commands.executeCommand('workbench.action.reloadWindow');
          }
        } else {
          const progressOptions: ProgressOptions = {
            title: progressTitle,
            location: ProgressLocation.Notification,
          };

          await helpers.withProgress(progressOptions, progress =>
            distributionManager.installExtensionManagedDistributionRelease(result.updatedRelease, progress));

          await ctx.globalState.update(shouldUpdateOnNextActivationKey, false);
          helpers.showAndLogInformationMessage(`CodeQL CLI updated to version "${result.updatedRelease.name}".`);
        }
        break;
      default:
        assertNever(result);
    }
  }

  async function installOrUpdateDistribution(config: DistributionUpdateConfig): Promise<void> {
    if (isInstallingOrUpdatingDistribution) {
      throw new Error('Already installing or updating CodeQL CLI');
    }
    isInstallingOrUpdatingDistribution = true;
    const codeQlInstalled = await distributionManager.getCodeQlPathWithoutVersionCheck() !== undefined;
    const willUpdateCodeQl = ctx.globalState.get(shouldUpdateOnNextActivationKey);
    const messageText = willUpdateCodeQl
      ? 'Updating CodeQL CLI'
      : codeQlInstalled
        ? 'Checking for updates to CodeQL CLI'
        : 'Installing CodeQL CLI';

    try {
      await installOrUpdateDistributionWithProgressTitle(messageText, config);
    } catch (e) {
      // Don't rethrow the exception, because if the config is changed, we want to be able to retry installing
      // or updating the distribution.
      const alertFunction = (codeQlInstalled && !config.isUserInitiated) ?
        helpers.showAndLogWarningMessage : helpers.showAndLogErrorMessage;
      const taskDescription = (willUpdateCodeQl ? 'update' :
        codeQlInstalled ? 'check for updates to' : 'install') + ' CodeQL CLI';

      if (e instanceof GithubRateLimitedError) {
        alertFunction(`Rate limited while trying to ${taskDescription}. Please try again after ` +
          `your rate limit window resets at ${e.rateLimitResetDate.toLocaleString(env.language)}.`);
      } else if (e instanceof GithubApiError) {
        alertFunction(`Encountered GitHub API error while trying to ${taskDescription}. ` + e);
      }
      alertFunction(`Unable to ${taskDescription}. ` + e);
    } finally {
      isInstallingOrUpdatingDistribution = false;
    }
  }

  async function getDistributionDisplayingDistributionWarnings(): Promise<FindDistributionResult> {
    const result = await distributionManager.getDistribution();
    switch (result.kind) {
      case FindDistributionResultKind.CompatibleDistribution:
        logger.log(`Found compatible version of CodeQL CLI (version ${result.version.raw})`);
        break;
      case FindDistributionResultKind.IncompatibleDistribution: {
        const fixGuidanceMessage = (() => {
          switch (result.distribution.kind) {
            case DistributionKind.ExtensionManaged:
              return 'Please update the CodeQL CLI by running the "CodeQL: Check for CLI Updates" command.';
            case DistributionKind.CustomPathConfig:
              return `Please update the \"CodeQL CLI Executable Path\" setting to point to a CLI in the version range ${codeQlVersionRange}.`;
            case DistributionKind.PathEnvironmentVariable:
              return `Please update the CodeQL CLI on your PATH to a version compatible with ${codeQlVersionRange}, or ` +
                `set the \"CodeQL CLI Executable Path\" setting to the path of a CLI version compatible with ${codeQlVersionRange}.`;
          }
        })();

        helpers.showAndLogWarningMessage(`The current version of the CodeQL CLI (${result.version.raw}) ` +
          'is incompatible with this extension. ' + fixGuidanceMessage);
        break;
      }
      case FindDistributionResultKind.UnknownCompatibilityDistribution:
        helpers.showAndLogWarningMessage('Compatibility with the configured CodeQL CLI could not be determined. ' +
          'You may experience problems using the extension.');
        break;
      case FindDistributionResultKind.NoDistribution:
        helpers.showAndLogErrorMessage('The CodeQL CLI could not be found.');
        break;
      default:
        assertNever(result);
    }
    return result;
  }

  async function installOrUpdateThenTryActivate(config: DistributionUpdateConfig): Promise<void> {
    await installOrUpdateDistribution(config);

    // Display the warnings even if the extension has already activated.
    const distributionResult = await getDistributionDisplayingDistributionWarnings();

    if (!beganMainExtensionActivation && distributionResult.kind !== FindDistributionResultKind.NoDistribution) {
      await activateWithInstalledDistribution(ctx, distributionManager);
    } else if (distributionResult.kind === FindDistributionResultKind.NoDistribution) {
      registerErrorStubs([checkForUpdatesCommand], command => async () => {
        const installActionName = 'Install CodeQL CLI';
        const chosenAction = await helpers.showAndLogErrorMessage(`Can't execute ${command}: missing CodeQL CLI.`, {
          items: [installActionName]
        });
        if (chosenAction === installActionName) {
          installOrUpdateThenTryActivate({
            isUserInitiated: true,
            shouldDisplayMessageWhenNoUpdates: false,
            allowAutoUpdating: true
          });
        }
      });
    }
  }

  ctx.subscriptions.push(distributionConfigListener.onDidChangeConfiguration(() => installOrUpdateThenTryActivate({
    isUserInitiated: true,
    shouldDisplayMessageWhenNoUpdates: false,
    allowAutoUpdating: true
  })));
  ctx.subscriptions.push(helpers.commandRunner(checkForUpdatesCommand, () => installOrUpdateThenTryActivate({
    isUserInitiated: true,
    shouldDisplayMessageWhenNoUpdates: true,
    allowAutoUpdating: true
  })));

  await installOrUpdateThenTryActivate({
    isUserInitiated: !!ctx.globalState.get(shouldUpdateOnNextActivationKey),
    shouldDisplayMessageWhenNoUpdates: false,

    // only auto update on startup if the user has previously requested an update
    // otherwise, ask user to accept the update
    allowAutoUpdating: !!ctx.globalState.get(shouldUpdateOnNextActivationKey)
  });
}

async function activateWithInstalledDistribution(
  ctx: ExtensionContext,
  distributionManager: DistributionManager
): Promise<void> {
  beganMainExtensionActivation = true;
  // Remove any error stubs command handlers left over from first part
  // of activation.
  errorStubs.forEach((stub) => stub.dispose());

  logger.log('Initializing configuration listener...');
  const qlConfigurationListener = await QueryServerConfigListener.createQueryServerConfigListener(
    distributionManager
  );
  ctx.subscriptions.push(qlConfigurationListener);

  logger.log('Initializing CodeQL cli server...');
  const cliServer = new CodeQLCliServer(
    distributionManager,
    new CliConfigListener(),
    logger
  );
  ctx.subscriptions.push(cliServer);

  logger.log('Initializing query server client.');
  const qs = new qsClient.QueryServerClient(
    qlConfigurationListener,
    cliServer,
    {
      logger: queryServerLogger,
    },
    (task) =>
      Window.withProgress(
        { title: 'CodeQL query server', location: ProgressLocation.Window },
        task
      )
  );
  ctx.subscriptions.push(qs);
  await qs.startQueryServer();

  logger.log('Initializing database manager.');
  const dbm = new DatabaseManager(ctx, qlConfigurationListener, logger);
  ctx.subscriptions.push(dbm);
  logger.log('Initializing database panel.');
  const databaseUI = new DatabaseUI(
    cliServer,
    dbm,
    qs,
    getContextStoragePath(ctx),
    ctx.extensionPath
  );
  ctx.subscriptions.push(databaseUI);

  logger.log('Initializing query history manager.');
  const queryHistoryConfigurationListener = new QueryHistoryConfigListener();
  const showResults = async (item: CompletedQuery) =>
    showResultsForCompletedQuery(item, WebviewReveal.Forced);

  const qhm = new QueryHistoryManager(
    qs,
    ctx.extensionPath,
    queryHistoryConfigurationListener,
    showResults,
    async (from: CompletedQuery, to: CompletedQuery) =>
      showResultsForComparison(from, to),
  );
  ctx.subscriptions.push(qhm);
  logger.log('Initializing results panel interface.');
  const intm = new InterfaceManager(ctx, dbm, cliServer, queryServerLogger);
  ctx.subscriptions.push(intm);

  logger.log('Initializing compare panel interface.');
  const cmpm = new CompareInterfaceManager(
    ctx,
    dbm,
    cliServer,
    queryServerLogger,
    showResults
  );
  ctx.subscriptions.push(cmpm);

  logger.log('Initializing source archive filesystem provider.');
  archiveFilesystemProvider.activate(ctx);

  async function showResultsForComparison(
    from: CompletedQuery,
    to: CompletedQuery
  ): Promise<void> {
    try {
      await cmpm.showResults(from, to);
    } catch (e) {
      helpers.showAndLogErrorMessage(e.message);
    }
  }

  async function showResultsForCompletedQuery(
    query: CompletedQuery,
    forceReveal: WebviewReveal
  ): Promise<void> {
    await intm.showResults(query, forceReveal, false);
  }

  async function compileAndRunQuery(
    quickEval: boolean,
    selectedQuery: Uri | undefined,
    progress: helpers.ProgressCallback,
    token: CancellationToken,
  ): Promise<void> {
    if (qs !== undefined) {
      const dbItem = await databaseUI.getDatabaseItem(progress, token);
      if (dbItem === undefined) {
        throw new Error('Can\'t run query without a selected database');
      }
      const info = await compileAndRunQueryAgainstDatabase(
        cliServer,
        qs,
        dbItem,
        quickEval,
        selectedQuery,
        progress,
        token
      );
      const item = qhm.addQuery(info);
      await showResultsForCompletedQuery(item, WebviewReveal.NotForced);
      // The call to showResults potentially creates SARIF file;
      // Update the tree item context value to allow viewing that
      // SARIF file from context menu.
      await qhm.updateTreeItemContextValue(item);
    }
  }

  ctx.subscriptions.push(tmpDirDisposal);

  logger.log('Initializing CodeQL language server.');
  const client = new LanguageClient(
    'CodeQL Language Server',
    () => spawnIdeServer(qlConfigurationListener),
    {
      documentSelector: [
        { language: 'ql', scheme: 'file' },
        { language: 'yaml', scheme: 'file', pattern: '**/qlpack.yml' },
      ],
      synchronize: {
        configurationSection: 'codeQL',
      },
      // Ensure that language server exceptions are logged to the same channel as its output.
      outputChannel: ideServerLogger.outputChannel,
    },
    true
  );

  logger.log('Initializing QLTest interface.');
  const testExplorerExtension = extensions.getExtension<TestHub>(
    testExplorerExtensionId
  );
  if (testExplorerExtension) {
    const testHub = testExplorerExtension.exports;
    const testAdapterFactory = new QLTestAdapterFactory(testHub, cliServer);
    ctx.subscriptions.push(testAdapterFactory);

    const testUIService = new TestUIService(testHub);
    ctx.subscriptions.push(testUIService);
  }

  logger.log('Registering top-level command palette commands.');
  ctx.subscriptions.push(
    helpers.commandRunnerWithProgress(
      'codeQL.runQuery',
      async (
        progress: helpers.ProgressCallback,
        token: CancellationToken,
        uri: Uri | undefined
      ) => await compileAndRunQuery(false, uri, progress, token),
      {
        title: 'Running query',
        cancellable: true
      }
    )
  );
  ctx.subscriptions.push(
    helpers.commandRunnerWithProgress(
      'codeQL.runQueries',
      async (
        progress: helpers.ProgressCallback,
        token: CancellationToken,
        _: Uri | undefined,
        multi: Uri[]
      ) => {
        const maxQueryCount = MAX_QUERIES.getValue() as number;
        const [files, dirFound] = await gatherQlFiles(multi.map(uri => uri.fsPath));
        if (files.length > maxQueryCount) {
          throw new Error(`You tried to run ${files.length} queries, but the maximum is ${maxQueryCount}. Try selecting fewer queries or changing the 'codeQL.runningQueries.maxQueries' setting.`);
        }
        // warn user and display selected files when a directory is selected because some ql
        // files may be hidden from the user.
        if (dirFound) {
          const fileString = files.map(file => path.basename(file)).join(', ');
          const res = await helpers.showBinaryChoiceDialog(
            `You are about to run ${files.length} queries: ${fileString} Do you want to continue?`
          );
          if (!res) {
            return;
          }
        }
        const queryUris = files.map(path => Uri.parse(`file:${path}`, true));

        // Use a wrapped progress so that messages appear with the queries remaining in it.
        let queriesRemaining = queryUris.length;
        function wrappedProgress(update: helpers.ProgressUpdate) {
          const message = queriesRemaining > 1
            ? `${queriesRemaining} remaining. ${update.message}`
            : update.message;
          progress({
            ...update,
            message
          });
        }

        if (queryUris.length > 1) {
          // Try to upgrade the current database before running any queries
          // so that the user isn't confronted with multiple upgrade
          // requests for each query to run.
          // Only do it if running multiple queries since this check is
          // performed on each query run anyway.
          await databaseUI.tryUpgradeCurrentDatabase(progress, token);
        }

        wrappedProgress({
          maxStep: queryUris.length,
          step: queryUris.length - queriesRemaining,
          message: ''
        });

        await Promise.all(queryUris.map(async uri =>
          compileAndRunQuery(false, uri, wrappedProgress, token)
            .then(() => queriesRemaining--)
        ));
      },
      {
        title: 'Running queries',
        cancellable: true
      })
  );
  ctx.subscriptions.push(
    helpers.commandRunnerWithProgress(
      'codeQL.quickEval',
      async (
        progress: helpers.ProgressCallback,
        token: CancellationToken,
        uri: Uri | undefined
      ) => await compileAndRunQuery(true, uri, progress, token),
      {
        title: 'Running query',
        cancellable: true
      })
  );
  ctx.subscriptions.push(
    helpers.commandRunnerWithProgress('codeQL.quickQuery', async (
      progress: helpers.ProgressCallback,
      token: CancellationToken
    ) =>
      displayQuickQuery(ctx, cliServer, databaseUI, progress, token),
      {
        title: 'Run Quick Query'
      }
    )
  );

  ctx.subscriptions.push(
    helpers.commandRunner('codeQL.restartQueryServer', async () => {
      await qs.restartQueryServer();
      helpers.showAndLogInformationMessage('CodeQL Query Server restarted.', {
        outputLogger: queryServerLogger,
      });
    })
  );
  ctx.subscriptions.push(
    helpers.commandRunner('codeQL.chooseDatabaseFolder', (
      progress: helpers.ProgressCallback,
      token: CancellationToken
    ) =>
      databaseUI.handleChooseDatabaseFolder(progress, token)
    )
  );
  ctx.subscriptions.push(
    helpers.commandRunner('codeQL.chooseDatabaseArchive', (
      progress: helpers.ProgressCallback,
      token: CancellationToken
    ) =>
      databaseUI.handleChooseDatabaseArchive(progress, token)
    )
  );
  ctx.subscriptions.push(
    helpers.commandRunnerWithProgress('codeQL.chooseDatabaseLgtm', (
      progress: helpers.ProgressCallback,
      token: CancellationToken
    ) =>
      databaseUI.handleChooseDatabaseLgtm(progress, token),
      {
        title: 'Adding database from LGTM',
      })
  );
  ctx.subscriptions.push(
    helpers.commandRunnerWithProgress('codeQL.chooseDatabaseInternet', (
      progress: helpers.ProgressCallback,
      token: CancellationToken
    ) =>
      databaseUI.handleChooseDatabaseInternet(progress, token),

      {
        title: 'Adding database from URL',
      })
  );

  logger.log('Starting language server.');
  ctx.subscriptions.push(client.start());

  // Jump-to-definition and find-references
  logger.log('Registering jump-to-definition handlers.');
  languages.registerDefinitionProvider(
    { scheme: archiveFilesystemProvider.zipArchiveScheme },
    new TemplateQueryDefinitionProvider(cliServer, qs, dbm)
  );

  languages.registerReferenceProvider(
    { scheme: archiveFilesystemProvider.zipArchiveScheme },
    new TemplateQueryReferenceProvider(cliServer, qs, dbm)
  );

  const astViewer = new AstViewer();
  ctx.subscriptions.push(astViewer);
  ctx.subscriptions.push(helpers.commandRunnerWithProgress('codeQL.viewAst', async (
    progress: helpers.ProgressCallback,
    token: CancellationToken
  ) => {
    const ast = await new TemplatePrintAstProvider(cliServer, qs, dbm, progress, token)
      .provideAst(window.activeTextEditor?.document);
    if (ast) {
      astViewer.updateRoots(await ast.getRoots(), ast.db, ast.fileName);
    }
  }, {
    cancellable: true,
    title: 'Calculate AST'
  }));

  logger.log('Successfully finished extension initialization.');
}

function getContextStoragePath(ctx: ExtensionContext) {
  return ctx.storagePath || ctx.globalStoragePath;
}

function initializeLogging(ctx: ExtensionContext): void {
  const storagePath = getContextStoragePath(ctx);
  logger.init(storagePath);
  queryServerLogger.init(storagePath);
  ideServerLogger.init(storagePath);
  ctx.subscriptions.push(logger);
  ctx.subscriptions.push(queryServerLogger);
  ctx.subscriptions.push(ideServerLogger);
}

const checkForUpdatesCommand = 'codeQL.checkForUpdatesToCLI';
