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
     * @returns {string} JSON string with {success: boolean, result?: string, error?: string}
     */
    formatJson(input, indentType) {
        // Always return a valid JSON string, no matter what
        const makeError = (msg) => JSON.stringify({ success: false, error: String(msg) });
        const makeSuccess = (res) => JSON.stringify({ success: true, result: String(res) });

        if (!isInitialized) {
            return makeError('WASM not initialized');
        }

        // Defensive: ensure inputs are valid strings
        let inputStr, indentStr;
        try {
            inputStr = (input === null || input === undefined) ? '' : String(input);
            indentStr = (indentType === null || indentType === undefined) ? 'spaces:4' : String(indentType);
        } catch (e) {
            console.error('[Bridge] formatJson input conversion error:', e);
            return makeError('Invalid input parameters');
        }

        // Debug: log input info (truncated for large inputs)
        console.debug('[Bridge] formatJson called with input length:', inputStr.length,
                      'indent:', indentStr,
                      'first 100 chars:', inputStr.substring(0, 100));

        try {
            const result = wasmModule.formatJson(inputStr, indentStr);
            if (result === null || result === undefined) {
                console.error('[Bridge] formatJson returned null/undefined');
                return makeError('formatJson returned null/undefined');
            }
            return makeSuccess(result);
        } catch (e) {
            console.error('[Bridge] formatJson error:', e);
            return makeError(e && e.message ? e.message : String(e));
        }
    },

    /**
     * Minify JSON by removing whitespace
     * @param {string} input - JSON string to minify
     * @returns {string} JSON string with {success: boolean, result?: string, error?: string}
     */
    minifyJson(input) {
        if (!isInitialized) {
            return JSON.stringify({ success: false, error: 'WASM not initialized' });
        }
        // Ensure input is a string
        const inputStr = String(input || '');
        try {
            const result = wasmModule.minifyJson(inputStr);
            return JSON.stringify({ success: true, result: String(result || '') });
        } catch (e) {
            return JSON.stringify({ success: false, error: String(e) });
        }
    },

    /**
     * Validate JSON and return statistics
     * @param {string} input - JSON string to validate
     * @returns {string} JSON string with validation result
     */
    validateJson(input) {
        if (!isInitialized) {
            return JSON.stringify({
                isValid: false,
                error: { message: 'WASM not initialized', line: 0, column: 0 },
                stats: {}
            });
        }
        // Ensure input is a string
        const inputStr = String(input || '');
        try {
            // wasmModule.validateJson already returns a JSON string
            const result = wasmModule.validateJson(inputStr);
            return String(result || '{}');
        } catch (e) {
            return JSON.stringify({
                isValid: false,
                error: { message: String(e), line: 0, column: 0 },
                stats: {}
            });
        }
    },

    /**
     * Highlight JSON with syntax colors
     * @param {string} input - JSON string to highlight
     * @returns {string} HTML with syntax highlighting
     */
    highlightJson(input) {
        // Ensure input is a string
        const inputStr = String(input || '');
        if (!isInitialized) {
            return this.escapeHtml(inputStr);
        }
        try {
            const result = wasmModule.highlightJson(inputStr);
            return String(result || '');
        } catch (e) {
            console.error('[Bridge] highlightJson error:', e);
            return this.escapeHtml(inputStr);
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
        const CLIPBOARD_TIMEOUT_MS = 5000; // 5 second timeout

        // Early check: Clipboard API availability
        if (!navigator.clipboard || typeof navigator.clipboard.readText !== 'function') {
            console.warn('[Bridge] Clipboard API not available');
            return '';
        }

        try {
            // Pre-check: Verify clipboard-read permission status (fail fast if denied)
            if (navigator.permissions && typeof navigator.permissions.query === 'function') {
                try {
                    const permissionStatus = await Promise.race([
                        navigator.permissions.query({ name: 'clipboard-read' }),
                        new Promise((_, reject) =>
                            setTimeout(() => reject(new Error('Permission query timeout')), 1000)
                        )
                    ]);

                    if (permissionStatus && permissionStatus.state === 'denied') {
                        console.warn('[Bridge] Clipboard read permission denied');
                        return '';
                    }
                } catch (permErr) {
                    // Permission API not supported or timed out - continue with read attempt
                    console.debug('[Bridge] Permission pre-check skipped:', permErr && permErr.message);
                }
            }

            // Attempt clipboard read with timeout
            const text = await Promise.race([
                navigator.clipboard.readText(),
                new Promise((_, reject) =>
                    setTimeout(() => reject(new Error('Clipboard read timeout')), CLIPBOARD_TIMEOUT_MS)
                )
            ]);

            return text || '';
        } catch (e) {
            if (e && e.message === 'Clipboard read timeout') {
                console.warn('[Bridge] Clipboard read timed out after', CLIPBOARD_TIMEOUT_MS, 'ms');
            } else if (e && e.name === 'NotAllowedError') {
                console.warn('[Bridge] Clipboard read not allowed (permission denied or no focus)');
            } else {
                console.error('[Bridge] Clipboard read failed:', e);
            }
            return '';
        }
    },

    // ========== History Storage Methods ==========

    /**
     * Initialize history storage
     * @returns {Promise<boolean>}
     */
    async initHistory() {
        if (window.HistoryStorage) {
            return await window.HistoryStorage.init();
        }
        console.warn('[Bridge] HistoryStorage not available');
        return false;
    },

    /**
     * Save JSON to history
     * @param {string} json - JSON content to save
     * @returns {Promise<string>} JSON result with success/error
     */
    async saveToHistory(json) {
        try {
            if (!window.HistoryStorage) {
                return JSON.stringify({ success: false, error: 'HistoryStorage not available' });
            }
            const jsonStr = String(json || '');
            const result = await window.HistoryStorage.save(jsonStr);
            // Ensure we always return a valid JSON string
            if (result === null || result === undefined) {
                return JSON.stringify({ success: false, error: 'HistoryStorage.save returned null' });
            }
            return JSON.stringify(result);
        } catch (e) {
            console.error('[Bridge] saveToHistory error:', e);
            return JSON.stringify({ success: false, error: String(e && e.message ? e.message : e) });
        }
    },

    /**
     * Load all history entries
     * @returns {Promise<string>} JSON array of entries
     */
    async loadHistory() {
        try {
            if (!window.HistoryStorage) {
                return JSON.stringify({ success: false, error: 'HistoryStorage not available' });
            }
            const result = await window.HistoryStorage.loadAll();
            if (result === null || result === undefined) {
                return JSON.stringify({ success: false, error: 'HistoryStorage.loadAll returned null' });
            }
            return JSON.stringify(result);
        } catch (e) {
            console.error('[Bridge] loadHistory error:', e);
            return JSON.stringify({ success: false, error: String(e && e.message ? e.message : e) });
        }
    },

    /**
     * Get a single history entry by ID
     * @param {string} id - Entry ID
     * @returns {Promise<string>} JSON entry or error
     */
    async getHistoryEntry(id) {
        if (!window.HistoryStorage) {
            return JSON.stringify({ success: false, error: 'HistoryStorage not available' });
        }
        const result = await window.HistoryStorage.get(id);
        return JSON.stringify(result);
    },

    /**
     * Delete a history entry by ID
     * @param {string} id - Entry ID to delete
     * @returns {Promise<string>} JSON result
     */
    async deleteHistoryEntry(id) {
        if (!window.HistoryStorage) {
            return JSON.stringify({ success: false, error: 'HistoryStorage not available' });
        }
        const result = await window.HistoryStorage.delete(id);
        return JSON.stringify(result);
    },

    /**
     * Clear all history entries
     * @returns {Promise<string>} JSON result
     */
    async clearHistory() {
        if (!window.HistoryStorage) {
            return JSON.stringify({ success: false, error: 'HistoryStorage not available' });
        }
        const result = await window.HistoryStorage.clear();
        return JSON.stringify(result);
    },

    /**
     * Check if history storage is available
     * @returns {boolean}
     */
    isHistoryAvailable() {
        return window.HistoryStorage && window.HistoryStorage.isAvailable();
    }
};

// Export to global scope for Qt access
window.JsonBridge = JsonBridge;
window.initBridge = initBridge;

// ES module exports for modern usage
export { initBridge, JsonBridge };
