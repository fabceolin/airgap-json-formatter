pragma Singleton
import QtQuick

QtObject {
    id: theme

    // App version
    readonly property string appVersion: "0.1.7"

    // Theme mode
    property bool darkMode: true

    // Toggle theme function
    function toggleTheme() {
        darkMode = !darkMode
    }

    // Background colors
    readonly property color background: darkMode ? "#1e1e1e" : "#f5f5f5"
    readonly property color backgroundSecondary: darkMode ? "#252526" : "#ffffff"
    readonly property color backgroundTertiary: darkMode ? "#2d2d2d" : "#e8e8e8"

    // Text colors
    readonly property color textPrimary: darkMode ? "#d4d4d4" : "#1e1e1e"
    readonly property color textSecondary: darkMode ? "#808080" : "#6e6e6e"
    readonly property color textError: darkMode ? "#f44747" : "#d32f2f"
    readonly property color textSuccess: darkMode ? "#4ec9b0" : "#2e7d32"

    // Accent colors
    readonly property color accent: darkMode ? "#0078d4" : "#0066cc"
    readonly property color border: darkMode ? "#3c3c3c" : "#d0d0d0"
    readonly property color splitHandle: darkMode ? "#505050" : "#c0c0c0"

    // Focus indicators (accessibility)
    readonly property color focusRing: darkMode ? "#0078d4" : "#0066cc"
    readonly property int focusRingWidth: 2

    // Typography
    readonly property string monoFont: "Consolas, Monaco, 'Courier New', monospace"
    readonly property int monoFontSize: 14

    // Badge colors (Airgap Protected, Valid JSON, Invalid JSON)
    readonly property color badgeSuccessBg: darkMode ? "#1a3a1a" : "#e6f4ea"
    readonly property color badgeSuccessBorder: darkMode ? "#2d5a2d" : "#34a853"
    readonly property color badgeErrorBg: darkMode ? "#4a2d2d" : "#fce8e6"
    readonly property color badgeErrorBorder: darkMode ? "#5a3d3d" : "#ea4335"

    // Responsive breakpoints (single source of truth)
    readonly property int breakpointMobile: 768
    readonly property int breakpointDesktop: 1024

    // Responsive sizing
    readonly property int touchTargetSize: 44  // Minimum touch target (Apple HIG)
    readonly property int mobileButtonHeight: 44
    readonly property int desktopButtonHeight: 34
    readonly property int mobileFontSize: 14
    readonly property int desktopFontSize: 13

    // Syntax highlighting colors (JSON/XML)
    // Dark: base16-ocean.dark theme
    // Light: inspired by GitHub light theme
    readonly property color syntaxKey: darkMode ? "#8fa1b3" : "#005cc5"
    readonly property color syntaxString: darkMode ? "#a3be8c" : "#22863a"
    readonly property color syntaxNumber: darkMode ? "#d08770" : "#e36209"
    readonly property color syntaxBoolean: darkMode ? "#b48ead" : "#6f42c1"
    readonly property color syntaxNull: darkMode ? "#bf616a" : "#d73a49"
    readonly property color syntaxPunctuation: darkMode ? "#c0c5ce" : "#586069"
    readonly property color syntaxBadge: darkMode ? "#65737e" : "#959da5"

    // ========================================
    // Markdown Syntax Highlighting Colors
    // ========================================
    // NOTE: Uses 'syntaxMd*' prefix to distinguish from JSON/XML syntax colors

    // Headings (#, ##, ###, etc.) - blue for visual hierarchy
    readonly property color syntaxMdHeading: darkMode ? "#569cd6" : "#0066cc"

    // Bold text - uses textPrimary with font-weight in actual rendering
    readonly property color syntaxMdBold: textPrimary

    // Italic text - uses textPrimary with font-style in actual rendering
    readonly property color syntaxMdItalic: textPrimary

    // Strikethrough text (~~text~~) - muted gray for de-emphasized content (WCAG AA: 4.5:1)
    readonly property color syntaxMdStrike: darkMode ? "#9d9d9d" : "#57606a"

    // Link text [text](url) - green for clickable elements (WCAG AA: 4.5:1)
    readonly property color syntaxMdLink: darkMode ? "#4ec9b0" : "#1a7f37"

    // URL portion of links - purple to distinguish from link text
    readonly property color syntaxMdUrl: darkMode ? "#ce9178" : "#6f42c1"

    // Inline code (`code`) background - subtle highlight
    readonly property color syntaxMdCodeInlineBg: darkMode ? "#2d2d2d" : "#f0f0f0"

    // Fenced code block (``` ```) background - slightly darker than inline
    readonly property color syntaxMdCodeBlockBg: darkMode ? "#1e1e1e" : "#f6f8fa"

    // Code border color for both inline and block
    readonly property color syntaxMdCodeBorder: darkMode ? "#3c3c3c" : "#d0d0d0"

    // Blockquote text (>) - muted green for quoted content (WCAG AA: 4.5:1)
    readonly property color syntaxMdBlockquote: darkMode ? "#73a561" : "#57606a"

    // Blockquote left border - visual indicator
    readonly property color syntaxMdBlockquoteBorder: darkMode ? "#608b4e" : "#dfe2e5"

    // List markers (-, *, 1.) - standard text color for readability
    readonly property color syntaxMdListMarker: darkMode ? "#d4d4d4" : "#24292e"

    // Mermaid diagram block background - blue tint to distinguish diagrams
    readonly property color syntaxMdMermaidBg: darkMode ? "#1a365d" : "#ebf8ff"

    // Mermaid diagram block border - consistent blue accent
    readonly property color syntaxMdMermaidBorder: darkMode ? "#3182ce" : "#3182ce"

    // ========================================
    // Markdown Preview Pane Colors
    // ========================================

    // Preview pane background - matches app background
    readonly property color previewBackground: darkMode ? "#1e1e1e" : "#f5f5f5"

    // Preview pane text - high contrast for readability
    readonly property color previewText: darkMode ? "#d4d4d4" : "#24292e"

    // Preview pane headings - blue for visual hierarchy
    readonly property color previewHeading: darkMode ? "#569cd6" : "#0066cc"

    // Preview pane links - distinguishable clickable elements
    readonly property color previewLink: darkMode ? "#4ec9b0" : "#0366d6"

    // Preview pane code block background
    readonly property color previewCodeBg: darkMode ? "#252526" : "#f6f8fa"

    // Preview pane code text color
    readonly property color previewCodeText: darkMode ? "#d4d4d4" : "#24292e"

    // Preview pane table border
    readonly property color previewTableBorder: darkMode ? "#3c3c3c" : "#dfe2e5"

    // Preview pane table header background
    readonly property color previewTableHeaderBg: darkMode ? "#252526" : "#f6f8fa"

    // Preview pane blockquote left border
    readonly property color previewBlockquoteBorder: darkMode ? "#3c3c3c" : "#dfe2e5"

    // Preview pane horizontal rule color
    readonly property color previewHrColor: darkMode ? "#3c3c3c" : "#e1e4e8"

    // ========================================
    // Mermaid Diagram Theme Configuration
    // ========================================
    // NOTE: QML object literals do NOT support inline JS comments inside the object.
    // These theme objects are passed to Mermaid.js via the bridge layer.

    // Computed property that switches based on darkMode
    readonly property var mermaidTheme: darkMode ? mermaidDarkTheme : mermaidLightTheme

    // Dark theme for Mermaid diagrams - VS Code Dark+ inspired
    readonly property var mermaidDarkTheme: ({
        "theme": "dark",
        "themeVariables": {
            "primaryColor": "#1a365d",
            "primaryTextColor": "#d4d4d4",
            "primaryBorderColor": "#3182ce",
            "lineColor": "#808080",
            "secondaryColor": "#252526",
            "tertiaryColor": "#2d2d2d",
            "background": "#1e1e1e",
            "mainBkg": "#1a365d",
            "nodeBorder": "#3182ce",
            "clusterBkg": "#252526",
            "clusterBorder": "#3c3c3c",
            "titleColor": "#d4d4d4",
            "edgeLabelBackground": "#252526",
            "nodeTextColor": "#d4d4d4",
            "actorBkg": "#1a365d",
            "actorBorder": "#3182ce",
            "actorTextColor": "#d4d4d4",
            "signalColor": "#d4d4d4",
            "signalTextColor": "#d4d4d4",
            "fillType0": "#1a365d",
            "fillType1": "#2d3748",
            "fillType2": "#1a365d",
            "sectionBkgColor": "#252526",
            "taskBkgColor": "#1a365d",
            "taskBorderColor": "#3182ce",
            "taskTextColor": "#d4d4d4",
            "pie1": "#3182ce",
            "pie2": "#4299e1",
            "pie3": "#63b3ed",
            "pie4": "#90cdf4",
            "pieStrokeColor": "#1e1e1e",
            "pieLegendTextColor": "#d4d4d4"
        }
    })

    // Light theme for Mermaid diagrams - GitHub light inspired
    readonly property var mermaidLightTheme: ({
        "theme": "default",
        "themeVariables": {
            "primaryColor": "#dbeafe",
            "primaryTextColor": "#1e3a5f",
            "primaryBorderColor": "#3182ce",
            "lineColor": "#6b7280",
            "secondaryColor": "#f3f4f6",
            "tertiaryColor": "#e5e7eb",
            "background": "#f5f5f5",
            "mainBkg": "#dbeafe",
            "nodeBorder": "#3182ce",
            "clusterBkg": "#f3f4f6",
            "clusterBorder": "#d1d5db",
            "titleColor": "#1e3a5f",
            "edgeLabelBackground": "#f5f5f5",
            "nodeTextColor": "#1e3a5f",
            "actorBkg": "#dbeafe",
            "actorBorder": "#3182ce",
            "actorTextColor": "#1e3a5f",
            "signalColor": "#1e3a5f",
            "signalTextColor": "#1e3a5f",
            "fillType0": "#dbeafe",
            "fillType1": "#e0f2fe",
            "fillType2": "#bfdbfe",
            "sectionBkgColor": "#f3f4f6",
            "taskBkgColor": "#dbeafe",
            "taskBorderColor": "#3182ce",
            "taskTextColor": "#1e3a5f",
            "pie1": "#3182ce",
            "pie2": "#60a5fa",
            "pie3": "#93c5fd",
            "pie4": "#bfdbfe",
            "pieStrokeColor": "#f5f5f5",
            "pieLegendTextColor": "#1e3a5f"
        }
    })
}
