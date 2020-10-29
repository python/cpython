import 'chai/register-should';
import * as chai from 'chai';
import * as fs from 'fs-extra';
import * as path from 'path';
import * as tmp from 'tmp';
import 'mocha';
import * as sinonChai from 'sinon-chai';
import * as sinon from 'sinon';
import * as pq from 'proxyquire';

const proxyquire = pq.noPreserveCache().noCallThru();
chai.use(sinonChai);
const expect = chai.expect;

describe('OutputChannelLogger tests', () => {
  let OutputChannelLogger;
  const tempFolders: Record<string, tmp.DirResult> = {};
  let logger: any;
  let mockOutputChannel: Record<string, sinon.SinonStub>;

  beforeEach(async () => {
    OutputChannelLogger = createModule().OutputChannelLogger;
    tempFolders.globalStoragePath = tmp.dirSync({ prefix: 'logging-tests-global' });
    tempFolders.storagePath = tmp.dirSync({ prefix: 'logging-tests-workspace' });
    logger = new OutputChannelLogger('test-logger');
  });

  afterEach(() => {
    tempFolders.globalStoragePath.removeCallback();
    tempFolders.storagePath.removeCallback();
  });

  it('should log to the output channel', async () => {
    await logger.log('xxx');
    expect(mockOutputChannel.appendLine).to.have.been.calledWith('xxx');
    expect(mockOutputChannel.append).not.to.have.been.calledWith('xxx');

    await logger.log('yyy', { trailingNewline: false });
    expect(mockOutputChannel.appendLine).not.to.have.been.calledWith('yyy');
    expect(mockOutputChannel.append).to.have.been.calledWith('yyy');

    // additionalLogLocation ignored since not initialized
    await logger.log('zzz', { additionalLogLocation: 'hucairz' });

    // should not have created any side logs
    expect(fs.readdirSync(tempFolders.globalStoragePath.name).length).to.equal(0);
    expect(fs.readdirSync(tempFolders.storagePath.name).length).to.equal(0);
  });

  it('should create a side log in the workspace area', async () => {
    logger.init(tempFolders.storagePath.name);

    await logger.log('xxx', { additionalLogLocation: 'first' });
    await logger.log('yyy', { additionalLogLocation: 'second' });
    await logger.log('zzz', { additionalLogLocation: 'first', trailingNewline: false });
    await logger.log('aaa');

    // expect 2 side logs
    const testLoggerFolder = path.join(tempFolders.storagePath.name, 'test-logger');
    expect(fs.readdirSync(testLoggerFolder).length).to.equal(2);

    // contents
    expect(fs.readFileSync(path.join(testLoggerFolder, 'first'), 'utf8')).to.equal('xxx\nzzz');
    expect(fs.readFileSync(path.join(testLoggerFolder, 'second'), 'utf8')).to.equal('yyy\n');
  });

  it('should delete side logs on dispose', async () => {
    logger.init(tempFolders.storagePath.name);
    await logger.log('xxx', { additionalLogLocation: 'first' });
    await logger.log('yyy', { additionalLogLocation: 'second' });

    const testLoggerFolder = path.join(tempFolders.storagePath.name, 'test-logger');
    expect(fs.readdirSync(testLoggerFolder).length).to.equal(2);

    await logger.dispose();
    // need to wait for disposable-object to dispose
    await waitABit();
    expect(fs.readdirSync(testLoggerFolder).length).to.equal(0);
    expect(mockOutputChannel.dispose).to.have.been.calledWith();
  });

  it('should remove an additional log location', async () => {
    logger.init(tempFolders.storagePath.name);
    await logger.log('xxx', { additionalLogLocation: 'first' });
    await logger.log('yyy', { additionalLogLocation: 'second' });

    const testLoggerFolder = path.join(tempFolders.storagePath.name, 'test-logger');
    expect(fs.readdirSync(testLoggerFolder).length).to.equal(2);

    await logger.removeAdditionalLogLocation('first');
    // need to wait for disposable-object to dispose
    await waitABit();
    expect(fs.readdirSync(testLoggerFolder).length).to.equal(1);
    expect(fs.readFileSync(path.join(testLoggerFolder, 'second'), 'utf8')).to.equal('yyy\n');
  });

  it('should delete an existing folder on init', async () => {
    fs.createFileSync(path.join(tempFolders.storagePath.name, 'test-logger', 'xxx'));
    logger.init(tempFolders.storagePath.name);
    // should be empty dir

    const testLoggerFolder = path.join(tempFolders.storagePath.name, 'test-logger');
    expect(fs.readdirSync(testLoggerFolder).length).to.equal(1);
  });

  it('should show the output channel', () => {
    logger.show(true);
    expect(mockOutputChannel.show).to.have.been.calledWith(true);
  });

  function createModule(): any {
    mockOutputChannel = {
      append: sinon.stub(),
      appendLine: sinon.stub(),
      show: sinon.stub(),
      dispose: sinon.stub(),
    };

    return proxyquire('../../src/logging', {
      vscode: {
        window: {
          createOutputChannel: () => mockOutputChannel
        },
        Disposable: function() {
          /**/
        },
        '@noCallThru': true,
        '@global': true
      }
    });
  }

  function  waitABit(ms = 50): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
});
