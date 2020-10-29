import { DisposableObject } from './disposable-object';
import { EventEmitter, Event, Uri, GlobPattern, workspace } from 'vscode';

/**
 * A collection of `FileSystemWatcher` objects. Disposing this object disposes all of the individual
 * `FileSystemWatcher` objects and their event registrations.
 */
class WatcherCollection extends DisposableObject {
  constructor() {
    super();
  }

  /**
   * Create a `FileSystemWatcher` and add it to the collection.
   * @param pattern The pattern to watch.
   * @param listener The event listener to be invoked when a watched file is created, changed, or
   *   deleted.
   * @param thisArgs The `this` argument for the event listener.
   */
  public addWatcher(pattern: GlobPattern, listener: (e: Uri) => any, thisArgs: any): void {
    const watcher = workspace.createFileSystemWatcher(pattern);
    this.push(watcher.onDidCreate(listener, thisArgs));
    this.push(watcher.onDidChange(listener, thisArgs));
    this.push(watcher.onDidDelete(listener, thisArgs));
  }
}

/**
 * A class to watch multiple patterns in the file system at the same time, reporting all
 * notifications via a single event.
 */
export class MultiFileSystemWatcher extends DisposableObject {
  private readonly _onDidChange = this.push(new EventEmitter<Uri>());
  private watchers = this.track(new WatcherCollection());

  constructor() {
    super();
  }

  /**
   * Event to be fired when any watched file is created, changed, or deleted.
   */
  public get onDidChange(): Event<Uri> { return this._onDidChange.event; }

  /**
   * Adds a new pattern to watch.
   * @param pattern The pattern to watch.
   */
  public addWatch(pattern: GlobPattern): void {
    this.watchers.addWatcher(pattern, this.handleDidChange, this);
  }

  /**
   * Deletes all existing watchers.
   */
  public clear(): void {
    this.disposeAndStopTracking(this.watchers);
    this.watchers = this.track(new WatcherCollection());
  }

  private handleDidChange(uri: Uri): void {
    this._onDidChange.fire(uri);
  }
}

