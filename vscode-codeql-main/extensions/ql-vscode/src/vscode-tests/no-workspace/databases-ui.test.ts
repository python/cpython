import 'vscode-test';
import 'mocha';
import * as tmp from 'tmp';
import * as path from 'path';
import * as fs from 'fs-extra';
import { expect } from 'chai';
import { Uri } from 'vscode';

import { DatabaseUI } from '../../databases-ui';

describe('databases-ui', () => {
  describe('fixDbUri', () => {
    const fixDbUri = (DatabaseUI.prototype as any).fixDbUri;
    it('should choose current directory direcory normally', async () => {
      const dir = tmp.dirSync().name;
      const uri = await fixDbUri(Uri.file(dir));
      expect(uri.toString()).to.eq(Uri.file(dir).toString());
    });

    it('should choose parent direcory when file is selected', async () => {
      const file = tmp.fileSync().name;
      const uri = await fixDbUri(Uri.file(file));
      expect(uri.toString()).to.eq(Uri.file(path.dirname(file)).toString());
    });

    it('should choose parent direcory when db-* is selected', async () => {
      const dir = tmp.dirSync().name;
      const dbDir = path.join(dir, 'db-javascript');
      await fs.mkdirs(dbDir);

      const uri = await fixDbUri(Uri.file(dbDir));
      expect(uri.toString()).to.eq(Uri.file(dir).toString());
    });

    it('should choose parent\'s parent direcory when file selected is in db-*', async () => {
      const dir = tmp.dirSync().name;
      const dbDir = path.join(dir, 'db-javascript');
      const file = path.join(dbDir, 'nested');
      await fs.mkdirs(dbDir);
      await fs.createFile(file);

      const uri = await fixDbUri(Uri.file(file));
      expect(uri.toString()).to.eq(Uri.file(dir).toString());
    });

    it('should handle a parent whose name is db-*', async () => {
      // fixes https://github.com/github/vscode-codeql/issues/482
      const dir = tmp.dirSync().name;
      const parentDir = path.join(dir, 'db-hucairz');
      const dbDir = path.join(parentDir, 'db-javascript');
      const file = path.join(dbDir, 'nested');
      await fs.mkdirs(dbDir);
      await fs.createFile(file);

      const uri = await fixDbUri(Uri.file(file));
      expect(uri.toString()).to.eq(Uri.file(parentDir).toString());
    });
  });
});
