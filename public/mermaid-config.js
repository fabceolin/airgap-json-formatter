/**
 * Mermaid.js Configuration
 * Initializes mermaid with secure defaults for airgap operation
 */

document.addEventListener('DOMContentLoaded', () => {
    if (typeof mermaid !== 'undefined') {
        mermaid.initialize({
            startOnLoad: false,           // Manual rendering only
            securityLevel: 'strict',      // XSS protection
            theme: 'dark',                // Default theme
            fontFamily: 'Consolas, Monaco, "Courier New", monospace',
            logLevel: 'error',            // Suppress verbose logging

            // Disable features that might make network requests
            flowchart: {
                useMaxWidth: true,
                htmlLabels: false         // Safer rendering
            }
        });
        console.log('[Mermaid] Initialized with secure defaults');
    }
});
