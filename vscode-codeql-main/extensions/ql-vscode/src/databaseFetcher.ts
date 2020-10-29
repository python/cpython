import fetch, { Response } from 'node-fetch';
import * as unzipper from 'unzipper';
import { zip } from 'zip-a-folder';
import {
  Uri,
  CancellationToken,
  commands,
  window,
} from 'vscode';
import * as fs from 'fs-extra';
import * as path from 'path';

import { DatabaseManager, DatabaseItem } from './databases';
import {
  ProgressCallback,
  showAndLogInformationMessage,
} from './helpers';
import { logger } from './logging';

/**
 * Prompts a user to fetch a database from a remote location. Database is assumed to be an archive file.
 *
 * @param databasesManager the DatabaseManager
 * @param storagePath where to store the unzipped database.
 */
export async function promptImportInternetDatabase(
  databasesManager: DatabaseManager,
  storagePath: string,
  progress: ProgressCallback,
  _: CancellationToken,
): Promise<DatabaseItem | undefined> {
  const databaseUrl = await window.showInputBox({
    prompt: 'Enter URL of zipfile of database to download',
  });
  if (!databaseUrl) {
    return;
  }

  validateHttpsUrl(databaseUrl);

  const item = await databaseArchiveFetcher(
    databaseUrl,
    databasesManager,
    storagePath,
    progress
  );

  if (item) {
    commands.executeCommand('codeQLDatabases.focus');
    showAndLogInformationMessage('Database downloaded and imported successfully.');
  }
  return item;

}

/**
 * Prompts a user to fetch a database from lgtm.
 * User enters a project url and then the user is asked which language
 * to download (if there is more than one)
 *
 * @param databasesManager the DatabaseManager
 * @param storagePath where to store the unzipped database.
 */
export async function promptImportLgtmDatabase(
  databasesManager: DatabaseManager,
  storagePath: string,
  progress: ProgressCallback,
  _: CancellationToken
): Promise<DatabaseItem | undefined> {
  const lgtmUrl = await window.showInputBox({
    prompt:
      'Enter the project URL on LGTM (e.g., https://lgtm.com/projects/g/github/codeql)',
  });
  if (!lgtmUrl) {
    return;
  }

  if (looksLikeLgtmUrl(lgtmUrl)) {
    const databaseUrl = await convertToDatabaseUrl(lgtmUrl);
    if (databaseUrl) {
      const item = await databaseArchiveFetcher(
        databaseUrl,
        databasesManager,
        storagePath,
        progress
      );
      if (item) {
        commands.executeCommand('codeQLDatabases.focus');
        showAndLogInformationMessage('Database downloaded and imported successfully.');
      }
      return item;
    }
  } else {
    throw new Error(`Invalid LGTM URL: ${lgtmUrl}`);
  }
  return;
}

/**
 * Imports a database from a local archive.
 *
 * @param databaseUrl the file url of the archive to import
 * @param databasesManager the DatabaseManager
 * @param storagePath where to store the unzipped database.
 */
export async function importArchiveDatabase(
  databaseUrl: string,
  databasesManager: DatabaseManager,
  storagePath: string,
  progress: ProgressCallback,
  _: CancellationToken,
): Promise<DatabaseItem | undefined> {
  try {
    const item = await databaseArchiveFetcher(
      databaseUrl,
      databasesManager,
      storagePath,
      progress
    );
    if (item) {
      commands.executeCommand('codeQLDatabases.focus');
      showAndLogInformationMessage('Database unzipped and imported successfully.');
    }
    return item;
  } catch (e) {
    if (e.message.includes('unexpected end of file')) {
      throw new Error('Database is corrupt or too large. Try unzipping outside of VS Code and importing the unzipped folder instead.');
    } else {
      // delegate
      throw e;
    }
  }
}

/**
 * Fetches an archive database. The database might be on the internet
 * or in the local filesystem.
 *
 * @param databaseUrl URL from which to grab the database
 * @param databasesManager the DatabaseManager
 * @param storagePath where to store the unzipped database.
 * @param progressCallback optional callback to send progress messages to
 */
