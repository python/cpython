import { expect } from 'chai';
import * as path from 'path';
import * as vscode from 'vscode';
import { Uri } from 'vscode';
import { determineSelectedQuery } from '../../run-queries';

async function showQlDocument(name: string): Promise<vscode.TextDocument> {
  const folderPath = vscode.workspace.workspaceFolders![0].uri.fsPath;
  const documentPath = path.resolve(folderPath, name);
  const document = await vscode.workspace.openTextDocument(documentPath);
  await vscode.window.showTextDocument(document!);
  return document;
}

export function run() {
  describe('Determining selected query', async () => {
    it('should allow ql files to be queried', async () => {
      const q = await determineSelectedQuery(Uri.parse('file:///tmp/queryname.ql'), false);
      expect(q.queryPath).to.equal(path.join('/', 'tmp', 'queryname.ql'));
      expect(q.quickEvalPosition).to.equal(undefined);
    });

    it('should allow ql files to be quick-evaled', async () => {
      const doc = await showQlDocument('query.ql');
      const q = await determineSelectedQuery(doc.uri, true);
      expect(q.queryPath).to.satisfy((p: string) => p.endsWith(path.join('ql-vscode', 'test', 'data', 'query.ql')));
    });

    it('should allow qll files to be quick-evaled', async () => {
      const doc = await showQlDocument('library.qll');
      const q = await determineSelectedQuery(doc.uri, true);
      expect(q.queryPath).to.satisfy((p: string) => p.endsWith(path.join('ql-vscode', 'test', 'data', 'library.qll')));
    });

    it('should reject non-ql files when running a query', async () => {
      await expect(determineSelectedQuery(Uri.parse('file:///tmp/queryname.txt'), false)).to.be.rejectedWith(Error, 'The selected resource is not a CodeQL query file');
      await expect(determineSelectedQuery(Uri.parse('file:///tmp/queryname.qll'), false)).to.be.rejectedWith(Error, 'The selected resource is not a CodeQL query file');
    });

    it('should reject non-ql[l] files when running a quick eval', async () => {
      await expect(determineSelectedQuery(Uri.parse('file:///tmp/queryname.txt'), true)).to.be.rejectedWith(Error, 'The selected resource is not a CodeQL file');
    });
  });
}
