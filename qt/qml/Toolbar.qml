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
        spacing: 12

        Button {
            id: formatButton
            text: "Format"
            onClicked: toolbar.formatRequested(toolbar.selectedIndent)

            contentItem: Text {
                text: formatButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            background: Rectangle {
                implicitWidth: 80
                implicitHeight: 34
                color: formatButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: formatButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: formatButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }
        }

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
                implicitWidth: 80
                implicitHeight: 34
                color: minifyButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: minifyButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: minifyButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }
        }

        Rectangle {
            width: 1
            height: 30
            color: Theme.border
        }

        Label {
            text: "Indent:"
            color: Theme.textSecondary
            font.pixelSize: 13
        }

        ComboBox {
            id: indentCombo
            model: ["2 spaces", "4 spaces", "Tabs"]
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
                font.pixelSize: 13
            }

            background: Rectangle {
                implicitWidth: 100
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
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: highlighted ? Theme.accent : "transparent"
                }
                highlighted: indentCombo.highlightedIndex === index
            }
        }

        Item { Layout.fillWidth: true }  // Spacer

        // View mode toggle
        Button {
            id: viewToggleButton
            text: toolbar.viewMode === "tree" ? "Tree" : "Text"
            onClicked: toolbar.viewModeToggled()

            contentItem: Text {
                text: viewToggleButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            background: Rectangle {
                implicitWidth: 70
                implicitHeight: 34
                color: viewToggleButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: viewToggleButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: viewToggleButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: toolbar.viewMode === "tree" ? "Switch to Text View (Ctrl+T)" : "Switch to Tree View (Ctrl+T)"
            ToolTip.delay: 500
        }

        Rectangle {
            width: 1
            height: 30
            color: Theme.border
        }

        // Expand All button (only enabled in tree mode)
        Button {
            id: expandAllButton
            text: "[+]"
            enabled: toolbar.viewMode === "tree"
            opacity: enabled ? 1.0 : 0.5
            onClicked: toolbar.expandAllRequested()

            contentItem: Text {
                text: expandAllButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
                font.family: Theme.monoFont
            }
            background: Rectangle {
                implicitWidth: 40
                implicitHeight: 34
                color: expandAllButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: expandAllButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: expandAllButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Expand All (Ctrl+E)"
            ToolTip.delay: 500
        }

        // Collapse All button (only enabled in tree mode)
        Button {
            id: collapseAllButton
            text: "[-]"
            enabled: toolbar.viewMode === "tree"
            opacity: enabled ? 1.0 : 0.5
            onClicked: toolbar.collapseAllRequested()

            contentItem: Text {
                text: collapseAllButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
                font.family: Theme.monoFont
            }
            background: Rectangle {
                implicitWidth: 40
                implicitHeight: 34
                color: collapseAllButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: collapseAllButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: collapseAllButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Collapse All (Ctrl+Shift+E)"
            ToolTip.delay: 500
        }

        Rectangle {
            width: 1
            height: 30
            color: Theme.border
        }

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
                implicitWidth: 70
                implicitHeight: 34
                color: loadButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: loadButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: loadButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            ToolTip.visible: hovered
            ToolTip.text: "Load from History (Ctrl+O)"
            ToolTip.delay: 500
        }

        Button {
            id: copyButton
            text: "Copy Output"
            onClicked: toolbar.copyRequested()

            contentItem: Text {
                text: copyButton.text
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 34
                color: copyButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: copyButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: copyButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }
        }

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
                implicitWidth: 70
                implicitHeight: 34
                color: clearButton.pressed ? Theme.accent : Theme.backgroundSecondary
                border.color: clearButton.activeFocus ? Theme.focusRing : Theme.border
                border.width: clearButton.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }
        }
    }
}
