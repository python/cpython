import { expect } from 'chai';
import * as vscode from 'vscode';
import * as path from 'path';
import * as sinon from 'sinon';
import * as tmp from 'tmp';
import { window, ViewColumn, Uri } from 'vscode';
import {
  fileUriToWebviewUri,
  tryResolveLocation,
} from '../../interface-utils';
import { getDefaultResultSetName } from '../../interface-types';
import { DatabaseItem } from '../../databases';

describe('interface-utils', () => {
  describe('webview uri conversion', function() {
    const fileSuffix = '.bqrs';

    function setupWebview(filePrefix: string) {
      const tmpFile = tmp.fileSync({
        prefix: `uri_test_${filePrefix}_`,
        postfix: fileSuffix,
        keep: false,
      });
      const fileUriOnDisk = Uri.file(tmpFile.name);
      const panel = window.createWebviewPanel(
        'test panel',
        'test panel',
        ViewColumn.Beside,
        {
          enableScripts: false,
          localResourceRoots: [fileUriOnDisk],
        }
      );

      after(function() {
        panel.dispose();
        tmpFile.removeCallback();
      });

      // CSP allowing nothing, to prevent warnings.
      const html = '<html><head><meta http-equiv="Content-Security-Policy" content="default-src \'none\';"></head></html>';
      panel.webview.html = html;
      return {
        fileUriOnDisk,
        panel,
      };
    }

    it('does not double-encode # in URIs', function() {
      const { fileUriOnDisk, panel } = setupWebview('#');
      const webviewUri = fileUriToWebviewUri(panel, fileUriOnDisk);
      const parsedUri = Uri.parse(webviewUri);
      expect(path.basename(parsedUri.path, fileSuffix)).to.equal(
        path.basename(fileUriOnDisk.path, fileSuffix)
      );
    });
  });

  describe('getDefaultResultSetName', () => {
    it('should get the default name', () => {
      expect(getDefaultResultSetName(['a', 'b', '#select', 'alerts'])).to.equal(
        'alerts'
      );
      expect(getDefaultResultSetName(['a', 'b', '#select'])).to.equal(
        '#select'
      );
      expect(getDefaultResultSetName(['a', 'b'])).to.equal('a');
      expect(getDefaultResultSetName([])).to.be.undefined;
    });
  });

  describe('resolveWholeFileLocation', () => {
    it('should resolve a whole file location', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.file('abc')),
      } as unknown) as DatabaseItem;
      expect(
        tryResolveLocation(
          'file://hucairz:0:0:0:0',
          mockDatabaseItem
        )
      ).to.deep.equal(
        new vscode.Location(
          vscode.Uri.file('abc'),
          new vscode.Range(0, 0, 0, 0)
        )
      );
    });

    it('should resolve a five-part location edge case', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.file('abc')),
      } as unknown) as DatabaseItem;
      expect(
        tryResolveLocation(
          'file://hucairz:1:1:1:1',
          mockDatabaseItem
        )
      ).to.deep.equal(
        new vscode.Location(
          vscode.Uri.file('abc'),
          new vscode.Range(0, 0, 0, 1)
        )
      );
    });

    it('should resolve a five-part location', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.parse('abc')),
      } as unknown) as DatabaseItem;

      expect(
        tryResolveLocation(
          {
            startColumn: 1,
            endColumn: 3,
            startLine: 4,
            endLine: 5,
            uri: 'hucairz',
          },
          mockDatabaseItem
        )
      ).to.deep.equal(
        new vscode.Location(
          vscode.Uri.parse('abc'),
          new vscode.Range(new vscode.Position(4, 3), new vscode.Position(3, 0))
        )
      );
      expect(mockDatabaseItem.resolveSourceFile).to.have.been.calledOnceWith(
        'hucairz'
      );
    });

    it('should resolve a five-part location with an empty path', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.parse('abc')),
      } as unknown) as DatabaseItem;

      expect(
        tryResolveLocation(
          {
            startColumn: 1,
            endColumn: 3,
            startLine: 4,
            endLine: 5,
            uri: '',
          },
          mockDatabaseItem
        )
      ).to.be.undefined;
    });

    it('should resolve a string location for whole file', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.parse('abc')),
      } as unknown) as DatabaseItem;

      expect(
        tryResolveLocation(
          'file://hucairz:0:0:0:0',
          mockDatabaseItem
        )
      ).to.deep.equal(
        new vscode.Location(
          vscode.Uri.parse('abc'),
          new vscode.Range(0, 0, 0, 0)
        )
      );
      expect(mockDatabaseItem.resolveSourceFile).to.have.been.calledOnceWith(
        'hucairz'
      );
    });

    it('should resolve a string location for five-part location', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.parse('abc')),
      } as unknown) as DatabaseItem;

      expect(
        tryResolveLocation(
          'file://hucairz:5:4:3:2',
          mockDatabaseItem
        )
      ).to.deep.equal(
        new vscode.Location(
          vscode.Uri.parse('abc'),
          new vscode.Range(new vscode.Position(4, 3), new vscode.Position(2, 2))
        )
      );
      expect(mockDatabaseItem.resolveSourceFile).to.have.been.calledOnceWith(
        'hucairz'
      );
    });

    it('should resolve a string location for invalid string', () => {
      const mockDatabaseItem: DatabaseItem = ({
        resolveSourceFile: sinon.stub().returns(vscode.Uri.parse('abc')),
      } as unknown) as DatabaseItem;

      expect(
        tryResolveLocation(
          'file://hucairz:x:y:z:a',
          mockDatabaseItem
        )
      ).to.be.undefined;
    });
  });
});
