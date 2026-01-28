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
    },

    // ========== Markdown Rendering Methods (Story 10.3) ==========

    /**
     * Render Markdown to HTML
     * @param {string} input - Markdown string to render
     * @returns {string} JSON string with {success: boolean, html?: string, error?: string}
     */
    renderMarkdown(input) {
        const makeError = (msg) => JSON.stringify({ success: false, error: String(msg) });
        const makeSuccess = (html) => JSON.stringify({ success: true, html: String(html) });

        if (!isInitialized) {
            return makeError('WASM not initialized');
        }

        // Handle null/undefined/empty input gracefully (AC6)
        let inputStr;
        try {
            inputStr = (input === null || input === undefined) ? '' : String(input);
        } catch (e) {
            return makeError('Invalid input');
        }

        if (inputStr.trim() === '') {
            return makeSuccess('');
        }

        try {
            const html = wasmModule.renderMarkdown(inputStr);
            return makeSuccess(html);
        } catch (e) {
            console.error('[Bridge] renderMarkdown error:', e);
            return makeError(e.message || String(e));
        }
    },

    /**
     * Decode HTML entities in a string
     * @param {string} text - Text with HTML entities
     * @returns {string} Decoded text
     */
    decodeHtmlEntities(text) {
        const textarea = document.createElement('textarea');
        textarea.innerHTML = text;
        return textarea.value;
    },

    /**
     * Render Markdown with Mermaid diagrams to HTML
     * @param {string} input - Markdown string (may contain mermaid code blocks)
     * @param {string} theme - 'dark' or 'light'
     * @returns {Promise<string>} JSON string with {success: boolean, html?: string, warnings?: string[], error?: string}
     */
    async renderMarkdownWithMermaid(input, theme = 'dark') {
        const makeError = (msg) => JSON.stringify({ success: false, error: String(msg) });

        // Step 1: Render Markdown to HTML
        const mdResultStr = this.renderMarkdown(input);
        let mdResult;
        try {
            mdResult = JSON.parse(mdResultStr);
        } catch (e) {
            return makeError('Failed to parse Markdown result');
        }

        if (!mdResult.success) {
            return JSON.stringify(mdResult);
        }

        let html = mdResult.html;

        // Step 2: Find Mermaid code blocks
        // Match <pre><code class="language-mermaid">...</code></pre> blocks
        const mermaidRegex = /<pre><code class="language-mermaid">([\s\S]*?)<\/code><\/pre>/g;
        const matches = [...html.matchAll(mermaidRegex)];

        if (matches.length === 0) {
            return JSON.stringify({ success: true, html: html });
        }

        // Step 3: Render each Mermaid block
        const errors = [];
        for (const match of matches) {
            const fullMatch = match[0];
            const code = this.decodeHtmlEntities(match[1]);

            try {
                const result = await window.renderMermaid(code, theme);
                if (result.success) {
                    html = html.replace(fullMatch, `<div class="mermaid-diagram">${result.svg}</div>`);
                } else {
                    const escapedError = (result.error || 'Unknown error')
                        .replace(/&/g, '&amp;')
                        .replace(/</g, '&lt;')
                        .replace(/>/g, '&gt;');
                    const errorHtml = `<div class="mermaid-error"><strong>Diagram Error:</strong> ${escapedError}</div>`;
                    html = html.replace(fullMatch, errorHtml);
                    errors.push(result.error);
                }
            } catch (e) {
                const escapedError = (e.message || String(e))
                    .replace(/&/g, '&amp;')
                    .replace(/</g, '&lt;')
                    .replace(/>/g, '&gt;');
                const errorHtml = `<div class="mermaid-error"><strong>Diagram Error:</strong> ${escapedError}</div>`;
                html = html.replace(fullMatch, errorHtml);
                errors.push(e.message || String(e));
            }
        }

        return JSON.stringify({
            success: true,
            html: html,
            warnings: errors.length > 0 ? errors : undefined
        });
    },

    // ========== XML Formatting Methods (Story 8.3) ==========

    /**
     * Format XML with specified indentation
     * @param {string} input - XML string to format
     * @param {string} indentType - "spaces:2", "spaces:4", or "tabs"
     * @returns {string} JSON string with {success: boolean, result?: string, error?: string}
     */
    formatXml(input, indentType) {
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
            console.error('[Bridge] formatXml input conversion error:', e);
            return makeError('Invalid input parameters');
        }

        try {
            const result = wasmModule.formatXml(inputStr, indentStr);
            if (result === null || result === undefined) {
                console.error('[Bridge] formatXml returned null/undefined');
                return makeError('formatXml returned null/undefined');
            }
            return makeSuccess(result);
        } catch (e) {
            console.error('[Bridge] formatXml error:', e);
            return makeError(e && e.message ? e.message : String(e));
        }
    },

    /**
     * Minify XML by removing whitespace
     * @param {string} input - XML string to minify
     * @returns {string} JSON string with {success: boolean, result?: string, error?: string}
     */
    minifyXml(input) {
        const makeError = (msg) => JSON.stringify({ success: false, error: String(msg) });
        const makeSuccess = (res) => JSON.stringify({ success: true, result: String(res) });

        if (!isInitialized) {
            return makeError('WASM not initialized');
        }

        // Ensure input is a string
        const inputStr = String(input || '');
        try {
            const result = wasmModule.minifyXml(inputStr);
            if (result === null || result === undefined) {
                console.error('[Bridge] minifyXml returned null/undefined');
                return makeError('minifyXml returned null/undefined');
            }
            return makeSuccess(result);
        } catch (e) {
            console.error('[Bridge] minifyXml error:', e);
            return makeError(e && e.message ? e.message : String(e));
        }
    },

    /**
     * Highlight XML with syntax colors
     * @param {string} input - XML string to highlight
     * @returns {string} HTML with syntax highlighting
     */
    highlightXml(input) {
        // Ensure input is a string
        const inputStr = String(input || '');
        if (!isInitialized) {
            return this.escapeHtml(inputStr);
        }
        try {
            const result = wasmModule.highlightXml(inputStr);
            return String(result || '');
        } catch (e) {
            console.error('[Bridge] highlightXml error:', e);
            return this.escapeHtml(inputStr);
        }
    },

    // ========== Format Auto-Detection (Story 8.4) ==========

    /**
     * Detect format of input content (JSON, XML, or unknown)
     * Detection logic: '<' prefix = XML, '{' or '[' prefix = JSON
     * Handles leading whitespace, returns 'unknown' for ambiguous content
     * @param {string} input - Content to detect format of
     * @returns {string} "json", "xml", or "unknown"
     */
    detectFormat(input) {
        // Ensure input is a string
        const inputStr = String(input || '');

        // Trim whitespace to find first meaningful character
        const trimmed = inputStr.trim();

        if (trimmed.length === 0) {
            return 'unknown';
        }

        const firstChar = trimmed.charAt(0);

        if (firstChar === '<') {
            return 'xml';
        }

        if (firstChar === '{' || firstChar === '[') {
            return 'json';
        }

        return 'unknown';
    },

    // ========== Share Methods (Story 9.3) ==========

    /**
     * Create a share URL from JSON content
     * @param {string} json - JSON string to share
     * @param {string} passphrase - Optional passphrase for protected share
     * @returns {string} JSON string with {success, url, mode} or {success: false, error}
     */
    createShareUrl(json, passphrase) {
        if (!isInitialized) {
            return JSON.stringify({ success: false, error: 'WASM not initialized' });
        }

        try {
            const jsonStr = String(json || '');
            if (!jsonStr.trim()) {
                return JSON.stringify({ success: false, error: 'No JSON to share' });
            }

            const passphraseStr = String(passphrase || '');
            const result = JSON.parse(wasmModule.createSharePayload(jsonStr, passphraseStr));

            if (!result.success) {
                return JSON.stringify({ success: false, error: result.error });
            }

            const baseUrl = `${window.location.origin}${window.location.pathname}`;
            let shareUrl;

            if (result.mode === 'quick') {
                // Quick share: key in fragment (not sent to server)
                shareUrl = `${baseUrl}?d=${encodeURIComponent(result.data)}#${result.key}`;
            } else {
                // Protected: no key in URL, p=1 flag indicates passphrase mode
                shareUrl = `${baseUrl}?d=${encodeURIComponent(result.data)}&p=1`;
            }

            // Check URL length (browsers have limits, ~8000 is safe)
            if (shareUrl.length > 8000) {
                return JSON.stringify({
                    success: false,
                    error: 'JSON too large to share (max ~50KB)'
                });
            }

            return JSON.stringify({ success: true, url: shareUrl, mode: result.mode });
        } catch (e) {
            console.error('[Bridge] createShareUrl error:', e);
            return JSON.stringify({ success: false, error: String(e) });
        }
    },

    /**
     * Get share parameters from current URL
     * @returns {string} JSON string with {hasShareData, data?, key?, isProtected?}
     */
    getShareParams() {
        try {
            const params = new URLSearchParams(window.location.search);
            const data = params.get('d');
            const isProtected = params.get('p') === '1';
            const key = window.location.hash.slice(1); // Remove leading #

            if (!data) {
                return JSON.stringify({ hasShareData: false });
            }

            return JSON.stringify({
                hasShareData: true,
                data: decodeURIComponent(data),
                key: isProtected ? null : key,
                isProtected: isProtected
            });
        } catch (e) {
            console.error('[Bridge] getShareParams error:', e);
            return JSON.stringify({ hasShareData: false });
        }
    },

    /**
     * Decode a shared payload
     * @param {string} data - Base64URL encoded payload
     * @param {string} keyOrPassphrase - Decryption key or passphrase
     * @param {boolean} isPassphrase - True if keyOrPassphrase is a passphrase
     * @returns {string} JSON string with decode result
     */
    decodeShare(data, keyOrPassphrase, isPassphrase) {
        if (!isInitialized) {
            return JSON.stringify({ success: false, error: 'WASM not initialized', errorCode: 'not_initialized' });
        }

        try {
            const result = wasmModule.decodeSharePayload(data, keyOrPassphrase, isPassphrase);
            return result; // Already JSON string from WASM
        } catch (e) {
            console.error('[Bridge] decodeShare error:', e);
            return JSON.stringify({ success: false, error: String(e), errorCode: 'unknown' });
        }
    },

    /**
     * Clear share parameters from URL (preserves history)
     */
    clearShareParams() {
        try {
            const url = new URL(window.location.href);
            url.search = '';
            url.hash = '';
            window.history.replaceState({}, '', url.pathname);
        } catch (e) {
            console.error('[Bridge] clearShareParams error:', e);
        }
    },

    // ========== Markdown Syntax Highlighting (Story 10.6) ==========

    /**
     * Highlight Markdown with syntax colors
     * @param {string} input - Markdown string to highlight
     * @returns {string} HTML with syntax highlighting
     */
    highlightMarkdown(input) {
        // Ensure input is a string
        const inputStr = String(input || '');
        if (!isInitialized) {
            return this.escapeHtml(inputStr);
        }
        try {
            const result = wasmModule.highlightMarkdown(inputStr);
            return String(result || '');
        } catch (e) {
            console.error('[Bridge] highlightMarkdown error:', e);
            return this.escapeHtml(inputStr);
        }
    }
};

