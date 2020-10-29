/**
 * The npm library jszip is designed to work in both the browser and
 * node. Consequently its typings @types/jszip refers to both node
 * types like `Buffer` (which don't exist in the browser), and browser
 * types like `Blob` (which don't exist in node). Instead of sticking
 * all of `dom` in `compilerOptions.lib`, it suffices just to put in a
 * stub definition of the type `Blob` here so that compilation
 * succeeds.
 */

declare type Blob = string;
