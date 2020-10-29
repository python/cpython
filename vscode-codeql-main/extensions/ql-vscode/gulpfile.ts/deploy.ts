import * as fs from 'fs-extra';
import * as jsonc from 'jsonc-parser';
import * as path from 'path';

export interface DeployedPackage {
  distPath: string;
  name: string;
  version: string;
}

const packageFiles = [
  '.vscodeignore',
  'CHANGELOG.md',
  'README.md',
  'language-configuration.json',
  'media',
  'node_modules',
  'out'
];

async function copyPackage(sourcePath: string, destPath: string): Promise<void> {
  for (const file of packageFiles) {
    console.log(`copying ${path.resolve(sourcePath, file)} to ${path.resolve(destPath, file)}`);
    await fs.copy(path.resolve(sourcePath, file), path.resolve(destPath, file));
  }
}

export async function deployPackage(packageJsonPath: string): Promise<DeployedPackage> {
  try {
    const packageJson: any = jsonc.parse(await fs.readFile(packageJsonPath, 'utf8'));

    // Default to development build; use flag --release to indicate release build.
    const isDevBuild = !process.argv.includes('--release');
    const distDir = path.join(__dirname, '../../../dist');
    await fs.mkdirs(distDir);

    if (isDevBuild) {
      // NOTE: rootPackage.name had better not have any regex metacharacters
      const oldDevBuildPattern = new RegExp('^' + packageJson.name + '[^/]+-dev[0-9.]+\\.vsix$');
      // Dev package filenames are of the form
      //    vscode-codeql-0.0.1-dev.2019.9.27.19.55.20.vsix
      (await fs.readdir(distDir)).filter(name => name.match(oldDevBuildPattern)).map(build => {
        console.log(`Deleting old dev build ${build}...`);
        fs.unlinkSync(path.join(distDir, build));
      });
      const now = new Date();
      packageJson.version = packageJson.version +
        `-dev.${now.getUTCFullYear()}.${now.getUTCMonth() + 1}.${now.getUTCDate()}` +
        `.${now.getUTCHours()}.${now.getUTCMinutes()}.${now.getUTCSeconds()}`;
    }

    const distPath = path.join(distDir, packageJson.name);
    await fs.remove(distPath);
    await fs.mkdirs(distPath);

    await fs.writeFile(path.join(distPath, 'package.json'), JSON.stringify(packageJson, null, 2));

    const sourcePath = path.join(__dirname, '..');
    console.log(`Copying package '${packageJson.name}' and its dependencies to '${distPath}'...`);
    await copyPackage(sourcePath, distPath);

    return {
      distPath: distPath,
      name: packageJson.name,
      version: packageJson.version
    };
  }
  catch (e) {
    console.error(e);
    throw e;
  }
}
