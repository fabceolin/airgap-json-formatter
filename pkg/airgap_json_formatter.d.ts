/* tslint:disable */
/* eslint-disable */

/**
 * Format JSON with specified indentation.
 *
 * # Arguments
 * * `input` - The JSON string to format
 * * `indent` - Indent style: "spaces:2", "spaces:4", or "tabs"
 *
 * # Returns
 * * Formatted JSON string on success
 * * Throws error string on failure
 */
export function formatJson(input: string, indent: string): string;

/**
 * Format XML with specified indentation.
 *
 * # Arguments
 * * `input` - The XML string to format
 * * `indent` - Indent style: "spaces:2", "spaces:4", or "tabs"
 *
 * # Returns
 * * Formatted XML string on success
 * * Throws error string on failure
 */
export function formatXml(input: string, indent: string): string;

/**
 * Placeholder function to verify WASM binding works.
 * Returns a greeting message to confirm the module is loaded.
 */
export function greet(): string;

/**
 * Highlight JSON with syntax colors, returning HTML with inline styles.
 *
 * # Arguments
 * * `input` - The JSON string to highlight
 *
 * # Returns
 * * HTML string with inline styles for syntax highlighting
 * * Empty string if input is empty
 * * Escaped plain text if highlighting fails
 */
export function highlightJson(input: string): string;

/**
 * Highlight XML with syntax colors, returning HTML with inline styles.
 *
 * # Arguments
 * * `input` - The XML string to highlight
 *
 * # Returns
 * * HTML string with inline styles for syntax highlighting
 */
export function highlightXml(input: string): string;

/**
 * Minify JSON by removing all unnecessary whitespace.
 *
 * # Arguments
 * * `input` - The JSON string to minify
 *
 * # Returns
 * * Minified JSON string on success
 * * Throws error string on failure
 */
export function minifyJson(input: string): string;

/**
 * Minify XML by removing all unnecessary whitespace.
 *
 * # Arguments
 * * `input` - The XML string to minify
 *
 * # Returns
 * * Minified XML string on success
 * * Throws error string on failure
 */
export function minifyXml(input: string): string;

/**
 * Validate JSON and return statistics as JSON string.
 *
 * # Arguments
 * * `input` - The JSON string to validate
 *
 * # Returns
 * * JSON string containing validation result:
 *   ```json
 *   {
 *     "isValid": boolean,
 *     "error": { "message": string, "line": number, "column": number } | null,
 *     "stats": {
 *       "objectCount": number,
 *       "arrayCount": number,
 *       "stringCount": number,
 *       "numberCount": number,
 *       "booleanCount": number,
 *       "nullCount": number,
 *       "maxDepth": number,
 *       "totalKeys": number
 *     }
 *   }
 *   ```
 */
export function validateJson(input: string): string;

export type InitInput = RequestInfo | URL | Response | BufferSource | WebAssembly.Module;

export interface InitOutput {
    readonly memory: WebAssembly.Memory;
    readonly formatJson: (a: number, b: number, c: number, d: number) => [number, number, number, number];
    readonly formatXml: (a: number, b: number, c: number, d: number) => [number, number, number, number];
    readonly greet: () => [number, number];
    readonly highlightJson: (a: number, b: number) => [number, number];
    readonly highlightXml: (a: number, b: number) => [number, number];
    readonly minifyJson: (a: number, b: number) => [number, number, number, number];
    readonly minifyXml: (a: number, b: number) => [number, number, number, number];
    readonly validateJson: (a: number, b: number) => [number, number];
    readonly __wbindgen_externrefs: WebAssembly.Table;
    readonly __wbindgen_malloc: (a: number, b: number) => number;
    readonly __wbindgen_realloc: (a: number, b: number, c: number, d: number) => number;
    readonly __externref_table_dealloc: (a: number) => void;
    readonly __wbindgen_free: (a: number, b: number, c: number) => void;
    readonly __wbindgen_start: () => void;
}

export type SyncInitInput = BufferSource | WebAssembly.Module;

/**
 * Instantiates the given `module`, which can either be bytes or
 * a precompiled `WebAssembly.Module`.
 *
 * @param {{ module: SyncInitInput }} module - Passing `SyncInitInput` directly is deprecated.
 *
 * @returns {InitOutput}
 */
export function initSync(module: { module: SyncInitInput } | SyncInitInput): InitOutput;

/**
 * If `module_or_path` is {RequestInfo} or {URL}, makes a request and
 * for everything else, calls `WebAssembly.instantiate` directly.
 *
 * @param {{ module_or_path: InitInput | Promise<InitInput> }} module_or_path - Passing `InitInput` directly is deprecated.
 *
 * @returns {Promise<InitOutput>}
 */
export default function __wbg_init (module_or_path?: { module_or_path: InitInput | Promise<InitInput> } | InitInput | Promise<InitInput>): Promise<InitOutput>;
