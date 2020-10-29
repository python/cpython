import * as semver from 'semver';
import { runCodeQlCliCommand } from './cli';
import { Logger } from './logging';

/**
 * Get the version of a CodeQL CLI.
 */
export async function getCodeQlCliVersion(codeQlPath: string, logger: Logger): Promise<semver.SemVer | undefined> {
  try {
    const output: string = await runCodeQlCliCommand(
      codeQlPath,
      ['version'],
      ['--format=terse'],
      'Checking CodeQL version',
      logger
    );
    return semver.parse(output.trim()) || undefined;
  } catch (e) {
    // Failed to run the version command. This might happen if the cli version is _really_ old, or it is corrupted.
    // Either way, we can't determine compatibility.
    logger.log(`Failed to run 'codeql version'. Reason: ${e.message}`);
    return undefined;
  }
}