async function databaseArchiveFetcher(
  databaseUrl: string,
  databasesManager: DatabaseManager,
  storagePath: string,
  progressCallback?: ProgressCallback
): Promise<DatabaseItem> {
  progressCallback?.({
    message: 'Getting database',
    step: 1,
    maxStep: 4,
  });
  if (!storagePath) {
    throw new Error('No storage path specified.');
  }
  await fs.ensureDir(storagePath);
  const unzipPath = await getStorageFolder(storagePath, databaseUrl);

  if (isFile(databaseUrl)) {
    await readAndUnzip(databaseUrl, unzipPath);
  } else {
    await fetchAndUnzip(databaseUrl, unzipPath, progressCallback);
  }

  progressCallback?.({
    message: 'Opening database',
    step: 3,
    maxStep: 4,
  });

  // find the path to the database. The actual database might be in a sub-folder
  const dbPath = await findDirWithFile(
    unzipPath,
    '.dbinfo',
    'codeql-database.yml'
  );
  if (dbPath) {
    progressCallback?.({
      message: 'Validating and fixing source location',
      step: 4,
      maxStep: 4,
    });
    await ensureZippedSourceLocation(dbPath);

    const item = await databasesManager.openDatabase(Uri.file(dbPath));
    databasesManager.setCurrentDatabaseItem(item);
    return item;
  } else {
    throw new Error('Database not found in archive.');
  }
}

async function getStorageFolder(storagePath: string, urlStr: string) {
  // we need to generate a folder name for the unzipped archive,
  // this needs to be human readable since we may use this name as the initial
  // name for the database
  const url = Uri.parse(urlStr);
  // MacOS has a max filename length of 255
  // and remove a few extra chars in case we need to add a counter at the end.
  let lastName = path.basename(url.path).substring(0, 250);
  if (lastName.endsWith('.zip')) {
    lastName = lastName.substring(0, lastName.length - 4);
  }

  const realpath = await fs.realpath(storagePath);
  let folderName = path.join(realpath, lastName);

  // avoid overwriting existing folders
  let counter = 0;
  while (await fs.pathExists(folderName)) {
    counter++;
    folderName = path.join(realpath, `${lastName}-${counter}`);
    if (counter > 100) {
      throw new Error('Could not find a unique name for downloaded database.');
    }
  }
  return folderName;
}

function validateHttpsUrl(databaseUrl: string) {
  let uri;
  try {
    uri = Uri.parse(databaseUrl, true);
  } catch (e) {
    throw new Error(`Invalid url: ${databaseUrl}`);
  }

  if (uri.scheme !== 'https') {
    throw new Error('Must use https for downloading a database.');
  }
}

async function readAndUnzip(databaseUrl: string, unzipPath: string) {
  const databaseFile = Uri.parse(databaseUrl).fsPath;
  const directory = await unzipper.Open.file(databaseFile);
  await directory.extract({ path: unzipPath });
}

async function fetchAndUnzip(
  databaseUrl: string,
  unzipPath: string,
  progressCallback?: ProgressCallback
) {
  const response = await fetch(databaseUrl);

  await checkForFailingResponse(response);

  const unzipStream = unzipper.Extract({
    path: unzipPath,
  });

  progressCallback?.({
    maxStep: 3,
    message: 'Unzipping database',
    step: 2,
  });
  await new Promise((resolve, reject) => {
    const handler = (err: Error) => {
      if (err.message.startsWith('invalid signature')) {
        reject(new Error('Not a valid archive.'));
      } else {
        reject(err);
      }
    };
    response.body.on('error', handler);
    unzipStream.on('error', handler);
    unzipStream.on('close', resolve);
    response.body.pipe(unzipStream);
  });
}

async function checkForFailingResponse(response: Response): Promise<void | never> {
  if (response.ok) {
    return;
  }

  // An error downloading the database. Attempt to extract the resaon behind it.
  const text = await response.text();
  let msg: string;
  try {
    const obj = JSON.parse(text);
    msg = obj.error || obj.message || obj.reason || JSON.stringify(obj, null, 2);
  } catch (e) {
    msg = text;
  }
  throw new Error(`Error downloading database.\n\nReason: ${msg}`);
}

