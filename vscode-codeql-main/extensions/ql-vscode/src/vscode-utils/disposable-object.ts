import { Disposable } from 'vscode';

/**
 * Base class to make it easier to implement a `Disposable` that owns other disposable object.
 */
export abstract class DisposableObject implements Disposable {
  private disposables: Disposable[] = [];
  private tracked?: Set<Disposable> = undefined;

  /**
   * Adds `obj` to a list of objects to dispose when `this` is disposed. Objects added by `push` are
   * disposed in reverse order of being added.
   * @param obj The object to take ownership of.
   */
  protected push<T extends Disposable>(obj: T): T {
    if (obj !== undefined) {
      this.disposables.push(obj);
    }
    return obj;
  }

  /**
   * Adds `obj` to a set of objects to dispose when `this` is disposed. Objects added by
   * `track` are disposed in an unspecified order.
   * @param obj The object to track.
   */
  protected track<T extends Disposable>(obj: T): T {
    if (obj !== undefined) {
      if (this.tracked === undefined) {
        this.tracked = new Set<Disposable>();
      }
      this.tracked.add(obj);
    }
    return obj;
  }

  /**
   * Removes `obj`, which must have been previously added by `track`, from the set of objects to
   * dispose when `this` is disposed. `obj` itself is disposed.
   * @param obj The object to stop tracking.
   */
  protected disposeAndStopTracking(obj: Disposable): void {
    if (obj !== undefined) {
      this.tracked!.delete(obj);
      obj.dispose();
    }
  }

  public dispose() {
    if (this.tracked !== undefined) {
      for (const trackedObject of this.tracked.values()) {
        trackedObject.dispose();
      }
      this.tracked = undefined;
    }
    while (this.disposables.length > 0) {
      this.disposables.pop()!.dispose();
    }
  }
}
