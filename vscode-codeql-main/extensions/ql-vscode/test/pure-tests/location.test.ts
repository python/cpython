import { expect } from 'chai';
import 'mocha';
import { tryGetResolvableLocation } from '../../src/bqrs-utils';

describe('processing string locations', function() {
  it('should detect Windows whole-file locations', function() {
    const loc = 'file://C:/path/to/file.ext:0:0:0:0';
    const wholeFileLoc = tryGetResolvableLocation(loc);
    expect(wholeFileLoc).to.eql({ uri: 'C:/path/to/file.ext' });
  });
  it('should detect Unix whole-file locations', function() {
    const loc = 'file:///path/to/file.ext:0:0:0:0';
    const wholeFileLoc = tryGetResolvableLocation(loc);
    expect(wholeFileLoc).to.eql({ uri: '/path/to/file.ext' });
  });

  it('should detect Unix 5-part locations', function() {
    const loc = 'file:///path/to/file.ext:1:2:3:4';
    const wholeFileLoc = tryGetResolvableLocation(loc);
    expect(wholeFileLoc).to.eql({
      uri: '/path/to/file.ext',
      startLine: 1,
      startColumn: 2,
      endLine: 3,
      endColumn: 4
    });
  });
  it('should ignore other string locations', function() {
    for (const loc of ['file:///path/to/file.ext', 'I am not a location']) {
      const wholeFileLoc = tryGetResolvableLocation(loc);
      expect(wholeFileLoc).to.be.undefined;
    }
  });
});
