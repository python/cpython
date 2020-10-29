import 'vscode-test';
import 'mocha';
import * as sinon from 'sinon';
import { expect } from 'chai';
import { ExtensionContext, Uri } from 'vscode';

import {
  DatabaseEventKind,
  DatabaseItem,
  DatabaseManager,
  DatabaseItemImpl,
  DatabaseContents
} from '../../databases';
import { QueryServerConfig } from '../../config';
import { Logger } from '../../logging';
import { encodeSourceArchiveUri } from '../../archive-filesystem-provider';

describe('databases', () => {
  let databaseManager: DatabaseManager;
  let updateSpy: sinon.SinonSpy;

  beforeEach(() => {
    updateSpy = sinon.spy();
    databaseManager = new DatabaseManager(
      {
        workspaceState: {
          update: updateSpy
        }
      } as unknown as ExtensionContext,
      {} as QueryServerConfig,
      {} as Logger,
    );
  });

  it('should fire events when adding and removing a db item', () => {
    const mockDbItem = {
      databaseUri: { path: 'file:/abc' },
      name: 'abc',
      getPersistedState() {
        return this.name;
      }
    };
    const spy = sinon.spy();
    databaseManager.onDidChangeDatabaseItem(spy);
    (databaseManager as any).addDatabaseItem(mockDbItem);

    expect((databaseManager as any)._databaseItems).to.deep.eq([mockDbItem]);
    expect(updateSpy).to.have.been.calledWith('databaseList', ['abc']);
    expect(spy).to.have.been.calledWith({
      item: undefined,
      kind: DatabaseEventKind.Add
    });

    sinon.reset();

    // now remove the item
    databaseManager.removeDatabaseItem(mockDbItem as unknown as DatabaseItem);
    expect((databaseManager as any)._databaseItems).to.deep.eq([]);
    expect(updateSpy).to.have.been.calledWith('databaseList', []);
    expect(spy).to.have.been.calledWith({
      item: undefined,
      kind: DatabaseEventKind.Remove
    });
  });

  it('should rename a db item and emit an event', () => {
    const mockDbItem = {
      databaseUri: 'file:/abc',
      name: 'abc',
      getPersistedState() {
        return this.name;
      }
    };
    const spy = sinon.spy();
    databaseManager.onDidChangeDatabaseItem(spy);
    (databaseManager as any).addDatabaseItem(mockDbItem);
    sinon.restore();

    databaseManager.renameDatabaseItem(mockDbItem as unknown as DatabaseItem, 'new name');

    expect(mockDbItem.name).to.eq('new name');
    expect(updateSpy).to.have.been.calledWith('databaseList', ['new name']);
    expect(spy).to.have.been.calledWith({
      item: mockDbItem,
      kind: DatabaseEventKind.Rename
    });
  });

  describe('resolveSourceFile', () => {
    describe('unzipped source archive', () => {
      it('should resolve a source file in an unzipped database', () => {
        const db = createMockDB();
        const resolved = db.resolveSourceFile('abc');
        expect(resolved.toString()).to.eq('file:///sourceArchive-uri/abc');
      });

      it('should resolve a source file in an unzipped database with trailing slash', () => {
        const db = createMockDB(Uri.parse('file:/sourceArchive-uri/'));
        const resolved = db.resolveSourceFile('abc');
        expect(resolved.toString()).to.eq('file:///sourceArchive-uri/abc');
      });

      it('should resolve a source uri in an unzipped database with trailing slash', () => {
        const db = createMockDB(Uri.parse('file:/sourceArchive-uri/'));
        const resolved = db.resolveSourceFile('file:/abc');
        expect(resolved.toString()).to.eq('file:///sourceArchive-uri/abc');
      });
    });

    describe('no source archive', () => {
      it('should resolve a file', () => {
        const db = createMockDB(Uri.parse('file:/sourceArchive-uri/'));
        (db as any)._contents.sourceArchiveUri = undefined;
        const resolved = db.resolveSourceFile('abc');
        expect(resolved.toString()).to.eq('file:///abc');
      });

      it('should resolve an empty file', () => {
        const db = createMockDB(Uri.parse('file:/sourceArchive-uri/'));
        (db as any)._contents.sourceArchiveUri = undefined;
        const resolved = db.resolveSourceFile('file:');
        expect(resolved.toString()).to.eq('file:///database-uri');
      });
    });

    describe('zipped source archive', () => {
      it('should encode a source archive url', () => {
        const db = createMockDB(encodeSourceArchiveUri({
          sourceArchiveZipPath: 'sourceArchive-uri',
          pathWithinSourceArchive: 'def'
        }));
        const resolved = db.resolveSourceFile(Uri.file('abc').toString());

        // must recreate an encoded archive uri instead of typing out the
        // text since the uris will be different on windows and ubuntu.
        expect(resolved.toString()).to.eq(encodeSourceArchiveUri({
          sourceArchiveZipPath: 'sourceArchive-uri',
          pathWithinSourceArchive: 'def/abc'
        }).toString());
      });

      it('should encode a source archive url with trailing slash', () => {
        const db = createMockDB(encodeSourceArchiveUri({
          sourceArchiveZipPath: 'sourceArchive-uri',
          pathWithinSourceArchive: 'def/'
        }));
        const resolved = db.resolveSourceFile(Uri.file('abc').toString());

        // must recreate an encoded archive uri instead of typing out the
        // text since the uris will be different on windows and ubuntu.
        expect(resolved.toString()).to.eq(encodeSourceArchiveUri({
          sourceArchiveZipPath: 'sourceArchive-uri',
          pathWithinSourceArchive: 'def/abc'
        }).toString());
      });

      it('should encode an empty source archive url', () => {
        const db = createMockDB(encodeSourceArchiveUri({
          sourceArchiveZipPath: 'sourceArchive-uri',
          pathWithinSourceArchive: 'def'
        }));
        const resolved = db.resolveSourceFile('file:');
        expect(resolved.toString()).to.eq('codeql-zip-archive://1-18/sourceArchive-uri/def');
      });
    });

    it('should handle an empty file', () => {
      const db = createMockDB(Uri.parse('file:/sourceArchive-uri/'));
      const resolved = db.resolveSourceFile('');
      expect(resolved.toString()).to.eq('file:///sourceArchive-uri/');
    });
    function createMockDB(
      sourceArchiveUri = Uri.parse('file:/sourceArchive-uri'),
      databaseUri = Uri.parse('file:/database-uri')
    ) {
      return new DatabaseItemImpl(
        databaseUri,
        {
          sourceArchiveUri
        } as DatabaseContents,
        {
          dateAdded: 123,
          ignoreSourceArchive: false
        },
        () => { /**/ }
      );
    }
  });
});
