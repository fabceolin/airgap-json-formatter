/**
 * Bridge between Qt WASM and Rust WASM modules
 * Provides a clean API for Qt/QML to call Rust JSON functions
 */

let wasmModule = null;
let isInitialized = false;

/**
 * Initialize the Rust WASM module
 * @returns {Promise<void>}
 */
async function initBridge() {
    if (isInitialized) {
        console.log('[Bridge] Already initialized');
        return;
    }

    console.log('[Airgap] Initializing local JSON processor...');

    try {
        const wasm = await import('./pkg/airgap_json_formatter.js');
        await wasm.default();
        wasmModule = wasm;
        isInitialized = true;

        console.log('[Airgap] Rust WASM module loaded');
        console.log('[Airgap] All processing happens locally in browser');
        console.log('[Airgap] Zero network requests - data never leaves device');
        console.log('[Airgap] Mode Active - Ready for secure JSON processing');
    } catch (error) {
        console.error('[Airgap] Failed to initialize WASM:', error);
        throw error;
    }
}

/**
 * JSON Bridge object - exposed to Qt/QML via window.JsonBridge
 */
const JsonBridge = {
    /**
     * Check if the bridge is initialized
     * @returns {boolean}
     */
    isReady() {
        return isInitialized;
    },

    /**
     * Format JSON with specified indentation
     * @param {string} input - JSON string to format
     * @param {string} indentType - "spaces:2", "spaces:4", or "tabs"
     * @returns {{success: boolean, result?: string, error?: string}}
     */
    formatJson(input, indentType) {
        if (!isInitialized) {
            return { success: false, error: 'WASM not initialized' };
        }
        try {
            const result = wasmModule.formatJson(input, indentType);
            return { success: true, result };
        } catch (e) {
            return { success: false, error: String(e) };
        }
    },

    /**
     * Minify JSON by removing whitespace
     * @param {string} input - JSON string to minify
     * @returns {{success: boolean, result?: string, error?: string}}
     */
    minifyJson(input) {
        if (!isInitialized) {
            return { success: false, error: 'WASM not initialized' };
        }
        try {
            const result = wasmModule.minifyJson(input);
            return { success: true, result };
        } catch (e) {
            return { success: false, error: String(e) };
        }
    },

    /**
     * Validate JSON and return statistics
     * @param {string} input - JSON string to validate
     * @returns {{isValid: boolean, error?: {message: string, line: number, column: number}, stats: object}}
     */
    validateJson(input) {
        if (!isInitialized) {
            return {
                isValid: false,
                error: { message: 'WASM not initialized', line: 0, column: 0 },
                stats: {}
            };
        }
        try {
            const resultJson = wasmModule.validateJson(input);
            return JSON.parse(resultJson);
        } catch (e) {
            return {
                isValid: false,
                error: { message: String(e), line: 0, column: 0 },
                stats: {}
            };
        }
    },

    /**
     * Highlight JSON with syntax colors
     * @param {string} input - JSON string to highlight
     * @returns {string} HTML with syntax highlighting
     */
    highlightJson(input) {
        if (!isInitialized) {
            return this.escapeHtml(input);
        }
        try {
            return wasmModule.highlightJson(input);
        } catch (e) {
            console.error('[Bridge] highlightJson error:', e);
            return this.escapeHtml(input);
        }
    },

    /**
     * Escape HTML special characters
     * @param {string} text - Text to escape
     * @returns {string} Escaped text
     */
    escapeHtml(text) {
        if (!text) return '';
        return text
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');
    },

    /**
     * Copy text to clipboard
     * @param {string} text - Text to copy
     * @returns {Promise<boolean>}
     */
    async copyToClipboard(text) {
        try {
            await navigator.clipboard.writeText(text);
            return true;
        } catch (e) {
            console.error('[Bridge] Clipboard write failed:', e);
            return false;
        }
    },

    /**
     * Read text from clipboard
     * @returns {Promise<string>} The clipboard text or empty string on failure
     */
    async readFromClipboard() {
        try {
            const text = await navigator.clipboard.readText();
            return text || '';
        } catch (e) {
            console.error('[Bridge] Clipboard read failed:', e);
            return '';
        }
    }
};

// Export to global scope for Qt access
window.JsonBridge = JsonBridge;
window.initBridge = initBridge;

// ES module exports for modern usage
export { initBridge, JsonBridge };
