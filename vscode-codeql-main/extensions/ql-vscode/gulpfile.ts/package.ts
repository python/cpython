import * as path from 'path';
import { deployPackage } from './deploy';
import * as childProcess from 'child-process-promise';

export async function packageExtension(): Promise<void> {
  const deployedPackage = await deployPackage(path.resolve('package.json'));
  console.log(`Packaging extension '${deployedPackage.name}@${deployedPackage.version}'...`);
  const args = [
    'package',
    '--out', path.resolve(deployedPackage.distPath, '..', `${deployedPackage.name}-${deployedPackage.version}.vsix`)
  ];
  const proc = childProcess.spawn('./node_modules/.bin/vsce', args, {
    cwd: deployedPackage.distPath
  });
  proc.childProcess.stdout!.on('data', (data) => {
    console.log(data.toString());
  });
  proc.childProcess.stderr!.on('data', (data) => {
    console.error(data.toString());
  });

  await proc;
}
