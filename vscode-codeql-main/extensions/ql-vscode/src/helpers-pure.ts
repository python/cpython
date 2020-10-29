/**
 * helpers-pure.ts
 * ------------
 *
 * Helper functions that don't depend on vscode and therefore can be used by the front-end and pure unit tests.
 */

/**
 * This error is used to indicate a runtime failure of an exhaustivity check enforced at compile time.
 */
class ExhaustivityCheckingError extends Error {
  constructor(public expectedExhaustiveValue: never) {
    super('Internal error: exhaustivity checking failure');
  }
}

/**
 * Used to perform compile-time exhaustivity checking on a value.  This function will not be executed at runtime unless
 * the type system has been subverted.
 */
export function assertNever(value: never): never {
  throw new ExhaustivityCheckingError(value);
}
