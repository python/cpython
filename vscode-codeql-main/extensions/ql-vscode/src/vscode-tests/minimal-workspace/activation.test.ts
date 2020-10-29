import * as assert from 'assert';
import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import 'mocha';
import * as path from 'path';
import * as vscode from 'vscode';
import * as determiningSelectedQueryTest from './determining-selected-query-test';

chai.use(chaiAsPromised);

describe('launching with a minimal workspace', async () => {
  const ext = vscode.extensions.getExtension('GitHub.vscode-codeql');
  it('should install the extension', () => {
    assert(ext);
  });

  it('should not activate the extension at first', () => {
    assert(ext!.isActive === false);
  });

  it('should activate the extension when a .ql file is opened', async function() {
    const folders = vscode.workspace.workspaceFolders;
    assert(folders && folders.length === 1);
    const folderPath = folders![0].uri.fsPath;
    const documentPath = path.resolve(folderPath, 'query.ql');
    const document = await vscode.workspace.openTextDocument(documentPath);
    assert(document.languageId === 'ql');
    // Delay slightly so that the extension has time to activate.
    this.timeout(4000);
    await new Promise(resolve => setTimeout(resolve, 2000));
    assert(ext!.isActive);
  });
});

determiningSelectedQueryTest.run();
