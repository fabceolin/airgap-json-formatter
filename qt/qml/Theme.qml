import QtQuick

QtObject {
    // Background colors
    readonly property color background: "#1e1e1e"
    readonly property color backgroundSecondary: "#252526"
    readonly property color backgroundTertiary: "#2d2d2d"

    // Text colors
    readonly property color textPrimary: "#d4d4d4"
    readonly property color textSecondary: "#808080"
    readonly property color textError: "#f44747"
    readonly property color textSuccess: "#4ec9b0"

    // Accent colors
    readonly property color accent: "#0078d4"
    readonly property color border: "#3c3c3c"
    readonly property color splitHandle: "#505050"

    // Focus indicators (accessibility)
    readonly property color focusRing: "#0078d4"
    readonly property int focusRingWidth: 2

    // Typography
    readonly property string monoFont: "Consolas, Monaco, 'Courier New', monospace"
    readonly property int monoFontSize: 14

    // Syntax highlighting colors (reference only - provided by Rust syntect via inline HTML styles)
    // Theme: base16-ocean.dark
    // Keys:        #8fa1b3 (light blue)
    // Strings:     #a3be8c (green)
    // Numbers:     #d08770 (orange)
    // Booleans:    #b48ead (purple)
    // Null:        #bf616a (red)
    // Punctuation: #c0c5ce (light gray)
}
