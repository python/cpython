import 'vscode-test';
import 'mocha';
import * as yaml from 'js-yaml';
import * as chaiAsPromised from 'chai-as-promised';
import * as sinon from 'sinon';
import * as chai from 'chai';
import * as sinonChai from 'sinon-chai';
import * as pq from 'proxyquire';
import { KeyType } from '../../../contextual/keyType';

const proxyquire = pq.noPreserveCache().noCallThru();
chai.use(chaiAsPromised);
chai.use(sinonChai);
const expect = chai.expect;

describe('queryResolver', () => {
  let module: Record<string, Function>;
  let writeFileSpy: sinon.SinonSpy;
  let resolveDatasetFolderSpy: sinon.SinonStub;
  let mockCli: Record<string, sinon.SinonStub>;
  beforeEach(() => {
    mockCli = {
      resolveQueriesInSuite: sinon.stub()
    };
    module = createModule();
  });

  describe('resolveQueries', () => {

    it('should resolve a query', async () => {
      mockCli.resolveQueriesInSuite.returns(['a', 'b']);
      const result = await module.resolveQueries(mockCli, 'my-qlpack', KeyType.DefinitionQuery);
      expect(result).to.deep.equal(['a', 'b']);
      expect(writeFileSpy.getCall(0).args[0]).to.match(/.qls$/);
      expect(yaml.safeLoad(writeFileSpy.getCall(0).args[1])).to.deep.equal({
        qlpack: 'my-qlpack',
        include: {
          kind: 'definitions',
          'tags contain': 'ide-contextual-queries/local-definitions'
        }
      });
    });

    it('should throw an error when there are no queries found', async () => {
      mockCli.resolveQueriesInSuite.returns([]);

      // TODO: Figure out why chai-as-promised isn't failing the test on an
      // unhandled rejection.
      try {
        await module.resolveQueries(mockCli, 'my-qlpack', KeyType.DefinitionQuery);
        // should reject
        expect(true).to.be.false;
      } catch (e) {
        expect(e.message).to.eq(
          'Couldn\'t find any queries tagged ide-contextual-queries/local-definitions for qlpack my-qlpack'
        );
      }
    });
  });

  describe('qlpackOfDatabase', () => {
    it('should get the qlpack of a database', async () => {
      resolveDatasetFolderSpy.returns({ qlpack: 'my-qlpack' });
      const db = {
        contents: {
          datasetUri: {
            fsPath: '/path/to/database'
          }
        }
      };
      const result = await module.qlpackOfDatabase(mockCli, db);
      expect(result).to.eq('my-qlpack');
      expect(resolveDatasetFolderSpy).to.have.been.calledWith(mockCli, '/path/to/database');
    });
  });

  function createModule() {
    writeFileSpy = sinon.spy();
    resolveDatasetFolderSpy = sinon.stub();
    return proxyquire('../../../contextual/queryResolver', {
      'fs-extra': {
        writeFile: writeFileSpy
      },

      '../helpers': {
        resolveDatasetFolder: resolveDatasetFolderSpy,
        getOnDiskWorkspaceFolders: () => ({}),
        showAndLogErrorMessage: () => ({})
      }
    });
  }
});
