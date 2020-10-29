import 'vscode-test';
import 'mocha';
import { expect } from 'chai';
import { Uri, Range } from 'vscode';

import fileRangeFromURI from '../../../contextual/fileRangeFromURI';
import { DatabaseItem } from '../../../databases';
import { WholeFileLocation, LineColumnLocation } from '../../../bqrs-cli-types';

describe('fileRangeFromURI', () => {
  it('should return undefined when value is a string', () => {
    expect(fileRangeFromURI('hucairz', createMockDatabaseItem())).to.be.undefined;
  });

  it('should return undefined when value is an empty uri', () => {
    expect(fileRangeFromURI({
      uri: 'file:/',
      startLine: 1,
      startColumn: 2,
      endLine: 3,
      endColumn: 4,
    } as LineColumnLocation, createMockDatabaseItem())).to.be.undefined;
  });

  it('should return a range for a WholeFileLocation', () => {
    expect(fileRangeFromURI({
      uri: 'file:///hucairz',
    } as WholeFileLocation, createMockDatabaseItem())).to.deep.eq({
      uri: Uri.parse('file:///hucairz', true),
      range: new Range(0, 0, 0, 0)
    });
  });

  it('should return a range for a LineColumnLocation', () => {
    expect(fileRangeFromURI({
      uri: 'file:///hucairz',
      startLine: 1,
      startColumn: 2,
      endLine: 3,
      endColumn: 4,
    } as LineColumnLocation, createMockDatabaseItem())).to.deep.eq({
      uri: Uri.parse('file:///hucairz', true),
      range: new Range(0, 1, 2, 4)
    });
  });

  function createMockDatabaseItem(): DatabaseItem {
    return {
      resolveSourceFile: (file: string) => Uri.file(file)
    } as DatabaseItem;
  }
});
