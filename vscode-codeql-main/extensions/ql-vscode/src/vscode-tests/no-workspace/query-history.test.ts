import * as chai from 'chai';
import 'mocha';
import 'sinon-chai';
import * as vscode from 'vscode';
import * as sinon from 'sinon';
import * as chaiAsPromised from 'chai-as-promised';
import { logger } from '../../logging';
import { QueryHistoryManager } from '../../query-history';

chai.use(chaiAsPromised);
const expect = chai.expect;
const assert = chai.assert;


describe('query-history', () => {
  let showTextDocumentSpy: sinon.SinonStub;
  let showInformationMessageSpy: sinon.SinonStub;
  let executeCommandSpy: sinon.SinonStub;
  let showQuickPickSpy: sinon.SinonStub;

  let tryOpenExternalFile: Function;

  beforeEach(() => {
    showTextDocumentSpy = sinon.stub(vscode.window, 'showTextDocument');
    showInformationMessageSpy = sinon.stub(
      vscode.window,
      'showInformationMessage'
    );
    showQuickPickSpy = sinon.stub(
      vscode.window,
      'showQuickPick'
    );
    executeCommandSpy = sinon.stub(vscode.commands, 'executeCommand');
    sinon.stub(logger, 'log');
    tryOpenExternalFile = (QueryHistoryManager.prototype as any).tryOpenExternalFile;
  });

  afterEach(() => {
    (vscode.window.showTextDocument as sinon.SinonStub).restore();
    (vscode.commands.executeCommand as sinon.SinonStub).restore();
    (logger.log as sinon.SinonStub).restore();
    (vscode.window.showInformationMessage as sinon.SinonStub).restore();
    (vscode.window.showQuickPick as sinon.SinonStub).restore();
  });

  describe('tryOpenExternalFile', () => {

    it('should open an external file', async () => {
      await tryOpenExternalFile('xxx');
      expect(showTextDocumentSpy).to.have.been.calledOnceWith(
        vscode.Uri.file('xxx')
      );
      expect(executeCommandSpy).not.to.have.been.called;
    });

    [
      'too large to open',
      'Files above 50MB cannot be synchronized with extensions',
    ].forEach(msg => {
      it(`should fail to open a file because "${msg}" and open externally`, async () => {
        showTextDocumentSpy.throws(new Error(msg));
        showInformationMessageSpy.returns({ title: 'Yes' });

        await tryOpenExternalFile('xxx');
        const uri = vscode.Uri.file('xxx');
        expect(showTextDocumentSpy).to.have.been.calledOnceWith(
          uri
        );
        expect(executeCommandSpy).to.have.been.calledOnceWith(
          'revealFileInOS',
          uri
        );
      });

      it(`should fail to open a file because "${msg}" and NOT open externally`, async () => {
        showTextDocumentSpy.throws(new Error(msg));
        showInformationMessageSpy.returns({ title: 'No' });

        await tryOpenExternalFile('xxx');
        const uri = vscode.Uri.file('xxx');
        expect(showTextDocumentSpy).to.have.been.calledOnceWith(uri);
        expect(showInformationMessageSpy).to.have.been.called;
        expect(executeCommandSpy).not.to.have.been.called;
      });
    });
  });

  describe('findOtherQueryToCompare', () => {
    let allHistory: { database: { name: string }; didRunSuccessfully: boolean }[];

    beforeEach(() => {
      allHistory = [
        { didRunSuccessfully: true, database: { name: 'a' } },
        { didRunSuccessfully: true, database: { name: 'b' } },
        { didRunSuccessfully: false, database: { name: 'a' } },
        { didRunSuccessfully: true, database: { name: 'a' } },
      ];
    });

    it('should find the second query to compare when one is selected', async () => {
      const thisQuery = allHistory[3];
      const queryHistory = createMockQueryHistory(allHistory);
      showQuickPickSpy.returns({ query: allHistory[0] });

      const otherQuery = await queryHistory.findOtherQueryToCompare(thisQuery, []);
      expect(otherQuery).to.eq(allHistory[0]);

      // only called with first item, other items filtered out
      expect(showQuickPickSpy.getCalls().length).to.eq(1);
      expect(showQuickPickSpy.firstCall.args[0][0].query).to.eq(allHistory[0]);
    });

    it('should handle cancelling out of the quick select', async () => {
      const thisQuery = allHistory[3];
      const queryHistory = createMockQueryHistory(allHistory);

      const otherQuery = await queryHistory.findOtherQueryToCompare(thisQuery, []);
      expect(otherQuery).to.be.undefined;

      // only called with first item, other items filtered out
      expect(showQuickPickSpy.getCalls().length).to.eq(1);
      expect(showQuickPickSpy.firstCall.args[0][0].query).to.eq(allHistory[0]);
    });

    it('should compare against 2 queries', async () => {
      const thisQuery = allHistory[3];
      const queryHistory = createMockQueryHistory(allHistory);

      const otherQuery = await queryHistory.findOtherQueryToCompare(thisQuery, [thisQuery, allHistory[0]]);
      expect(otherQuery).to.eq(allHistory[0]);
      expect(showQuickPickSpy).not.to.have.been.called;
    });

    it('should throw an error when a query is not successful', async () => {
      const thisQuery = allHistory[3];
      const queryHistory = createMockQueryHistory(allHistory);
      allHistory[0].didRunSuccessfully = false;

      try {
        await queryHistory.findOtherQueryToCompare(thisQuery, [thisQuery, allHistory[0]]);
        assert(false, 'Should have thrown');
      } catch (e) {
        expect(e.message).to.eq('Please select a successful query.');
      }
    });

    it('should throw an error when a databases are not the same', async () => {
      const thisQuery = allHistory[3];
      const queryHistory = createMockQueryHistory(allHistory);
      allHistory[0].database.name = 'c';

      try {
        await queryHistory.findOtherQueryToCompare(thisQuery, [thisQuery, allHistory[0]]);
        assert(false, 'Should have thrown');
      } catch (e) {
        expect(e.message).to.eq('Query databases must be the same.');
      }
    });

    it('should throw an error when more than 2 queries selected', async () => {
      const thisQuery = allHistory[3];
      const queryHistory = createMockQueryHistory(allHistory);

      try {
        await queryHistory.findOtherQueryToCompare(thisQuery, [thisQuery, allHistory[0], allHistory[1]]);
        assert(false, 'Should have thrown');
      } catch (e) {
        expect(e.message).to.eq('Please select no more than 2 queries.');
      }
    });
  });

  describe('updateCompareWith', () => {
    it('should update compareWithItem when there is a single item', () => {
      const queryHistory = createMockQueryHistory([]);
      queryHistory.updateCompareWith(['a']);
      expect(queryHistory.compareWithItem).to.be.eq('a');
    });

    it('should delete compareWithItem when there are 0 items', () => {
      const queryHistory = createMockQueryHistory([]);
      queryHistory.compareWithItem = 'a';
      queryHistory.updateCompareWith([]);
      expect(queryHistory.compareWithItem).to.be.undefined;
    });

    it('should delete compareWithItem when there are more than 2 items', () => {
      const queryHistory = createMockQueryHistory([]);
      queryHistory.compareWithItem = 'a';
      queryHistory.updateCompareWith(['a', 'b', 'c']);
      expect(queryHistory.compareWithItem).to.be.undefined;
    });

    it('should delete compareWithItem when there are 2 items and disjoint from compareWithItem', () => {
      const queryHistory = createMockQueryHistory([]);
      queryHistory.compareWithItem = 'a';
      queryHistory.updateCompareWith(['b', 'c']);
      expect(queryHistory.compareWithItem).to.be.undefined;
    });

    it('should do nothing when compareWithItem exists and exactly 2 items', () => {
      const queryHistory = createMockQueryHistory([]);
      queryHistory.compareWithItem = 'a';
      queryHistory.updateCompareWith(['a', 'b']);
      expect(queryHistory.compareWithItem).to.be.eq('a');
    });
  });
});

function createMockQueryHistory(allHistory: {}[]) {
  return {
    assertSingleQuery: (QueryHistoryManager.prototype as any).assertSingleQuery,
    findOtherQueryToCompare: (QueryHistoryManager.prototype as any).findOtherQueryToCompare,
    treeDataProvider: {
      allHistory
    },
    updateCompareWith: (QueryHistoryManager.prototype as any).updateCompareWith,
    compareWithItem: undefined as undefined | string,
  };
}