// ========== Mermaid Rendering ==========

let mermaidIdCounter = 0;
let currentMermaidTheme = 'dark';

/**
 * Get Mermaid theme configuration based on dark/light mode
 * @param {boolean} isDark - Whether dark mode is enabled
 * @returns {Object} Mermaid theme configuration
 */
function getMermaidThemeConfig(isDark) {
    if (isDark) {
        return {
            theme: 'dark',
            themeVariables: {
                primaryColor: '#0078d4',
                primaryTextColor: '#d4d4d4',
                primaryBorderColor: '#3c3c3c',
                lineColor: '#808080',
                secondaryColor: '#252526',
                tertiaryColor: '#2d2d2d',
                background: '#1e1e1e',
                mainBkg: '#252526',
                nodeBorder: '#3c3c3c',
                clusterBkg: '#2d2d2d',
                titleColor: '#d4d4d4',
                edgeLabelBackground: '#252526'
            }
        };
    } else {
        return {
            theme: 'default',
            themeVariables: {
                primaryColor: '#0066cc',
                primaryTextColor: '#1e1e1e',
                primaryBorderColor: '#cccccc',
                lineColor: '#666666',
                secondaryColor: '#f5f5f5',
                tertiaryColor: '#e8e8e8',
                background: '#ffffff',
                mainBkg: '#f5f5f5',
                nodeBorder: '#cccccc',
                clusterBkg: '#e8e8e8',
                titleColor: '#1e1e1e',
                edgeLabelBackground: '#f5f5f5'
            }
        };
    }
}

