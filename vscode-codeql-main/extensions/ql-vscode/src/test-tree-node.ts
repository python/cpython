import { TestSuiteInfo, TestInfo } from 'vscode-test-adapter-api';

/**
 * Tree view node for a test, suite, or collection. This object is passed as the argument to the
 * command handler of a context menu item for a tree view item. 
 */
export interface TestTreeNode {
  readonly info: TestSuiteInfo | TestInfo;
}
