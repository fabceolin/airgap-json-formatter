pragma Singleton
import QtQuick

QtObject {
    id: theme

    // App version
    readonly property string appVersion: "0.1.8"

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

    // Syntax highlighting colors
    // Dark: base16-ocean.dark theme
    // Light: inspired by GitHub light theme
    readonly property color syntaxKey: darkMode ? "#8fa1b3" : "#005cc5"
    readonly property color syntaxString: darkMode ? "#a3be8c" : "#22863a"
    readonly property color syntaxNumber: darkMode ? "#d08770" : "#e36209"
    readonly property color syntaxBoolean: darkMode ? "#b48ead" : "#6f42c1"
    readonly property color syntaxNull: darkMode ? "#bf616a" : "#d73a49"
    readonly property color syntaxPunctuation: darkMode ? "#c0c5ce" : "#586069"
    readonly property color syntaxBadge: darkMode ? "#65737e" : "#959da5"
}
