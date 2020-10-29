import 'vscode-test';
import 'mocha';
import * as chaiAsPromised from 'chai-as-promised';
import 'sinon-chai';
import * as Sinon from 'sinon';
import * as chai from 'chai';
import { workspace } from 'vscode';

import {
  CliConfigListener,
  QueryHistoryConfigListener,
  QueryServerConfigListener
} from '../../config';

chai.use(chaiAsPromised);
const expect = chai.expect;

describe('config listeners', () => {
  let sandbox: Sinon.SinonSandbox;
  beforeEach(() => {
    sandbox = Sinon.createSandbox();
  });

  afterEach(() => {
    sandbox.restore();
  });

  interface TestConfig<T> {
    clazz: new() => {};
    settings: {
      name: string;
      property: string;
      values: [T, T];
    }[];
  }

  const testConfig: TestConfig<string | number | boolean>[] = [
    {
      clazz: CliConfigListener,
      settings: [{
        name: 'codeQL.runningTests.numberOfThreads',
        property: 'numberTestThreads',
        values: [1, 0]
      }]
    },
    {
      clazz: QueryHistoryConfigListener,
      settings: [{
        name: 'codeQL.queryHistory.format',
        property: 'format',
        values: ['abc', 'def']
      }]
    },
    {
      clazz: QueryServerConfigListener,
      settings: [{
        name: 'codeQL.runningQueries.numberOfThreads',
        property: 'numThreads',
        values: [0, 1]
      }, {
        name: 'codeQL.runningQueries.memory',
        property: 'queryMemoryMb',
        values: [0, 1]
      }, {
        name: 'codeQL.runningQueries.debug',
        property: 'debug',
        values: [true, false]
      }]
    }
  ];

  testConfig.forEach(config => {
    describe(config.clazz.name, () => {
      let listener: any;
      let spy: Sinon.SinonSpy;
      beforeEach(() => {
        listener = new config.clazz();
        spy = Sinon.spy();
        listener.onDidChangeConfiguration(spy);
      });

      config.settings.forEach(setting => {
        let origValue: any;
        beforeEach(async () => {
          origValue = workspace.getConfiguration().get(setting.name);
          await workspace.getConfiguration().update(setting.name, setting.values[0]);
          spy.resetHistory();
        });

        afterEach(async () => {
          await workspace.getConfiguration().update(setting.name, origValue);
        });

        it(`should listen for changes to '${setting.name}'`, async () => {
          await workspace.getConfiguration().update(setting.name, setting.values[1]);
          expect(spy.calledOnce).to.be.true;
          expect(listener[setting.property]).to.eq(setting.values[1]);
        });
      });
    });
  });
});
