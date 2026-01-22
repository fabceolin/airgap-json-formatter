import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Rectangle {
    id: toolbar
    height: 50
    color: Theme.backgroundTertiary

    signal formatRequested(string indentType)
    signal minifyRequested()
    signal copyRequested()
    signal clearRequested()
    signal viewModeToggled()
    signal expandAllRequested()
    signal collapseAllRequested()
    signal loadHistoryRequested()

    property string selectedIndent: "spaces:4"
    property alias copyButtonText: copyButton.text
    property string viewMode: "tree"  // "tree" or "text"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // ═══════════════════════════════════════════════════════════════
        // LEFT ZONE: Input Controls
        // ═══════════════════════════════════════════════════════════════

        // Load button
        Button {
            id: loadButton
            text: "Load"
            onClicked: toolbar.loadHistoryRequested()

            contentItem: Text {
                text: loadButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            background: Rectangle {
                implicitWidth: 60
                implicitHeight: 34
                color: loadButton.hovered ? Theme.backgroundSecondary : "transparent"
                border.color: loadButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: loadButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Load from History (Ctrl+O)"
            ToolTip.delay: 500
        }

        // Clear button
        Button {
            id: clearButton
            text: "Clear"
            onClicked: toolbar.clearRequested()

            contentItem: Text {
                text: clearButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            background: Rectangle {
                implicitWidth: 60
                implicitHeight: 34
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

        // Indent selector
        ComboBox {
            id: indentCombo
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
                implicitHeight: 34
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

        // Minify button (outline/secondary)
        Button {
            id: minifyButton
            text: "Minify"
            onClicked: toolbar.minifyRequested()

            contentItem: Text {
                text: minifyButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            background: Rectangle {
                implicitWidth: 70
                implicitHeight: 34
                color: minifyButton.hovered ? Theme.backgroundSecondary : "transparent"
                border.color: minifyButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: minifyButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Minify JSON (Ctrl+M)"
            ToolTip.delay: 500
        }

        // FORMAT button (PRIMARY CTA)
        Button {
            id: formatButton
            text: "Format"
            onClicked: toolbar.formatRequested(toolbar.selectedIndent)

            contentItem: Text {
                text: formatButton.text
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            background: Rectangle {
                implicitWidth: 90
                implicitHeight: 36
                color: formatButton.pressed ? Qt.darker(Theme.accent, 1.2) :
                       formatButton.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                border.color: formatButton.activeFocus ? Theme.focusRing : "transparent"
                border.width: formatButton.activeFocus ? Theme.focusRingWidth : 0
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Format JSON (Ctrl+Enter)"
            ToolTip.delay: 500
        }

        Item { Layout.fillWidth: true }  // Right spacer

        // ═══════════════════════════════════════════════════════════════
        // RIGHT ZONE: View & Output Controls
        // ═══════════════════════════════════════════════════════════════

        // Segmented View Mode Control (Code | Tree)
        Row {
            spacing: 0

            Button {
                id: codeViewBtn
                width: 50
                height: 34

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
                        toolbar.viewModeToggled()
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: "Code View (Ctrl+T)"
                ToolTip.delay: 500
            }

            Button {
                id: treeViewBtn
                width: 50
                height: 34

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
                        toolbar.viewModeToggled()
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: "Tree View (Ctrl+T)"
                ToolTip.delay: 500
            }
        }

        // Expand/Collapse buttons (tree mode only)
        Button {
            id: expandAllButton
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
                implicitWidth: 34
                implicitHeight: 34
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
                implicitWidth: 34
                implicitHeight: 34
                color: collapseAllButton.hovered && collapseAllButton.enabled ? Theme.backgroundSecondary : "transparent"
                border.color: collapseAllButton.hovered && collapseAllButton.enabled ? Theme.border : "transparent"
                border.width: 1
                radius: 4
            }

            ToolTip.visible: hovered && enabled
            ToolTip.text: "Collapse All (Ctrl+Shift+E)"
            ToolTip.delay: 500
        }

        // Separator
        Rectangle {
            width: 1
            height: 26
            color: Theme.border
        }

        // Copy Output button (icon only)
        Button {
            id: copyButton
            text: "Copy"
            onClicked: toolbar.copyRequested()

            contentItem: Text {
                text: "⧉"
                font.pixelSize: 16
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitWidth: 38
                implicitHeight: 34
                color: copyButton.hovered ? Theme.backgroundSecondary : "transparent"
                border.color: copyButton.hovered ? Theme.border : "transparent"
                border.width: 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Copy Output (Ctrl+C)"
            ToolTip.delay: 500
        }
    }
}
