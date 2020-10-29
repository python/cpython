import * as fs from 'fs-extra';
import * as path from 'path';


/**
 * Recursively finds all .ql files in this set of Uris.
 *
 * @param paths The list of Uris to search through
 *
 * @returns list of ql files and a boolean describing whether or not a directory was found/
 */
export async function gatherQlFiles(paths: string[]): Promise<[string[], boolean]> {
  const gatheredUris: Set<string> = new Set();
  let dirFound = false;
  for (const nextPath of paths) {
    if (
      (await fs.pathExists(nextPath)) &&
      (await fs.stat(nextPath)).isDirectory()
    ) {
      dirFound = true;
      const subPaths = await fs.readdir(nextPath);
      const fullPaths = subPaths.map(p => path.join(nextPath, p));
      const nestedFiles = (await gatherQlFiles(fullPaths))[0];
      nestedFiles.forEach(nested => gatheredUris.add(nested));
    } else if (nextPath.endsWith('.ql')) {
      gatheredUris.add(nextPath);
    }
  }
  return [Array.from(gatheredUris), dirFound];
}
