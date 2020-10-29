import * as fs from 'fs-extra';
import * as unzipper from 'unzipper';
import * as vscode from 'vscode';
import { logger } from './logging';

// All path operations in this file must be on paths *within* the zip
// archive.
import * as _path from 'path';
const path = _path.posix;

export class File implements vscode.FileStat {
  type: vscode.FileType;
  ctime: number;
  mtime: number;
  size: number;

  constructor(public name: string, public data: Uint8Array) {
    this.type = vscode.FileType.File;
    this.ctime = Date.now();
    this.mtime = Date.now();
    this.size = data.length;
    this.name = name;
  }
}

export class Directory implements vscode.FileStat {
  type: vscode.FileType;
  ctime: number;
  mtime: number;
  size: number;
  entries: Map<string, Entry> = new Map();

  constructor(public name: string) {
    this.type = vscode.FileType.Directory;
    this.ctime = Date.now();
    this.mtime = Date.now();
    this.size = 0;
  }
}

export type Entry = File | Directory;

/**
 * A map containing directory hierarchy information in a convenient form.
 *
 * For example, if dirMap : DirectoryHierarchyMap, and /foo/bar/baz.c is a file in the
 * directory structure being represented, then
 *
 * dirMap['/foo'] = {'bar': vscode.FileType.Directory}
 * dirMap['/foo/bar'] = {'baz': vscode.FileType.File}
 */
export type DirectoryHierarchyMap = Map<string, Map<string, vscode.FileType>>;

export type ZipFileReference = {
  sourceArchiveZipPath: string;
  pathWithinSourceArchive: string;
};

/** Encodes a reference to a source file within a zipped source archive into a single URI. */
export function encodeSourceArchiveUri(ref: ZipFileReference): vscode.Uri {
  const { sourceArchiveZipPath, pathWithinSourceArchive } = ref;

  // These two paths are put into a single URI with a custom scheme.
  // The path and authority components of the URI encode the two paths.

  // The path component of the URI contains both paths, joined by a slash.
  let encodedPath = path.join(sourceArchiveZipPath, pathWithinSourceArchive);

  // If a URI contains an authority component, then the path component
  // must either be empty or begin with a slash ("/") character.
  // (Source: https://tools.ietf.org/html/rfc3986#section-3.3)
  // Since we will use an authority component, we add a leading slash if necessary
  // (paths on Windows usually start with the drive letter).
  let sourceArchiveZipPathStartIndex: number;
  if (encodedPath.startsWith('/')) {
    sourceArchiveZipPathStartIndex = 0;
  } else {
    encodedPath = '/' + encodedPath;
    sourceArchiveZipPathStartIndex = 1;
  }

  // The authority component of the URI records the 0-based inclusive start and exclusive end index
  // of the source archive zip path within the path component of the resulting URI.
  // This lets us separate the paths, ignoring the leading slash if we added one.
  const sourceArchiveZipPathEndIndex = sourceArchiveZipPathStartIndex + sourceArchiveZipPath.length;
  const authority = `${sourceArchiveZipPathStartIndex}-${sourceArchiveZipPathEndIndex}`;
  return vscode.Uri.parse(zipArchiveScheme + ':/').with({
    path: encodedPath,
    authority,
  });
}

/**
 * Convenience method to create a codeql-zip-archive with a path to the root
 * archive
 *
 * @param pathToArchive the filesystem path to the root of the archive
 */
export function encodeArchiveBasePath(sourceArchiveZipPath: string) {
  return encodeSourceArchiveUri({
    sourceArchiveZipPath,
    pathWithinSourceArchive: ''
  });
}

const sourceArchiveUriAuthorityPattern = /^(\d+)-(\d+)$/;

class InvalidSourceArchiveUriError extends Error {
  constructor(uri: vscode.Uri) {
    super(`Can't decode uri ${uri}: authority should be of the form startIndex-endIndex (where both indices are integers).`);
  }
}

/** Decodes an encoded source archive URI into its corresponding paths. Inverse of `encodeSourceArchiveUri`. */
export function decodeSourceArchiveUri(uri: vscode.Uri): ZipFileReference {
  if (!uri.authority) {
    // Uri is malformed, but this is recoverable
    logger.log(`Warning: ${new InvalidSourceArchiveUriError(uri).message}`);
    return {
      pathWithinSourceArchive: '/',
      sourceArchiveZipPath: uri.path
    };
  }
  const match = sourceArchiveUriAuthorityPattern.exec(uri.authority);
  if (match === null)
    throw new InvalidSourceArchiveUriError(uri);
  const zipPathStartIndex = parseInt(match[1]);
  const zipPathEndIndex = parseInt(match[2]);
  if (isNaN(zipPathStartIndex) || isNaN(zipPathEndIndex))
    throw new InvalidSourceArchiveUriError(uri);
  return {
    pathWithinSourceArchive: uri.path.substring(zipPathEndIndex) || '/',
    sourceArchiveZipPath: uri.path.substring(zipPathStartIndex, zipPathEndIndex),
  };
}

/**
 * Make sure `file` and all of its parent directories are represented in `map`.
 */
function ensureFile(map: DirectoryHierarchyMap, file: string) {
  const dirname = path.dirname(file);
  if (dirname === '.') {
    const error = `Ill-formed path ${file} in zip archive (expected absolute path)`;
    logger.log(error);
    throw new Error(error);
  }
  ensureDir(map, dirname);
  map.get(dirname)!.set(path.basename(file), vscode.FileType.File);
}

