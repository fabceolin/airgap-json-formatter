import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: markdownPreview

    // HTML content to render (already sanitized by bridge layer)
    property string html: ""

    // Track theme for CSS updates
    property bool darkMode: Theme.darkMode

    // Performance measurement (AC: 11)
    property real lastRenderTime: 0

    color: Theme.previewBackground

    // Defense-in-depth sanitization check (AC: 10)
    // Primary sanitization is in Bridge Layer (Story 10.3 via DOMPurify)
    // This is a failsafe to reject any HTML that bypassed sanitization
    function sanitizationCheck(htmlContent) {
        if (!htmlContent) return { safe: true, html: "" };

        // Check for script tags
        if (/<script[\s>]/i.test(htmlContent) || /<\/script>/i.test(htmlContent)) {
            console.warn("[MarkdownPreview] Defense-in-depth: Rejected HTML containing <script> tag");
            return { safe: false, reason: "Script tag detected" };
        }

        // Check for iframe tags
        if (/<iframe[\s>]/i.test(htmlContent) || /<\/iframe>/i.test(htmlContent)) {
            console.warn("[MarkdownPreview] Defense-in-depth: Rejected HTML containing <iframe> tag");
            return { safe: false, reason: "Iframe tag detected" };
        }

        // Check for object tags
        if (/<object[\s>]/i.test(htmlContent) || /<\/object>/i.test(htmlContent)) {
            console.warn("[MarkdownPreview] Defense-in-depth: Rejected HTML containing <object> tag");
            return { safe: false, reason: "Object tag detected" };
        }

        // Check for embed tags
        if (/<embed[\s>]/i.test(htmlContent) || /<\/embed>/i.test(htmlContent)) {
            console.warn("[MarkdownPreview] Defense-in-depth: Rejected HTML containing <embed> tag");
            return { safe: false, reason: "Embed tag detected" };
        }

        // Check for on* event handlers (onclick, onload, onerror, etc.)
        if (/\son\w+\s*=/i.test(htmlContent)) {
            console.warn("[MarkdownPreview] Defense-in-depth: Rejected HTML containing event handler");
            return { safe: false, reason: "Event handler detected" };
        }

        // Check for javascript: URLs
        if (/javascript\s*:/i.test(htmlContent)) {
            console.warn("[MarkdownPreview] Defense-in-depth: Rejected HTML containing javascript: URL");
            return { safe: false, reason: "JavaScript URL detected" };
        }

        return { safe: true, html: htmlContent };
    }

    // Apply inline styles to HTML elements for theme support (AC: 4)
    // TextArea.RichText doesn't support <style> blocks, so we inject inline styles
    function applyThemeStyles(htmlContent) {
        if (!htmlContent) return "";

        // Theme colors
        const heading = darkMode ? "#569cd6" : "#0066cc";
        const link = darkMode ? "#4ec9b0" : "#0366d6";
        const codeBg = darkMode ? "#252526" : "#f6f8fa";
        const codeText = darkMode ? "#d4d4d4" : "#24292e";
        const tableBorder = darkMode ? "#3c3c3c" : "#dfe2e5";
        const tableHeaderBg = darkMode ? "#252526" : "#f6f8fa";
        const blockquoteBorder = darkMode ? "#3c3c3c" : "#dfe2e5";
        const text = darkMode ? "#d4d4d4" : "#24292e";
        const errorBg = darkMode ? "#4a2d2d" : "#fce8e6";
        const errorBorder = darkMode ? "#5a3d3d" : "#ea4335";
        const errorText = darkMode ? "#f44747" : "#d32f2f";

        let styled = htmlContent;

        // Headings (h1-h6) - apply color and font weight
        styled = styled.replace(/<h1([^>]*)>/gi, `<h1$1 style="color: ${heading}; font-weight: 600; font-size: 2em; margin: 24px 0 16px 0; border-bottom: 1px solid ${tableBorder}; padding-bottom: 0.3em;">`);
        styled = styled.replace(/<h2([^>]*)>/gi, `<h2$1 style="color: ${heading}; font-weight: 600; font-size: 1.5em; margin: 24px 0 16px 0; border-bottom: 1px solid ${tableBorder}; padding-bottom: 0.3em;">`);
        styled = styled.replace(/<h3([^>]*)>/gi, `<h3$1 style="color: ${heading}; font-weight: 600; font-size: 1.25em; margin: 24px 0 16px 0;">`);
        styled = styled.replace(/<h4([^>]*)>/gi, `<h4$1 style="color: ${heading}; font-weight: 600; font-size: 1em; margin: 24px 0 16px 0;">`);
        styled = styled.replace(/<h5([^>]*)>/gi, `<h5$1 style="color: ${heading}; font-weight: 600; font-size: 0.875em; margin: 24px 0 16px 0;">`);
        styled = styled.replace(/<h6([^>]*)>/gi, `<h6$1 style="color: ${heading}; font-weight: 600; font-size: 0.85em; margin: 24px 0 16px 0;">`);

        // Links - styled but not clickable (pointer-events handled by CSS, color here)
        styled = styled.replace(/<a ([^>]*)>/gi, `<a $1 style="color: ${link}; text-decoration: none;">`);

        // Code blocks (pre) - background and border
        styled = styled.replace(/<pre([^>]*)>/gi, `<pre$1 style="background: ${codeBg}; color: ${codeText}; padding: 16px; border-radius: 6px; border: 1px solid ${tableBorder}; overflow-x: auto; font-family: Consolas, Monaco, 'Courier New', monospace; font-size: 13px;">`);

        // Inline code - only style code tags not inside pre
        // First, mark pre>code to skip, then style remaining code, then restore
        styled = styled.replace(/<pre([^>]*)><code/gi, '<pre$1><code-in-pre');
        styled = styled.replace(/<code([^>]*)>/gi, `<code$1 style="background: ${codeBg}; color: ${codeText}; padding: 2px 6px; border-radius: 3px; font-family: Consolas, Monaco, 'Courier New', monospace; font-size: 13px;">`);
        styled = styled.replace(/<code-in-pre/gi, '<code style="background: transparent; padding: 0;"');

        // Tables
        styled = styled.replace(/<table([^>]*)>/gi, `<table$1 style="border-collapse: collapse; width: 100%; margin: 16px 0;">`);
        styled = styled.replace(/<th([^>]*)>/gi, `<th$1 style="border: 1px solid ${tableBorder}; padding: 8px 12px; text-align: left; background: ${tableHeaderBg}; font-weight: 600;">`);
        styled = styled.replace(/<td([^>]*)>/gi, `<td$1 style="border: 1px solid ${tableBorder}; padding: 8px 12px; text-align: left;">`);

        // Blockquotes
        styled = styled.replace(/<blockquote([^>]*)>/gi, `<blockquote$1 style="border-left: 4px solid ${blockquoteBorder}; margin: 16px 0; padding: 0 16px; color: ${text}; opacity: 0.85;">`);

        // Horizontal rules
        styled = styled.replace(/<hr([^>]*)>/gi, `<hr$1 style="border: none; border-top: 1px solid ${tableBorder}; margin: 24px 0;">`);

        // Lists
        styled = styled.replace(/<ul([^>]*)>/gi, `<ul$1 style="padding-left: 2em; margin: 16px 0;">`);
        styled = styled.replace(/<ol([^>]*)>/gi, `<ol$1 style="padding-left: 2em; margin: 16px 0;">`);
        styled = styled.replace(/<li([^>]*)>/gi, `<li$1 style="margin: 4px 0;">`);

        // Mermaid diagrams
        styled = styled.replace(/class="mermaid-diagram"/gi, `class="mermaid-diagram" style="text-align: center; margin: 16px 0;"`);
        styled = styled.replace(/class="mermaid-error"/gi, `class="mermaid-error" style="background: ${errorBg}; border: 1px solid ${errorBorder}; color: ${errorText}; padding: 12px; border-radius: 4px; margin: 16px 0; font-family: Consolas, Monaco, 'Courier New', monospace; font-size: 13px;"`);

        // Images - max-width for responsive
        styled = styled.replace(/<img([^>]*)>/gi, `<img$1 style="max-width: 100%; height: auto;">`);

        return styled;
    }

    // Process and display HTML
    function updatePreview() {
        const startTime = Date.now();

        if (!html || html.trim() === "") {
            previewText.text = "";
            lastRenderTime = 0;
            return;
        }

        // Defense-in-depth check (AC: 10)
        const checkResult = sanitizationCheck(html);
        if (!checkResult.safe) {
            // Display error instead of rendering unsafe content
            const errorColor = darkMode ? "#f44747" : "#d32f2f";
            const errorBg = darkMode ? "#4a2d2d" : "#fce8e6";
            previewText.text = `<div style="color: ${errorColor}; background: ${errorBg}; padding: 16px; border-radius: 4px; margin: 8px;">
                <strong>Security Warning:</strong> ${checkResult.reason}<br/>
                <em>The Markdown content contains potentially unsafe HTML that was rejected.</em>
            </div>`;
            console.error("[MarkdownPreview] Sanitization failed:", checkResult.reason);
            return;
        }

        // Apply theme styles and render (AC: 4)
        previewText.text = applyThemeStyles(html);

        // Measure render time (AC: 11)
        lastRenderTime = Date.now() - startTime;
        if (lastRenderTime > 200) {
            console.warn("[MarkdownPreview] Render time exceeded 200ms SLA:", lastRenderTime, "ms");
        }
    }

    // Debounce timer for updates (AC: 9 - 300ms debounce)
    Timer {
        id: updateDebounce
        interval: 300
        onTriggered: updatePreview()
    }

    onHtmlChanged: {
        updateDebounce.restart();
    }

    onDarkModeChanged: {
        // Re-render with new theme colors
        if (html) {
            updatePreview();
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 1
        clip: true

        // Smooth scrolling (AC: 8)
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        TextArea {
            id: previewText
            textFormat: TextArea.RichText
            readOnly: true
            wrapMode: TextEdit.Wrap
            selectByMouse: true

            color: Theme.previewText
            font.family: "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif"
            font.pixelSize: 14

            background: Rectangle {
                color: "transparent"
            }

            // Placeholder for empty content
            Text {
                anchors.centerIn: parent
                visible: previewText.text === ""
                text: "Preview will appear here"
                color: Theme.textSecondary
                font.pixelSize: 14
            }
        }
    }

    // Public function to get the raw HTML for copy operations (AC: 7)
    function getHtmlSource() {
        return html;
    }
}