/**
 * Set the Mermaid theme (called once at startup and on theme change)
 * @param {string} theme - 'dark' or 'light'
 */
function setMermaidTheme(theme) {
    if (typeof mermaid === 'undefined') {
        console.warn('[Mermaid] Library not loaded, cannot set theme');
        return;
    }

    const newTheme = theme === 'dark' ? 'dark' : 'default';
    if (newTheme !== currentMermaidTheme) {
        currentMermaidTheme = newTheme;
        const config = getMermaidThemeConfig(theme === 'dark');
        mermaid.initialize({
            ...config,
            startOnLoad: false,
            securityLevel: 'strict',
            logLevel: 'error',
            fontFamily: 'Consolas, Monaco, "Courier New", monospace',
            flowchart: { useMaxWidth: true, htmlLabels: false }
        });
        console.log('[Mermaid] Theme updated to:', theme);
    }
}

/**
 * Render a Mermaid diagram to SVG
 * @param {string} code - Mermaid diagram code
 * @param {string} theme - 'dark' or 'light'
 * @returns {Promise<Object>} Result with { success: boolean, svg?: string, error?: string, details?: string }
 */
async function renderMermaid(code, theme = 'dark') {
    // Check library availability
    if (typeof mermaid === 'undefined') {
        return { success: false, error: 'Mermaid library not loaded' };
    }
    if (typeof DOMPurify === 'undefined') {
        return { success: false, error: 'DOMPurify library not loaded' };
    }

    // Validate input
    if (!code || typeof code !== 'string' || code.trim() === '') {
        return { success: false, error: 'Empty or invalid diagram code' };
    }

    const id = `mermaid-${++mermaidIdCounter}`;

    // Update theme only if changed (initialize once, not per-render)
    setMermaidTheme(theme);

    try {
        // Race render against 5-second timeout (AC11)
        const renderPromise = mermaid.render(id, code);
        const timeoutPromise = new Promise((_, reject) =>
            setTimeout(() => reject(new Error('Render timeout (5s exceeded)')), 5000)
        );

        const { svg } = await Promise.race([renderPromise, timeoutPromise]);

        // Defense-in-depth: sanitize SVG output before returning (AC10)
        const cleanSvg = DOMPurify.sanitize(svg, {
            USE_PROFILES: { svg: true, svgFilters: true },
            ADD_TAGS: ['use']
        });

        return { success: true, svg: cleanSvg };
    } catch (e) {
        // Clean up orphaned render container on failure
        const orphan = document.getElementById('d' + id);
        if (orphan) orphan.remove();

        return {
            success: false,
            error: e.message || 'Failed to render diagram',
            details: e.toString()
        };
    }
}

// Export mermaid functions to window for Qt access (called FROM C++ JsonBridge via AsyncSerialiser)
window.renderMermaid = renderMermaid;
window.setMermaidTheme = setMermaidTheme;
window.getMermaidThemeConfig = getMermaidThemeConfig;

// Export to global scope for Qt access
window.JsonBridge = JsonBridge;
window.initBridge = initBridge;

// ES module exports for modern usage
export { initBridge, JsonBridge };
