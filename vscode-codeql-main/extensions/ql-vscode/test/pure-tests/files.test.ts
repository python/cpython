import * as chai from 'chai';
import 'chai/register-should';
import * as sinonChai from 'sinon-chai';
import 'mocha';
import * as path from 'path';

import { gatherQlFiles } from '../../src/files';

chai.use(sinonChai);
const expect = chai.expect;

describe('files', () => {
  const dataDir = path.join(path.dirname(__dirname), 'data');
  const data2Dir = path.join(path.dirname(__dirname), 'data2');

  it('should pass', () => {
    expect(true).to.be.eq(true);
  });
  it('should find one file', async () => {
    const singleFile = path.join(dataDir, 'query.ql');
    const result = await gatherQlFiles([singleFile]);
    expect(result).to.deep.equal([[singleFile], false]);
  });

  it('should find no files', async () => {
    const result = await gatherQlFiles([]);
    expect(result).to.deep.equal([[], false]);
  });

  it('should find no files', async () => {
    const singleFile = path.join(dataDir, 'library.qll');
    const result = await gatherQlFiles([singleFile]);
    expect(result).to.deep.equal([[], false]);
  });

  it('should handle invalid file', async () => {
    const singleFile = path.join(dataDir, 'xxx');
    const result = await gatherQlFiles([singleFile]);
    expect(result).to.deep.equal([[], false]);
  });

  it('should find two files', async () => {
    const singleFile = path.join(dataDir, 'query.ql');
    const otherFile = path.join(dataDir, 'multiple-result-sets.ql');
    const notFile = path.join(dataDir, 'library.qll');
    const invalidFile = path.join(dataDir, 'xxx');

    const result = await gatherQlFiles([singleFile, otherFile, notFile, invalidFile]);
    expect(result.sort()).to.deep.equal([[singleFile, otherFile], false]);
  });

  it('should scan a directory', async () => {
    const singleFile = path.join(dataDir, 'query.ql');
    const otherFile = path.join(dataDir, 'multiple-result-sets.ql');

    const result = await gatherQlFiles([dataDir]);
    expect(result.sort()).to.deep.equal([[otherFile, singleFile], true]);
  });

  it('should scan a directory and some files', async () => {
    const singleFile = path.join(dataDir, 'query.ql');
    const empty1File = path.join(data2Dir, 'empty1.ql');
    const empty2File = path.join(data2Dir, 'sub-folder', 'empty2.ql');

    const result = await gatherQlFiles([singleFile, data2Dir]);
    expect(result.sort()).to.deep.equal([[singleFile, empty1File, empty2File], true]);
  });

  it('should avoid duplicates', async () => {
    const singleFile = path.join(dataDir, 'query.ql');
    const otherFile = path.join(dataDir, 'multiple-result-sets.ql');

    const result = await gatherQlFiles([singleFile, dataDir, otherFile]);
    expect(result.sort()).to.deep.equal([[singleFile, otherFile], true]);
  });
});