function isFile(databaseUrl: string) {
  return Uri.parse(databaseUrl).scheme === 'file';
}

/**
 * Recursively looks for a file in a directory. If the file exists, then returns the directory containing the file.
 *
 * @param dir The directory to search
 * @param toFind The file to recursively look for in this directory
 *
 * @returns the directory containing the file, or undefined if not found.
 */
// exported for testing
export async function findDirWithFile(
  dir: string,
  ...toFind: string[]
): Promise<string | undefined> {
  if (!(await fs.stat(dir)).isDirectory()) {
    return;
  }
  const files = await fs.readdir(dir);
  if (toFind.some((file) => files.includes(file))) {
    return dir;
  }
  for (const file of files) {
    const newPath = path.join(dir, file);
    const result = await findDirWithFile(newPath, ...toFind);
    if (result) {
      return result;
    }
  }
  return;
}

/**
 * The URL pattern is https://lgtm.com/projects/{provider}/{org}/{name}/{irrelevant-subpages}.
 * There are several possibilities for the provider: in addition to GitHub.com(g),
 * LGTM currently hosts projects from Bitbucket (b), GitLab (gl) and plain git (git).
 *
 * After the {provider}/{org}/{name} path components, there may be the components
 * related to sub pages.
 *
 * This function accepts any url that matches the patter above
 *
 * @param lgtmUrl The URL to the lgtm project
 *
 * @return true if this looks like an LGTM project url
 */
// exported for testing
export function looksLikeLgtmUrl(lgtmUrl: string | undefined): lgtmUrl is string {
  if (!lgtmUrl) {
    return false;
  }

  try {
    const uri = Uri.parse(lgtmUrl, true);
    if (uri.scheme !== 'https') {
      return false;
    }

    if (uri.authority !== 'lgtm.com' && uri.authority !== 'www.lgtm.com') {
      return false;
    }

    const paths = uri.path.split('/').filter((segment) => segment);
    return paths.length >= 4 && paths[0] === 'projects';
  } catch (e) {
    return false;
  }
}

// exported for testing
export async function convertToDatabaseUrl(lgtmUrl: string) {
  try {
    const uri = Uri.parse(lgtmUrl, true);
    const paths = ['api', 'v1.0'].concat(
      uri.path.split('/').filter((segment) => segment)
    ).slice(0, 6);
    const projectUrl = `https://lgtm.com/${paths.join('/')}`;
    const projectResponse = await fetch(projectUrl);
    const projectJson = await projectResponse.json();

    if (projectJson.code === 404) {
      throw new Error();
    }

    const language = await promptForLanguage(projectJson);
    if (!language) {
      return;
    }
    return `https://lgtm.com/${[
      'api',
      'v1.0',
      'snapshots',
      projectJson.id,
      language,
    ].join('/')}`;
  } catch (e) {
    logger.log(`Error: ${e.message}`);
    throw new Error(`Invalid LGTM URL: ${lgtmUrl}`);
  }
}

async function promptForLanguage(
  projectJson: any
): Promise<string | undefined> {
  if (!projectJson?.languages?.length) {
    return;
  }
  if (projectJson.languages.length === 1) {
    return projectJson.languages[0].language;
  }

  return await window.showQuickPick(
    projectJson.languages.map((lang: { language: string }) => lang.language), {
    placeHolder: 'Select the database language to download:'
  }
  );
}

/**
 * Databases created by the old odasa tool will not have a zipped
 * source location. However, this extension works better if sources
 * are zipped.
 *
 * This function ensures that the source location is zipped. If the
 * `src` folder exists and the `src.zip` file does not, the `src`
 * folder will be zipped and then deleted.
 *
 * @param databasePath The full path to the unzipped database
 */
async function ensureZippedSourceLocation(databasePath: string): Promise<void> {
  const srcFolderPath = path.join(databasePath, 'src');
  const srcZipPath = srcFolderPath + '.zip';

  if ((await fs.pathExists(srcFolderPath)) && !(await fs.pathExists(srcZipPath))) {
    await zip(srcFolderPath, srcZipPath);
    await fs.remove(srcFolderPath);
  }
}
