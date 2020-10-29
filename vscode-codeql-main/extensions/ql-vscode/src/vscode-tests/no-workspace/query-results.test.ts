import * as chai from 'chai';
import * as path from 'path';
import * as fs from 'fs-extra';
import 'mocha';
import 'sinon-chai';
import * as Sinon from 'sinon';
import * as chaiAsPromised from 'chai-as-promised';
import { CompletedQuery, interpretResults } from '../../query-results';
import { QueryInfo, QueryWithResults, tmpDir } from '../../run-queries';
import { QueryHistoryConfig } from '../../config';
import { EvaluationResult, QueryResultType } from '../../messages';
import { SortDirection, SortedResultSetInfo } from '../../interface-types';
import { CodeQLCliServer, SourceInfo } from '../../cli';

chai.use(chaiAsPromised);
const expect = chai.expect;

describe('CompletedQuery', () => {
  let disposeSpy: Sinon.SinonSpy;
  let onDidChangeQueryHistoryConfigurationSpy: Sinon.SinonSpy;

  beforeEach(() => {
    disposeSpy = Sinon.spy();
    onDidChangeQueryHistoryConfigurationSpy = Sinon.spy();
  });

  it('should construct a CompletedQuery', () => {
    const completedQuery = mockCompletedQuery();

    expect(completedQuery.logFileLocation).to.eq('mno');
    expect(completedQuery.databaseName).to.eq('def');
  });

  it('should get the query name', () => {
    const completedQuery = mockCompletedQuery();

    // from the query path
    expect(completedQuery.queryName).to.eq('stu');

    // from the metadata
    (completedQuery.query as any).metadata = {
      name: 'vwx'
    };
    expect(completedQuery.queryName).to.eq('vwx');

    // from quick eval position
    (completedQuery.query as any).quickEvalPosition = {
      line: 1,
      endLine: 2,
      fileName: '/home/users/yz'
    };
    expect(completedQuery.queryName).to.eq('Quick evaluation of yz:1-2');
    (completedQuery.query as any).quickEvalPosition.endLine = 1;
    expect(completedQuery.queryName).to.eq('Quick evaluation of yz:1');
  });

  it('should get the label', () => {
    const completedQuery = mockCompletedQuery();
    expect(completedQuery.getLabel()).to.eq('ghi');
    completedQuery.options.label = '';
    expect(completedQuery.getLabel()).to.eq('pqr');
  });

  it('should get the getResultsPath', () => {
    const completedQuery = mockCompletedQuery();
    // from results path
    expect(completedQuery.getResultsPath('zxa', false)).to.eq('axa');

    completedQuery.sortedResultsInfo.set('zxa', {
      resultsPath: 'bxa'
    } as SortedResultSetInfo);

    // still from results path
    expect(completedQuery.getResultsPath('zxa', false)).to.eq('axa');

    // from sortedResultsInfo
    expect(completedQuery.getResultsPath('zxa')).to.eq('bxa');
  });

  it('should get the statusString', () => {
    const completedQuery = mockCompletedQuery();
    expect(completedQuery.statusString).to.eq('failed');

    completedQuery.result.message = 'Tremendously';
    expect(completedQuery.statusString).to.eq('failed: Tremendously');

    completedQuery.result.resultType = QueryResultType.OTHER_ERROR;
    expect(completedQuery.statusString).to.eq('failed: Tremendously');

    completedQuery.result.resultType = QueryResultType.CANCELLATION;
    completedQuery.result.evaluationTime = 2000;
    expect(completedQuery.statusString).to.eq('cancelled after 2 seconds');

    completedQuery.result.resultType = QueryResultType.OOM;
    expect(completedQuery.statusString).to.eq('out of memory');

    completedQuery.result.resultType = QueryResultType.SUCCESS;
    expect(completedQuery.statusString).to.eq('finished in 2 seconds');

    completedQuery.result.resultType = QueryResultType.TIMEOUT;
    expect(completedQuery.statusString).to.eq('timed out after 2 seconds');

  });

  it('should updateSortState', async () => {
    const completedQuery = mockCompletedQuery();
    const spy = Sinon.spy();
    const mockServer = {
      sortBqrs: spy
    } as unknown as CodeQLCliServer;
    const sortState = {
      columnIndex: 1,
      sortDirection: SortDirection.desc
    };
    await completedQuery.updateSortState(mockServer, 'result-name', sortState);
    const expectedPath = path.join(tmpDir.name, 'sortedResults111-result-name.bqrs');
    expect(spy).to.have.been.calledWith(
      'axa',
      expectedPath,
      'result-name',
      [sortState.columnIndex],
      [sortState.sortDirection],
    );

    expect(completedQuery.sortedResultsInfo.get('result-name')).to.deep.equal({
      resultsPath: expectedPath,
      sortState
    });

    // delete the sort stae
    await completedQuery.updateSortState(mockServer, 'result-name');
    expect(completedQuery.sortedResultsInfo.size).to.eq(0);
  });

  it('should interpolate', () => {
    const completedQuery = mockCompletedQuery();
    (completedQuery as any).time = '123';
    expect(completedQuery.interpolate('xxx')).to.eq('xxx');
    expect(completedQuery.interpolate('%t %q %d %s %%')).to.eq('123 stu def failed %');
    expect(completedQuery.interpolate('%t %q %d %s %%::%t %q %d %s %%')).to.eq('123 stu def failed %::123 stu def failed %');
  });

  it('should interpretResults', async () => {
    const spy = Sinon.mock();
    spy.returns('1234');
    const mockServer = {
      interpretBqrs: spy
    } as unknown as CodeQLCliServer;

    const interpretedResultsPath = path.join(tmpDir.name, 'interpreted.json');
    const resultsPath = '123';
    const sourceInfo = {};
    const metadata = {
      kind: 'my-kind',
      id: 'my-id' as string | undefined
    };
    const results1 = await interpretResults(
      mockServer,
      metadata,
      {
        resultsPath, interpretedResultsPath
      },
      sourceInfo as SourceInfo
    );

    expect(results1).to.eq('1234');
    expect(spy).to.have.been.calledWith(
      metadata,
      resultsPath, interpretedResultsPath, sourceInfo
    );

    // Try again, but with no id
    spy.reset();
    spy.returns('1234');
    delete metadata.id;
    const results2 = await interpretResults(
      mockServer,
      metadata,
      {
        resultsPath, interpretedResultsPath
      },
      sourceInfo as SourceInfo
    );
    expect(results2).to.eq('1234');
    expect(spy).to.have.been.calledWith(
      { kind: 'my-kind', id: 'dummy-id' },
      resultsPath, interpretedResultsPath, sourceInfo
    );

    // try a third time, but this time we get from file
    spy.reset();
    fs.writeFileSync(interpretedResultsPath, JSON.stringify({
      a: 6
    }), 'utf8');
    const results3 = await interpretResults(
      mockServer,
      metadata,
      {
        resultsPath, interpretedResultsPath
      },
      sourceInfo as SourceInfo
    );
    expect(results3).to.deep.eq({ a: 6 });
  });

  function mockCompletedQuery() {
    return  new CompletedQuery(
      mockQueryWithResults(),
      mockQueryHistoryConfig()
    );
  }

  function mockQueryWithResults(): QueryWithResults {
    return {
      query: {
        program: {
          queryPath: 'stu'
        },
        resultsPaths: {
          resultsPath: 'axa'
        },
        queryID: 111
      } as never as QueryInfo,
      result: {} as never as EvaluationResult,
      database: {
        databaseUri: 'abc',
        name: 'def'
      },
      options: {
        label: 'ghi',
        queryText: 'jkl',
        isQuickQuery: false
      },
      logFileLocation: 'mno',
      dispose: disposeSpy
    };
  }

  function mockQueryHistoryConfig(): QueryHistoryConfig {
    return {
      onDidChangeConfiguration: onDidChangeQueryHistoryConfigurationSpy,
      format: 'pqr'
    };
  }
});
