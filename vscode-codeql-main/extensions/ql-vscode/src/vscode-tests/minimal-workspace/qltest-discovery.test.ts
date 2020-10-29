import 'vscode-test';
import 'mocha';
import * as path from 'path';
import { Uri, workspace } from 'vscode';
import { expect } from 'chai';

import { QLTestDiscovery } from '../../qltest-discovery';

describe('qltest-discovery', () => {
  describe('isRelevantQlPack', () => {
    it('should check if a qlpack is relevant', () => {
      const qlTestDiscover: any = new QLTestDiscovery(
        { onDidChangeQLPacks: () => ({}) } as any,
        { index: 0 } as any,
        {} as any
      );

      const uri = workspace.workspaceFolders![0].uri;
      expect(qlTestDiscover.isRelevantQlPack({
        name: '-hucairz',
        uri
      })).to.be.false;

      expect(qlTestDiscover.isRelevantQlPack({
        name: '-tests',
        uri: Uri.file('/a/b/')
      })).to.be.false;

      expect(qlTestDiscover.isRelevantQlPack({
        name: '-tests',
        uri
      })).to.be.true;

      expect(qlTestDiscover.isRelevantQlPack({
        name: '-tests',
        uri: Uri.file(path.join(uri.fsPath, 'other'))
      })).to.be.true;
    });
  });
});
