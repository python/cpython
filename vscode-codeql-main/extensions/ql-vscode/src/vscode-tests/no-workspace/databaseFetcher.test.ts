import 'vscode-test';
import 'mocha';
import * as chaiAsPromised from 'chai-as-promised';
import * as sinon from 'sinon';
import * as path from 'path';
import * as fs from 'fs-extra';
import * as tmp from 'tmp';
import * as chai from 'chai';
import { window } from 'vscode';

import {
  convertToDatabaseUrl,
  looksLikeLgtmUrl,
  findDirWithFile,
} from '../../databaseFetcher';
chai.use(chaiAsPromised);
const expect = chai.expect;

describe('databaseFetcher', function() {
  // These tests make API calls and may need extra time to complete.
  this.timeout(10000);

  describe('convertToDatabaseUrl', () => {
    let quickPickSpy: sinon.SinonStub;
    beforeEach(() => {
      quickPickSpy = sinon.stub(window, 'showQuickPick');
    });

    afterEach(() => {
      (window.showQuickPick as sinon.SinonStub).restore();
    });

    it('should convert a project url to a database url', async () => {
      quickPickSpy.returns('javascript' as any);
      const lgtmUrl = 'https://lgtm.com/projects/g/github/codeql';
      const dbUrl = await convertToDatabaseUrl(lgtmUrl);

      expect(dbUrl).to.equal(
        'https://lgtm.com/api/v1.0/snapshots/1506465042581/javascript'
      );
      expect(quickPickSpy.firstCall.args[0]).to.contain('javascript');
      expect(quickPickSpy.firstCall.args[0]).to.contain('python');
    });

    it('should convert a project url to a database url with extra path segments', async () => {
      quickPickSpy.returns('python' as any);
      const lgtmUrl =
        'https://lgtm.com/projects/g/github/codeql/subpage/subpage2?query=xxx';
      const dbUrl = await convertToDatabaseUrl(lgtmUrl);

      expect(dbUrl).to.equal(
        'https://lgtm.com/api/v1.0/snapshots/1506465042581/python'
      );
    });

    it('should fail on a nonexistant prohect', async () => {
      quickPickSpy.returns('javascript' as any);
      const lgtmUrl = 'https://lgtm.com/projects/g/github/hucairz';
      expect(convertToDatabaseUrl(lgtmUrl)).to.rejectedWith(/Invalid LGTM URL/);
    });
  });

  describe('looksLikeLgtmUrl', () => {
    it('should handle invalid urls', () => {
      expect(looksLikeLgtmUrl('')).to.be.false;
      expect(looksLikeLgtmUrl('http://lgtm.com/projects/g/github/codeql')).to.be
        .false;
      expect(looksLikeLgtmUrl('https://ww.lgtm.com/projects/g/github/codeql'))
        .to.be.false;
      expect(looksLikeLgtmUrl('https://ww.lgtm.com/projects/g/github')).to.be
        .false;
    });

    it('should handle valid urls', () => {
      expect(looksLikeLgtmUrl('https://lgtm.com/projects/g/github/codeql')).to
        .be.true;
      expect(looksLikeLgtmUrl('https://www.lgtm.com/projects/g/github/codeql'))
        .to.be.true;
      expect(
        looksLikeLgtmUrl('https://lgtm.com/projects/g/github/codeql/sub/pages')
      ).to.be.true;
      expect(
        looksLikeLgtmUrl(
          'https://lgtm.com/projects/g/github/codeql/sub/pages?query=string'
        )
      ).to.be.true;
    });
  });

  describe('findDirWithFile', () => {
    let dir: tmp.DirResult;
    beforeEach(() => {
      dir = tmp.dirSync({ unsafeCleanup: true });
      createFile('a');
      createFile('b');
      createFile('c');

      createDir('dir1');
      createFile('dir1', 'd');
      createFile('dir1', 'e');
      createFile('dir1', 'f');

      createDir('dir2');
      createFile('dir2', 'g');
      createFile('dir2', 'h');
      createFile('dir2', 'i');

      createDir('dir2', 'dir3');
      createFile('dir2', 'dir3', 'j');
      createFile('dir2', 'dir3', 'k');
      createFile('dir2', 'dir3', 'l');
    });

    it('should find files', async () => {
      expect(await findDirWithFile(dir.name, 'k')).to.equal(
        path.join(dir.name, 'dir2', 'dir3')
      );
      expect(await findDirWithFile(dir.name, 'h')).to.equal(
        path.join(dir.name, 'dir2')
      );
      expect(await findDirWithFile(dir.name, 'z', 'a')).to.equal(dir.name);
      // there's some slight indeterminism when more than one name exists
      // but in general, this will find files in the current directory before
      // finding files in sub-dirs
      expect(await findDirWithFile(dir.name, 'k', 'a')).to.equal(dir.name);
    });


    it('should not find files', async () => {
      expect(await findDirWithFile(dir.name, 'x', 'y', 'z')).to.be.undefined;
    });


    function createFile(...segments: string[]) {
      fs.createFileSync(path.join(dir.name, ...segments));
    }

    function createDir(...segments: string[]) {
      fs.mkdirSync(path.join(dir.name, ...segments));
    }
  });
});
