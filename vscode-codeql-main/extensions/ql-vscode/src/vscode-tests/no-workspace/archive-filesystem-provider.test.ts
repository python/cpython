import { expect } from 'chai';
import * as path from 'path';

import {
  encodeSourceArchiveUri,
  encodeArchiveBasePath,
  ArchiveFileSystemProvider,
  decodeSourceArchiveUri,
  ZipFileReference,
  zipArchiveScheme
} from '../../archive-filesystem-provider';
import { FileType, FileSystemError, Uri } from 'vscode';

describe('archive-filesystem-provider', () => {
  it('reads empty file correctly', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/single_file.zip'),
      pathWithinSourceArchive: '/aFileName.txt'
    });
    const data = await archiveProvider.readFile(uri);
    expect(data.length).to.equal(0);
  });

  it('read non-empty file correctly', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1/textFile.txt'
    });
    const data = await archiveProvider.readFile(uri);
    expect(Buffer.from(data).toString('utf8')).to.be.equal('I am a text\n');
  });

  it('read a directory', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1'
    });
    const files = await archiveProvider.readDirectory(uri);
    expect(files).to.be.deep.equal([
      ['folder2', FileType.Directory],
      ['textFile.txt', FileType.File],
      ['textFile2.txt', FileType.File],
    ]);
  });

  it('should handle a missing directory', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1/not-here'
    });
    try {
      await archiveProvider.readDirectory(uri);
      throw new Error('Failed');
    } catch (e) {
      expect(e).to.be.instanceOf(FileSystemError);
    }
  });

  it('should handle a missing file', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1/not-here'
    });
    try {
      await archiveProvider.readFile(uri);
      throw new Error('Failed');
    } catch (e) {
      expect(e).to.be.instanceOf(FileSystemError);
    }
  });

  it('should handle reading a file as a directory', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1/textFile.txt'
    });
    try {
      await archiveProvider.readDirectory(uri);
      throw new Error('Failed');
    } catch (e) {
      expect(e).to.be.instanceOf(FileSystemError);
    }
  });

  it('should handle reading a directory as a file', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1/folder2'
    });
    try {
      await archiveProvider.readFile(uri);
      throw new Error('Failed');
    } catch (e) {
      expect(e).to.be.instanceOf(FileSystemError);
    }
  });

  it('read a nested directory', async () => {
    const archiveProvider = new ArchiveFileSystemProvider();
    const uri = encodeSourceArchiveUri({
      sourceArchiveZipPath: path.resolve(__dirname, 'data/archive-filesystem-provider-test/zip_with_folder.zip'),
      pathWithinSourceArchive: 'folder1/folder2'
    });
    const files = await archiveProvider.readDirectory(uri);
    expect(files).to.be.deep.equal([
      ['textFile3.txt', FileType.File],
    ]);
  });
});

describe('source archive uri encoding', function() {
  const testCases: { name: string; input: ZipFileReference }[] = [
    {
      name: 'mixed case and unicode',
      input: {
        sourceArchiveZipPath: '/I-\u2665-codeql.zip',
        pathWithinSourceArchive: '/foo/bar'
      }
    },
    {
      name: 'Windows path',
      input: {
        sourceArchiveZipPath: 'C:/Users/My Name/folder/src.zip',
        pathWithinSourceArchive: '/foo/bar.ext'
      }
    },
    {
      name: 'Unix path',
      input: {
        sourceArchiveZipPath: '/home/folder/src.zip',
        pathWithinSourceArchive: '/foo/bar.ext'
      }
    },
    {
      name: 'Empty path',
      input: {
        sourceArchiveZipPath: '/home/folder/src.zip',
        pathWithinSourceArchive: '/'
      }
    }
  ];
  for (const testCase of testCases) {
    it(`should work round trip with ${testCase.name}`, function() {
      const output = decodeSourceArchiveUri(encodeSourceArchiveUri(testCase.input));
      expect(output).to.eql(testCase.input);
    });
  }

  it('should decode an empty path as a "/"', () => {
    const uri = encodeSourceArchiveUri({
      pathWithinSourceArchive: '',
      sourceArchiveZipPath: 'a/b/c'
    });
    expect(decodeSourceArchiveUri(uri)).to.deep.eq({
      pathWithinSourceArchive: '/',
      sourceArchiveZipPath: 'a/b/c'
    });
  });

  it('should encode a uri at the root of the archive', () => {
    const path = '/a/b/c/src.zip';
    const uri = encodeArchiveBasePath(path);
    expect(uri.path).to.eq(path);
    expect(decodeSourceArchiveUri(uri).pathWithinSourceArchive).to.eq('/');
    expect(decodeSourceArchiveUri(uri).sourceArchiveZipPath).to.eq(path);
    expect(uri.authority).to.eq('0-14');
  });

  it('should handle malformed uri with no authority', () => {
    // This handles codeql-zip-archive uris generated using the `with` method
    const uri = Uri.parse('file:/a/b/c/src.zip').with({ scheme: zipArchiveScheme });
    expect(uri.authority).to.eq('');
    expect(decodeSourceArchiveUri(uri)).to.deep.eq({
      sourceArchiveZipPath: '/a/b/c/src.zip',
      pathWithinSourceArchive: '/'
    });
  });
});
