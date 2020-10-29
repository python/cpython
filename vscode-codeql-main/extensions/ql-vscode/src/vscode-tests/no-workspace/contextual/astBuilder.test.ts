import * as fs from 'fs-extra';
import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import * as sinon from 'sinon';

import AstBuilder from '../../../contextual/astBuilder';
import { QueryWithResults } from '../../../run-queries';
import { CodeQLCliServer } from '../../../cli';
import { DatabaseItem } from '../../../databases';

chai.use(chaiAsPromised);
const expect = chai.expect;

/**
 *
This test uses an AST generated from this file (already BQRS-decoded in ../data/astBuilder.json):

#include <common.h>

int interrupt_init(void)
{
	return 0;
}

void enable_interrupts(void)
{
	return;
}
int disable_interrupts(void)
{
	return 0;
}
 */


describe('AstBuilder', () => {
  let mockCli: CodeQLCliServer;
  let overrides: Record<string, object | undefined>;


  beforeEach(() => {
    mockCli = {
      bqrsDecode: sinon.stub().callsFake((_: string, resultSet: 'nodes' | 'edges' | 'graphProperties') => {
        return mockDecode(resultSet);
      })
    } as unknown as CodeQLCliServer;
    overrides = {
      nodes: undefined,
      edges: undefined,
      graphProperties: undefined
    };
  });

  it('should build the AST roots', async () => {
    const astBuilder = createAstBuilder();
    const roots = await astBuilder.getRoots();

    const options = { entities: ['id', 'url', 'string'] };
    expect(mockCli.bqrsDecode).to.have.been.calledWith('/a/b/c', 'nodes', options);
    expect(mockCli.bqrsDecode).to.have.been.calledWith('/a/b/c', 'edges', options);
    expect(mockCli.bqrsDecode).to.have.been.calledWith('/a/b/c', 'graphProperties', options);

    expect(roots.map(
      r => ({ ...r, children: undefined })
    )).to.deep.eq(expectedRoots);
  });

  it('should fail when graphProperties are not correct', async () => {
    overrides.graphProperties = {
      tuples: [
        [
          'semmle.graphKind',
          'hucairz'
        ]
      ]
    };

    const astBuilder = createAstBuilder();
    expect(astBuilder.getRoots()).to.be.rejectedWith('AST is invalid');
  });

  function createAstBuilder() {
    return new AstBuilder({
      query: {
        resultsPaths: {
          resultsPath: '/a/b/c'
        }
      }
    } as QueryWithResults, mockCli, {} as DatabaseItem, '');
  }

  function mockDecode(resultSet: 'nodes' | 'edges' | 'graphProperties') {
    if (overrides[resultSet]) {
      return overrides[resultSet];
    }

    const mapper = {
      nodes: 0,
      edges: 1,
      graphProperties: 2
    };
    const index = mapper[resultSet] as number;
    if (index >= 0 && index <= 2) {
      return JSON.parse(fs.readFileSync(`${__dirname}/../data/astBuilder.json`, 'utf8'))[index];
    } else {
      throw new Error(`Invalid resultSet: ${resultSet}`);
    }
  }
});

const expectedRoots = [
  {
    id: 0,
    label: '[TopLevelFunction] int disable_interrupts()',
    fileLocation: undefined,
    location: {
      uri: 'file:/opt/src/arch/sandbox/lib/interrupts.c',
      startLine: 19,
      startColumn: 5,
      endLine: 19,
      endColumn: 22
    },
    order: 3,
    children: undefined
  },
  {
    id: 26363,
    label: '[TopLevelFunction] void enable_interrupts()',
    fileLocation: undefined,
    location: {
      uri: 'file:/opt/src/arch/sandbox/lib/interrupts.c',
      startLine: 15,
      startColumn: 6,
      endLine: 15,
      endColumn: 22
    },
    order: 2,
    children: undefined
  },
  {
    id: 26364,
    label: '[TopLevelFunction] int interrupt_init()',
    fileLocation: undefined,
    location: {
      uri: 'file:/opt/src/arch/sandbox/lib/interrupts.c',
      startLine: 10,
      startColumn: 5,
      endLine: 10,
      endColumn: 18
    },
    order: 1,
    children: undefined
  }
];
