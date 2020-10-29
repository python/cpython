import * as fs from 'fs-extra';
import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import * as sinon from 'sinon';
import * as yaml from 'js-yaml';

import { AstViewer, AstItem } from '../../astViewer';
import { commands, Range } from 'vscode';
import { DatabaseItem } from '../../databases';

chai.use(chaiAsPromised);
const expect = chai.expect;



describe('AstViewer', () => {
  let astRoots: AstItem[];
  let viewer: AstViewer;
  beforeEach(async () => {
    // the ast is stored in yaml because there are back pointers
    // making a json representation impossible.
    // The complication here is that yaml files are not copied into the 'out' directory by tsc.
    astRoots = await buildAst();

    sinon.stub(commands, 'registerCommand');
    sinon.stub(commands, 'executeCommand');
  });

  afterEach(() => {
    sinon.restore();
  });

  it('should update the viewer roots', () => {
    const item = {} as DatabaseItem;
    viewer = new AstViewer();
    viewer.updateRoots(astRoots, item, 'def/abc');

    expect((viewer as any).treeDataProvider.roots).to.eq(astRoots);
    expect((viewer as any).treeDataProvider.db).to.eq(item);
    expect((viewer as any).treeView.message).to.eq('AST for abc');
  });

  it('should update the tree selection based on a change in the editor selection', () => {
    // Should select the namespace
    doSelectionTest(astRoots[0], astRoots[0].fileLocation?.range);
  });

  it('should select an AssignExpr', () => {
    // this one is interesting because it spans a couple of other nodes
    const expr = findNodeById(300, astRoots);
    expect(expr.label).to.eq('[AssignExpr] ... = ...');
    doSelectionTest(expr, expr.fileLocation?.range);
  });

  it('should select nothing', () => {
    doSelectionTest(undefined, new Range(2, 3, 4, 5));
  });


  function doSelectionTest(
    expectedSelection: any,
    selectionRange: Range | undefined,
    fsPath = 'def/abc',
  ) {
    const item = {} as DatabaseItem;
    viewer = new AstViewer();
    viewer.updateRoots(astRoots, item, fsPath);
    const spy = sinon.spy();
    (viewer as any).treeView.reveal = spy;
    Object.defineProperty((viewer as any).treeView, 'visible', {
      value: true
    });

    const mockEvent = createMockEvent(selectionRange, fsPath);
    (viewer as any).updateTreeSelection(mockEvent);
    if (expectedSelection) {
      expect(spy).to.have.been.calledWith(expectedSelection);
    } else {
      expect(spy).not.to.have.been.called;
    }
  }

  function createMockEvent(
    selectionRange: Range | undefined,
    fsPath: string,
  ) {
    return {
      selections: [{
        anchor: selectionRange?.start,
        active: selectionRange?.end
      }],
      textEditor: {
        document: {
          uri: {
            fsPath
          }
        }
      }
    };
  }

  function findNodeById(id: number, ast: any): any {
    if (Array.isArray(ast)) {
      for (const elt of ast) {
        const candidate = findNodeById(id, elt);
        if (candidate) {
          return candidate;
        }
      }

    } else if (typeof ast === 'object' && ast) {
      if (ast.id === id) {
        return ast;
      } else {
        for (const [name, prop] of Object.entries(ast)) {
          if (name !== 'parent') {
            const candidate = findNodeById(id, prop);
            if (candidate) {
              return candidate;
            }
          }
        }
      }
    }
  }

  async function buildAst() {
    const astRoots = yaml.safeLoad(await fs.readFile(`${__dirname}/data/astViewer.yml`, 'utf8')) as AstItem[];

    // convert range properties into vscode.Range instances
    function convertToRangeInstances(obj: any) {
      if (Array.isArray(obj)) {
        obj.forEach(elt => convertToRangeInstances(elt));
      } else if (typeof obj === 'object' && obj) {
        if ('range' in obj && '_start' in obj.range && '_end' in obj.range) {
          obj.range = new Range(
            obj.range._start._line,
            obj.range._start._character,
            obj.range._end._line,
            obj.range._end._character,
          );
        } else {
          Object.entries(obj).forEach(([name, prop]) => name !== 'parent' && convertToRangeInstances(prop));
        }
      }
    }
    convertToRangeInstances(astRoots);
    return astRoots;
  }
});