/**
 * Make sure `dir` and all of its parent directories are represented in `map`.
 */
function ensureDir(map: DirectoryHierarchyMap, dir: string) {
  const parent = path.dirname(dir);
  if (!map.has(dir)) {
    map.set(dir, new Map);
    if (dir !== parent) { // not the root directory
      ensureDir(map, parent);
      map.get(parent)!.set(path.basename(dir), vscode.FileType.Directory);
    }
  }
}

type Archive = {
  unzipped: unzipper.CentralDirectory;
  dirMap: DirectoryHierarchyMap;
};

export class ArchiveFileSystemProvider implements vscode.FileSystemProvider {
  private readOnlyError = vscode.FileSystemError.NoPermissions('write operation attempted, but source archive filesystem is readonly');
  private archives: Map<string, Archive> = new Map;

  private async getArchive(zipPath: string): Promise<Archive> {
    if (!this.archives.has(zipPath)) {
      if (!await fs.pathExists(zipPath))
        throw vscode.FileSystemError.FileNotFound(zipPath);
      const archive: Archive = { unzipped: await unzipper.Open.file(zipPath), dirMap: new Map };
      archive.unzipped.files.forEach(f => { ensureFile(archive.dirMap, path.resolve('/', f.path)); });
      this.archives.set(zipPath, archive);
    }
    return this.archives.get(zipPath)!;
  }

  root = new Directory('');

  // metadata

  async stat(uri: vscode.Uri): Promise<vscode.FileStat> {
    return await this._lookup(uri);
  }

  async readDirectory(uri: vscode.Uri): Promise<[string, vscode.FileType][]> {
    const ref = decodeSourceArchiveUri(uri);
    const archive = await this.getArchive(ref.sourceArchiveZipPath);
    const contents = archive.dirMap.get(ref.pathWithinSourceArchive);
    const result = contents === undefined ? undefined : Array.from(contents.entries());
    if (result === undefined) {
      throw vscode.FileSystemError.FileNotFound(uri);
    }
    return result;
  }

  // file contents

  async readFile(uri: vscode.Uri): Promise<Uint8Array> {
    const data = (await this._lookupAsFile(uri)).data;
    if (data) {
      return data;
    }
    throw vscode.FileSystemError.FileNotFound();
  }

  // write operations, all disabled

  writeFile(_uri: vscode.Uri, _content: Uint8Array, _options: { create: boolean; overwrite: boolean }): void {
    throw this.readOnlyError;
  }

  rename(_oldUri: vscode.Uri, _newUri: vscode.Uri, _options: { overwrite: boolean }): void {
    throw this.readOnlyError;
  }

  delete(_uri: vscode.Uri): void {
    throw this.readOnlyError;
  }

  createDirectory(_uri: vscode.Uri): void {
    throw this.readOnlyError;
  }

  // content lookup

  private async _lookup(uri: vscode.Uri): Promise<Entry> {
    const ref = decodeSourceArchiveUri(uri);
    const archive = await this.getArchive(ref.sourceArchiveZipPath);

    // this is a path inside the archive, so don't use `.fsPath`, and
    // use '/' as path separator throughout
    const reqPath = ref.pathWithinSourceArchive;

    const file = archive.unzipped.files.find(
      f => {
        const absolutePath = path.resolve('/', f.path);
        return absolutePath === reqPath
          || absolutePath === path.join('/src_archive', reqPath);
      }
    );
    if (file !== undefined) {
      if (file.type === 'File') {
        return new File(reqPath, await file.buffer());
      }
      else { // file.type === 'Directory'
        // I haven't observed this case in practice. Could it happen
        // with a zip file that contains empty directories?
        return new Directory(reqPath);
      }
    }
    if (archive.dirMap.has(reqPath)) {
      return new Directory(reqPath);
    }
    throw vscode.FileSystemError.FileNotFound(`uri '${uri.toString()}', interpreted as '${reqPath}' in archive '${ref.sourceArchiveZipPath}'`);
  }

  private async _lookupAsFile(uri: vscode.Uri): Promise<File> {
    const entry = await this._lookup(uri);
    if (entry instanceof File) {
      return entry;
    }
    throw vscode.FileSystemError.FileIsADirectory(uri);
  }

  // file events

  private _emitter = new vscode.EventEmitter<vscode.FileChangeEvent[]>();

  readonly onDidChangeFile: vscode.Event<vscode.FileChangeEvent[]> = this._emitter.event;

  watch(_resource: vscode.Uri): vscode.Disposable {
    // ignore, fires for all changes...
    return new vscode.Disposable(() => { /**/ });
  }
}

/**
 * Custom uri scheme for referring to files inside zip archives stored
 * in the filesystem. See `encodeSourceArchiveUri`/`decodeSourceArchiveUri` for
 * how these uris are constructed.
 *
 * (cf. https://www.ietf.org/rfc/rfc2396.txt (Appendix A, page 26) for
 * the fact that hyphens are allowed in uri schemes)
 */
export const zipArchiveScheme = 'codeql-zip-archive';

export function activate(ctx: vscode.ExtensionContext) {
  ctx.subscriptions.push(vscode.workspace.registerFileSystemProvider(
    zipArchiveScheme,
    new ArchiveFileSystemProvider(),
    {
      isCaseSensitive: true,
      isReadonly: true,
    }
  ));
}
