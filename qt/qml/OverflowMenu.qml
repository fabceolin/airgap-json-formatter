import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

// Overflow menu for mobile - slide-in drawer from right
Item {
    id: overflowMenu

    // Menu state
    property bool isOpen: false

    // Expose for testing (UNIT-013, UNIT-014)
    function open() {
        isOpen = true;
    }

    function close() {
        isOpen = false;
    }

    // Current indent setting (synced from toolbar)
    property string currentIndent: "spaces:4"

    // Current view mode
    property string viewMode: "tree"

    // Signals for menu actions (UNIT-016)
    signal onLoad()
    signal onClear()
    signal onMinify()
    signal onCopy()
    signal onIndentChange(string indent)
    signal onViewToggle()
    signal onThemeToggle()
    signal onExpandAll()
    signal onCollapseAll()

    // Menu item height (touch-friendly)
    readonly property int menuItemHeight: 48

    // Semi-transparent backdrop
    Rectangle {
        id: backdrop
        anchors.fill: parent
        color: "#000000"
        opacity: overflowMenu.isOpen ? 0.5 : 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: overflowMenu.close()  // UNIT-014: Close on backdrop tap
        }
    }

    // Slide-in menu panel from right
    Rectangle {
        id: menuPanel
        width: Math.min(280, parent.width * 0.8)
        height: parent.height
        anchors.right: parent.right
        anchors.rightMargin: overflowMenu.isOpen ? 0 : -width
        color: Theme.backgroundSecondary

        Behavior on anchors.rightMargin {
            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
        }

        // Left border
        Rectangle {
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: Theme.border
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            // Menu header
            Rectangle {
                Layout.fillWidth: true
                height: overflowMenu.menuItemHeight
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Text {
                        text: "Menu"
                        color: Theme.textPrimary
                        font.pixelSize: 18
                        font.weight: Font.Medium
                    }

                    Item { Layout.fillWidth: true }

                    // Close button
                    Rectangle {
                        width: 36
                        height: 36
                        radius: 18
                        color: closeMouseArea.pressed ? Theme.backgroundTertiary : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: Theme.textSecondary
                            font.pixelSize: 18
                        }

                        MouseArea {
                            id: closeMouseArea
                            anchors.fill: parent
                            onClicked: overflowMenu.close()
                        }
                    }
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // Menu items (UNIT-015: All items present)
            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentHeight: menuItemsColumn.height
                clip: true

                ColumnLayout {
                    id: menuItemsColumn
                    width: parent.width
                    spacing: 2

                    // Load
                    MenuItem {
                        icon: "📁"
                        label: "Load from History"
                        onClicked: {
                            overflowMenu.onLoad();
                            overflowMenu.close();
                        }
                    }

                    // Clear
                    MenuItem {
                        icon: "🗑"
                        label: "Clear All"
                        onClicked: {
                            overflowMenu.onClear();
                            overflowMenu.close();
                        }
                    }

                    // Separator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        height: 1
                        color: Theme.border
                    }

                    // Minify
                    MenuItem {
                        icon: "−"
                        label: "Minify"
                        onClicked: {
                            overflowMenu.onMinify();
                            overflowMenu.close();
                        }
                    }

                    // Copy
                    MenuItem {
                        icon: "📋"
                        label: "Copy Output"
                        onClicked: {
                            overflowMenu.onCopy();
                            overflowMenu.close();
                        }
                    }

                    // Separator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        height: 1
                        color: Theme.border
                    }

                    // Indent options
                    Text {
                        Layout.leftMargin: 12
                        Layout.topMargin: 8
                        text: "Indentation"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    MenuItem {
                        icon: overflowMenu.currentIndent === "spaces:2" ? "●" : "○"
                        label: "2 Spaces"
                        onClicked: {
                            overflowMenu.onIndentChange("spaces:2");
                            overflowMenu.close();
                        }
                    }

                    MenuItem {
                        icon: overflowMenu.currentIndent === "spaces:4" ? "●" : "○"
                        label: "4 Spaces"
                        onClicked: {
                            overflowMenu.onIndentChange("spaces:4");
                            overflowMenu.close();
                        }
                    }

                    MenuItem {
                        icon: overflowMenu.currentIndent === "tabs" ? "●" : "○"
                        label: "Tabs"
                        onClicked: {
                            overflowMenu.onIndentChange("tabs");
                            overflowMenu.close();
                        }
                    }

                    // Separator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        height: 1
                        color: Theme.border
                    }

                    // View toggle
                    MenuItem {
                        icon: overflowMenu.viewMode === "tree" ? "🌲" : "📝"
                        label: overflowMenu.viewMode === "tree" ? "Switch to Code View" : "Switch to Tree View"
                        onClicked: {
                            overflowMenu.onViewToggle();
                            overflowMenu.close();
                        }
                    }

                    // Expand/Collapse (only in tree mode)
                    MenuItem {
                        visible: overflowMenu.viewMode === "tree"
                        icon: "⊞"
                        label: "Expand All"
                        onClicked: {
                            overflowMenu.onExpandAll();
                            overflowMenu.close();
                        }
                    }

                    MenuItem {
                        visible: overflowMenu.viewMode === "tree"
                        icon: "⊟"
                        label: "Collapse All"
                        onClicked: {
                            overflowMenu.onCollapseAll();
                            overflowMenu.close();
                        }
                    }

                    // Separator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        height: 1
                        color: Theme.border
                    }

                    // Theme toggle
                    MenuItem {
                        icon: Theme.darkMode ? "☀" : "🌙"
                        label: Theme.darkMode ? "Light Mode" : "Dark Mode"
                        onClicked: {
                            overflowMenu.onThemeToggle();
                            overflowMenu.close();
                        }
                    }

                    // Spacer at bottom
                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: 20
                    }
                }
            }
        }
    }

    // Menu item component
    component MenuItem: Rectangle {
        property string icon: ""
        property string label: ""
        signal clicked()

        Layout.fillWidth: true
        height: overflowMenu.menuItemHeight
        radius: 8
        color: itemMouseArea.pressed ? Theme.backgroundTertiary : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 12

            Text {
                text: icon
                font.pixelSize: 18
                Layout.preferredWidth: 24
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: label
                color: Theme.textPrimary
                font.pixelSize: Theme.mobileFontSize
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: itemMouseArea
            anchors.fill: parent
            onClicked: parent.clicked()
        }
    }
}
