import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Rectangle {
    id: toolbar
    height: window.isMobile ? 56 : 50  // Larger on mobile for touch
    color: Theme.backgroundTertiary

    signal formatRequested(string indentType)
    signal minifyRequested()
    signal copyRequested()
    signal clearRequested()
    signal viewModeChanged(string mode)  // Story 10.5: Changed from toggle to explicit mode
    signal expandAllRequested()
    signal collapseAllRequested()
    signal loadHistoryRequested()
    signal overflowMenuRequested()  // For mobile overflow menu
    signal shareRequested()  // Story 9.3: Share button signal
    signal formatSelected(string format)  // Story 8.5: Format selector signal

    property string selectedIndent: "spaces:4"
    property alias copyButtonText: copyButton.text
    property string viewMode: "tree"  // "tree" or "text"
    property bool isBusy: JsonBridge.busy  // Track busy state from JsonBridge
    property bool canShare: false  // Story 9.3: Enable when valid JSON in input

    // Story 8.5: Format selector properties
    property string detectedFormat: "json"  // Bound from Main.qml
    property string effectiveFormat: formatCombo.effectiveFormat  // Computed from formatCombo

    // Story 8.5: Reset format selector to Auto (AC: 5)
    function resetFormatSelector() {
        formatCombo.currentIndex = 0;  // Reset to "Auto"
    }

    // Responsive mode properties (reference window breakpoints)
    property bool compactMode: typeof window !== "undefined" && window.width < Theme.breakpointDesktop
    property bool mobileMode: typeof window !== "undefined" && window.width < Theme.breakpointMobile

    // Sync combo box when selectedIndent changes externally (e.g., from overflow menu)
    onSelectedIndentChanged: {
        if (selectedIndent === "spaces:2" && indentCombo.currentIndex !== 0) {
            indentCombo.currentIndex = 0;
        } else if (selectedIndent === "spaces:4" && indentCombo.currentIndex !== 1) {
            indentCombo.currentIndex = 1;
        } else if (selectedIndent === "tabs" && indentCombo.currentIndex !== 2) {
            indentCombo.currentIndex = 2;
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: toolbar.mobileMode ? 4 : 8
        spacing: toolbar.mobileMode ? 4 : 8

        // ═══════════════════════════════════════════════════════════════
        // LEFT ZONE: Input Controls (hidden on mobile - moved to overflow)
        // ═══════════════════════════════════════════════════════════════

        // Load button (hidden on mobile)
        Button {
            id: loadButton
            text: toolbar.compactMode ? "📁" : "Load"
            visible: !toolbar.mobileMode
            onClicked: toolbar.loadHistoryRequested()

            contentItem: Text {
                text: loadButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: toolbar.compactMode ? 16 : 13
            }
            background: Rectangle {
                implicitWidth: toolbar.compactMode ? Theme.desktopButtonHeight : 60
                implicitHeight: Theme.desktopButtonHeight
                color: loadButton.hovered ? Theme.backgroundSecondary : "transparent"
                border.color: loadButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: loadButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Load from History (Ctrl+O)"
            ToolTip.delay: 500
        }

        // Clear button (hidden on mobile)
        Button {
            id: clearButton
            text: toolbar.compactMode ? "🗑" : "Clear"
            visible: !toolbar.mobileMode
            onClicked: toolbar.clearRequested()

            contentItem: Text {
                text: clearButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: toolbar.compactMode ? 16 : 13
            }
            background: Rectangle {
                implicitWidth: toolbar.compactMode ? Theme.desktopButtonHeight : 60
                implicitHeight: Theme.desktopButtonHeight
                color: clearButton.hovered ? Theme.backgroundSecondary : "transparent"
                border.color: clearButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: clearButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Clear All (Ctrl+Shift+X)"
            ToolTip.delay: 500
        }

        // ═══════════════════════════════════════════════════════════════
        // CENTER ZONE: Processing Actions
        // ═══════════════════════════════════════════════════════════════

        Item { Layout.fillWidth: true }  // Left spacer

        // Story 8.5 + 10.4: Format selector (hidden on mobile - moved to overflow menu)
        ComboBox {
            id: formatCombo
            visible: !toolbar.mobileMode
            model: ["Auto", "JSON", "XML", "Markdown"]  // Story 10.4: Added Markdown option
            currentIndex: 0  // Default to Auto
            focusPolicy: Qt.StrongFocus  // AC: 7 - Keyboard accessibility

            // Computed effective format (AC: 3)
            property string effectiveFormat: {
                if (currentIndex === 0) {
                    // Auto mode - use detected format
                    return toolbar.detectedFormat;
                } else if (currentIndex === 1) {
                    return "json";
                } else if (currentIndex === 2) {
                    return "xml";
                } else {
                    return "markdown";
                }
            }

            // Display text shows detection result in Auto mode (AC: 1, 2)
            displayText: {
                if (currentIndex === 0) {
                    var detected = toolbar.detectedFormat;
                    if (detected === "unknown") return "Auto";
                    return detected.toUpperCase() + " (auto)";
                }
                return currentText;
            }

            contentItem: Text {
                leftPadding: 8
                text: formatCombo.displayText
                color: Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
            }

            background: Rectangle {
                implicitWidth: 120  // Story 10.4: Wider to accommodate "MARKDOWN (auto)"
                implicitHeight: Theme.desktopButtonHeight
                color: Theme.backgroundSecondary
                border.color: formatCombo.activeFocus ? Theme.focusRing : Theme.border
                border.width: formatCombo.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            popup: Popup {
                y: formatCombo.height
                width: formatCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 1

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: formatCombo.popup.visible ? formatCombo.delegateModel : null
                    currentIndex: formatCombo.highlightedIndex
                }

                background: Rectangle {
                    color: Theme.backgroundSecondary
                    border.color: Theme.border
                    radius: 4
                }
            }

            delegate: ItemDelegate {
                width: formatCombo.width
                height: Theme.touchTargetSize  // Touch-friendly on popup
                contentItem: Text {
                    text: modelData
                    color: Theme.textPrimary
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: highlighted ? Theme.accent : "transparent"
                }
                highlighted: formatCombo.highlightedIndex === index
            }

            // AC: 6 - Tooltip
            ToolTip.visible: hovered
            ToolTip.text: "Format is auto-detected. Select manually to override."
            ToolTip.delay: 500

            // Emit signal on user selection (AC: 1, 3)
            onActivated: {
                toolbar.formatSelected(effectiveFormat);
            }
        }

        // Indent selector (hidden on mobile - moved to overflow menu)
        ComboBox {
            id: indentCombo
            visible: !toolbar.mobileMode
            model: ["2 sp", "4 sp", "Tab"]
            currentIndex: 1  // Default: 4 spaces

            onCurrentIndexChanged: {
                const mapping = ["spaces:2", "spaces:4", "tabs"];
                toolbar.selectedIndent = mapping[currentIndex];
            }

            contentItem: Text {
                leftPadding: 8
                text: indentCombo.displayText
                color: Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
            }

            background: Rectangle {
                implicitWidth: 70
                implicitHeight: Theme.desktopButtonHeight
                color: Theme.backgroundSecondary
                border.color: indentCombo.activeFocus ? Theme.focusRing : Theme.border
                border.width: indentCombo.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            popup: Popup {
                y: indentCombo.height
                width: indentCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 1

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: indentCombo.popup.visible ? indentCombo.delegateModel : null
                    currentIndex: indentCombo.highlightedIndex
                }

                background: Rectangle {
                    color: Theme.backgroundSecondary
                    border.color: Theme.border
                    radius: 4
                }
            }

            delegate: ItemDelegate {
                width: indentCombo.width
                height: Theme.touchTargetSize  // Touch-friendly on popup
                contentItem: Text {
                    text: modelData
                    color: Theme.textPrimary
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: highlighted ? Theme.accent : "transparent"
                }
                highlighted: indentCombo.highlightedIndex === index
            }

            ToolTip.visible: hovered
            ToolTip.text: "Indentation Style"
            ToolTip.delay: 500
        }

        // Minify button (hidden on mobile - moved to overflow menu)
        Button {
            id: minifyButton
            text: toolbar.compactMode ? "−" : "Minify"
            visible: !toolbar.mobileMode
            enabled: !toolbar.isBusy
            onClicked: toolbar.minifyRequested()

            contentItem: Text {
                text: minifyButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: toolbar.compactMode ? 18 : 13
                opacity: minifyButton.enabled ? 1.0 : 0.5
            }
            background: Rectangle {
                implicitWidth: toolbar.compactMode ? Theme.desktopButtonHeight : 70
                implicitHeight: Theme.desktopButtonHeight
                color: minifyButton.hovered && minifyButton.enabled ? Theme.backgroundSecondary : "transparent"
                border.color: minifyButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: minifyButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
                opacity: minifyButton.enabled ? 1.0 : 0.5
            }

            ToolTip.visible: hovered
            ToolTip.text: toolbar.isBusy ? "Processing..." : "Minify JSON (Ctrl+M)"
            ToolTip.delay: 500
        }

        // FORMAT button (PRIMARY CTA) - Always visible
        Button {
            id: formatButton
            text: toolbar.isBusy ? "..." : "Format"
            enabled: !toolbar.isBusy
            onClicked: toolbar.formatRequested(toolbar.selectedIndent)

            contentItem: Text {
                text: formatButton.text
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: toolbar.mobileMode ? Theme.mobileFontSize : 14
                font.weight: Font.DemiBold
                opacity: formatButton.enabled ? 1.0 : 0.7
            }
            background: Rectangle {
                implicitWidth: toolbar.mobileMode ? 80 : 90
                implicitHeight: toolbar.mobileMode ? Theme.mobileButtonHeight : 36
                color: !formatButton.enabled ? Qt.darker(Theme.accent, 1.3) :
                       formatButton.pressed ? Qt.darker(Theme.accent, 1.2) :
                       formatButton.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                border.color: formatButton.activeFocus ? Theme.focusRing : "transparent"
                border.width: formatButton.activeFocus ? Theme.focusRingWidth : 0
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: toolbar.isBusy ? "Processing..." : "Format JSON (Ctrl+Enter)"
            ToolTip.delay: 500
        }

        Item { Layout.fillWidth: true }  // Right spacer

        // ═══════════════════════════════════════════════════════════════
        // RIGHT ZONE: View & Output Controls
        // ═══════════════════════════════════════════════════════════════

        // Story 10.5: Segmented View Mode Control (Code | Tree | Preview) - hidden on mobile
        // Shows different options based on format: markdown shows Code/Preview, JSON/XML shows Code/Tree
        Row {
            visible: !toolbar.mobileMode
            spacing: 0

            // Code button - always first
            Button {
                id: codeViewBtn
                width: 50
                height: Theme.desktopButtonHeight

                contentItem: Text {
                    text: "Code"
                    color: toolbar.viewMode === "text" ? "#ffffff" : Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.weight: toolbar.viewMode === "text" ? Font.Medium : Font.Normal
                }
                background: Rectangle {
                    color: toolbar.viewMode === "text" ? Theme.accent : Theme.backgroundSecondary
                    border.color: Theme.border
                    border.width: 1
                    radius: 4
                    // Round only left corners
                    Rectangle {
                        anchors.right: parent.right
                        width: parent.radius
                        height: parent.height
                        color: parent.color
                    }
                }

                onClicked: {
                    if (toolbar.viewMode !== "text") {
                        toolbar.viewModeChanged("text")
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: "Code View (Ctrl+T)"
                ToolTip.delay: 500
            }

            // Tree button - visible for JSON/XML only (AC: 2)
            Button {
                id: treeViewBtn
                width: 50
                height: Theme.desktopButtonHeight
                visible: toolbar.effectiveFormat !== "markdown"

                contentItem: Text {
                    text: "Tree"
                    color: toolbar.viewMode === "tree" ? "#ffffff" : Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.weight: toolbar.viewMode === "tree" ? Font.Medium : Font.Normal
                }
                background: Rectangle {
                    color: toolbar.viewMode === "tree" ? Theme.accent : Theme.backgroundSecondary
                    border.color: Theme.border
                    border.width: 1
                    radius: 4
                    // Round only right corners
                    Rectangle {
                        anchors.left: parent.left
                        width: parent.radius
                        height: parent.height
                        color: parent.color
                    }
                }

                onClicked: {
                    if (toolbar.viewMode !== "tree") {
                        toolbar.viewModeChanged("tree")
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: "Tree View (Ctrl+T)"
                ToolTip.delay: 500
            }

            // Preview button - visible for Markdown only (AC: 1, 2)
            Button {
                id: previewViewBtn
                width: 60
                height: Theme.desktopButtonHeight
                visible: toolbar.effectiveFormat === "markdown"

                contentItem: Text {
                    text: "Preview"
                    color: toolbar.viewMode === "preview" ? "#ffffff" : Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.weight: toolbar.viewMode === "preview" ? Font.Medium : Font.Normal
                }
                background: Rectangle {
                    color: toolbar.viewMode === "preview" ? Theme.accent : Theme.backgroundSecondary
                    border.color: Theme.border
                    border.width: 1
                    radius: 4
                    // Round only right corners
                    Rectangle {
                        anchors.left: parent.left
                        width: parent.radius
                        height: parent.height
                        color: parent.color
                    }
                }

                onClicked: {
                    if (toolbar.viewMode !== "preview") {
                        toolbar.viewModeChanged("preview")
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: "Preview Mode (Ctrl+T)"
                ToolTip.delay: 500
            }
        }

        // Expand/Collapse buttons (tree mode only, hidden on mobile)
        Button {
            id: expandAllButton
            visible: !toolbar.mobileMode
            enabled: toolbar.viewMode === "tree"
            opacity: enabled ? 1.0 : 0.4
            onClicked: toolbar.expandAllRequested()

            contentItem: Text {
                text: "⊞"
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14
            }
            background: Rectangle {
                implicitWidth: Theme.desktopButtonHeight
                implicitHeight: Theme.desktopButtonHeight
                color: expandAllButton.hovered && expandAllButton.enabled ? Theme.backgroundSecondary : "transparent"
                border.color: expandAllButton.hovered && expandAllButton.enabled ? Theme.border : "transparent"
                border.width: 1
                radius: 4
            }

            ToolTip.visible: hovered && enabled
            ToolTip.text: "Expand All (Ctrl+E)"
            ToolTip.delay: 500
        }

        Button {
            id: collapseAllButton
            visible: !toolbar.mobileMode
            enabled: toolbar.viewMode === "tree"
            opacity: enabled ? 1.0 : 0.4
            onClicked: toolbar.collapseAllRequested()

            contentItem: Text {
                text: "⊟"
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14
            }
            background: Rectangle {
                implicitWidth: Theme.desktopButtonHeight
                implicitHeight: Theme.desktopButtonHeight
                color: collapseAllButton.hovered && collapseAllButton.enabled ? Theme.backgroundSecondary : "transparent"
                border.color: collapseAllButton.hovered && collapseAllButton.enabled ? Theme.border : "transparent"
                border.width: 1
                radius: 4
            }

            ToolTip.visible: hovered && enabled
            ToolTip.text: "Collapse All (Ctrl+Shift+E)"
            ToolTip.delay: 500
        }

        // Separator (hidden on mobile)
        Rectangle {
            visible: !toolbar.mobileMode
            width: 1
            height: 26
            color: Theme.border
        }

        // Share button (Story 9.3) - between separator and Copy
        Button {
            id: shareButton
            text: toolbar.compactMode ? "🔗" : "Share"
            visible: !toolbar.mobileMode
            enabled: toolbar.canShare && !toolbar.isBusy
            onClicked: toolbar.shareRequested()

            contentItem: Text {
                text: shareButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: toolbar.compactMode ? 16 : 13
                opacity: shareButton.enabled ? 1.0 : 0.5
            }
            background: Rectangle {
                implicitWidth: toolbar.compactMode ? Theme.desktopButtonHeight : 60
                implicitHeight: Theme.desktopButtonHeight
                color: shareButton.hovered && shareButton.enabled ? Theme.backgroundSecondary : "transparent"
                border.color: shareButton.hovered && shareButton.enabled ? Theme.border : "transparent"
                border.width: 1
                radius: 4
                opacity: shareButton.enabled ? 1.0 : 0.5
            }

            ToolTip.visible: hovered
            ToolTip.text: toolbar.canShare
                ? "Time-bounded encrypted link (5 min). Anyone with link can view unless passphrase-protected. (Ctrl+Shift+S)"
                : "Enter valid JSON to share"
            ToolTip.delay: 500
        }

        // Copy Output button (hidden on mobile - moved to overflow)
        Button {
            id: copyButton
            text: toolbar.compactMode ? "📋" : "Copy"
            visible: !toolbar.mobileMode
            onClicked: toolbar.copyRequested()

            contentItem: Text {
                text: copyButton.text
                font.pixelSize: toolbar.compactMode ? 16 : 13
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitWidth: toolbar.compactMode ? Theme.desktopButtonHeight : 60
                implicitHeight: Theme.desktopButtonHeight
                color: copyButton.hovered ? Theme.backgroundSecondary : "transparent"
                border.color: copyButton.hovered ? Theme.border : "transparent"
                border.width: 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Copy Output (Ctrl+C)"
            ToolTip.delay: 500
        }

        // ═══════════════════════════════════════════════════════════════
        // MOBILE: Overflow menu button (hamburger)
        // ═══════════════════════════════════════════════════════════════
        Button {
            id: overflowMenuButton
            visible: toolbar.mobileMode
            onClicked: toolbar.overflowMenuRequested()

            contentItem: Text {
                text: "☰"
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 20
            }
            background: Rectangle {
                implicitWidth: Theme.mobileButtonHeight
                implicitHeight: Theme.mobileButtonHeight
                color: overflowMenuButton.pressed ? Theme.backgroundSecondary : "transparent"
                border.color: Theme.border
                border.width: 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "More Options"
            ToolTip.delay: 500
        }
    }
}
