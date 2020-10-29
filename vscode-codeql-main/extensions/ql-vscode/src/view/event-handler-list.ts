export type EventHandler<T> = (event: T) => void;

/**
 * A set of listeners for events of type `T`.
 */
export class EventHandlers<T> {
  private handlers: EventHandler<T>[] = [];

  public addListener(handler: EventHandler<T>) {
    this.handlers.push(handler);
  }

  public removeListener(handler: EventHandler<T>) {
    const index = this.handlers.indexOf(handler);
    if (index !== -1) {
      this.handlers.splice(index, 1);
    }
  }

  public fire(event: T) {
    for (const handler of this.handlers) {
      handler(event);
    }
  }
}
