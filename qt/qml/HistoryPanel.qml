import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: historyDrawer
    edge: Qt.RightEdge
    width: Math.min(400, parent.width * 0.9)
    height: parent.height

    modal: true
    interactive: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    signal entrySelected(string content)

    // History data model
    property var historyEntries: []
    property string searchQuery: ""
    property bool isLoading: false

    // Filtered entries based on search
    property var filteredEntries: {
        if (!searchQuery.trim()) {
            return historyEntries;
        }
        const query = searchQuery.toLowerCase();
        return historyEntries.filter(function(entry) {
            return entry.preview.toLowerCase().indexOf(query) !== -1;
        });
    }

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1

        // Shadow effect
        Rectangle {
            anchors.right: parent.left
            width: 8
            height: parent.height
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.3) }
            }
        }
    }

    // Load history when drawer opens
    onOpened: {
        loadHistoryData();
        searchField.forceActiveFocus();
    }

    function loadHistoryData() {
        isLoading = true;
        const entries = JsonBridge.loadHistory();
        historyEntries = entries || [];
        isLoading = false;
    }

    function deleteEntry(id) {
        JsonBridge.deleteHistoryEntry(id);
        loadHistoryData();
    }

    function clearAllHistory() {
        JsonBridge.clearHistory();
        historyEntries = [];
    }

    function selectEntry(entry) {
        entrySelected(entry.content);
        historyDrawer.close();
    }

    function formatTimestamp(isoString) {
        const date = new Date(isoString);
        const now = new Date();
        const diffMs = now - date;
        const diffMins = Math.floor(diffMs / 60000);
        const diffHours = Math.floor(diffMs / 3600000);
        const diffDays = Math.floor(diffMs / 86400000);

        if (diffMins < 1) return "Just now";
        if (diffMins < 60) return diffMins + " min ago";
        if (diffHours < 24) return diffHours + " hour" + (diffHours > 1 ? "s" : "") + " ago";
        if (diffDays < 7) return diffDays + " day" + (diffDays > 1 ? "s" : "") + " ago";

        return date.toLocaleDateString();
    }

    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B";
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB";
        return (bytes / (1024 * 1024)).toFixed(1) + " MB";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.backgroundSecondary

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 8
                spacing: 8

                Text {
                    text: "History"
                    font.pixelSize: 18
                    font.bold: true
                    color: Theme.textPrimary
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Clear All"
                    visible: historyEntries.length > 0
                    flat: true
                    font.pixelSize: 12
                    onClicked: confirmClearDialog.open()

                    background: Rectangle {
                        color: parent.hovered ? Theme.backgroundTertiary : "transparent"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: closeButton
                    text: "X"
                    flat: true
                    font.pixelSize: 16
                    font.bold: true
                    implicitWidth: 40
                    implicitHeight: 40
                    onClicked: historyDrawer.close()

                    background: Rectangle {
                        color: parent.hovered ? Theme.backgroundTertiary : "transparent"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    Keys.onReturnPressed: clicked()
                }
            }
        }

        // Search input
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.margins: 8
            color: Theme.backgroundSecondary
            radius: 4
            border.color: searchField.activeFocus ? Theme.accent : Theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Text {
                    text: "\uD83D\uDD0D"
                    font.pixelSize: 14
                    color: Theme.textSecondary
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "Search history..."
                    placeholderTextColor: Theme.textSecondary
                    color: Theme.textPrimary
                    font.pixelSize: 14
                    background: Item {}

                    onTextChanged: {
                        historyDrawer.searchQuery = text;
                    }

                    Keys.onDownPressed: {
                        if (filteredEntries.length > 0) {
                            historyList.forceActiveFocus();
                            historyList.currentIndex = 0;
                        }
                    }

                    Keys.onEscapePressed: {
                        if (text) {
                            text = "";
                        } else {
                            historyDrawer.close();
                        }
                    }
                }

                Button {
                    visible: searchField.text.length > 0
                    text: "X"
                    flat: true
                    implicitWidth: 24
                    implicitHeight: 24
                    onClicked: searchField.text = ""

                    background: Rectangle {
                        color: parent.hovered ? Theme.backgroundTertiary : "transparent"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // History list
        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: filteredEntries
            spacing: 4
            focus: true

            ScrollBar.vertical: ScrollBar {
                active: true
                policy: ScrollBar.AsNeeded
            }

            // Empty state
            Text {
                visible: filteredEntries.length === 0 && !isLoading
                anchors.centerIn: parent
                text: historyEntries.length === 0 ? "No history yet\nFormat some JSON to get started" : "No matching entries"
                color: Theme.textSecondary
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
            }

            // Loading state
            BusyIndicator {
                visible: isLoading
                anchors.centerIn: parent
                running: visible
            }

            delegate: Rectangle {
                id: entryDelegate
                width: historyList.width - 16
                x: 8
                height: 80
                color: ListView.isCurrentItem ? Theme.backgroundTertiary :
                       mouseArea.containsMouse ? Theme.backgroundSecondary : "transparent"
                radius: 4
                border.color: ListView.isCurrentItem ? Theme.accent : "transparent"
                border.width: 1

                required property var modelData
                required property int index

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: selectEntry(modelData)
                    onDoubleClicked: selectEntry(modelData)
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    // Preview text
                    Text {
                        Layout.fillWidth: true
                        text: modelData.preview || ""
                        color: Theme.textPrimary
                        font.family: Theme.monoFont
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WrapAnywhere
                    }

                    // Metadata row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: formatTimestamp(modelData.timestamp)
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }

                        Text {
                            text: formatSize(modelData.size)
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }

                        Item { Layout.fillWidth: true }

                        // Delete button
                        Button {
                            id: deleteBtn
                            text: "Delete"
                            flat: true
                            font.pixelSize: 11
                            visible: mouseArea.containsMouse || entryDelegate.ListView.isCurrentItem
                            implicitHeight: 24
                            onClicked: {
                                deleteEntry(modelData.id);
                            }

                            background: Rectangle {
                                color: parent.hovered ? Qt.rgba(1, 0.3, 0.3, 0.2) : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: Theme.textError
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                // Keyboard navigation
                Keys.onReturnPressed: selectEntry(modelData)
                Keys.onEnterPressed: selectEntry(modelData)
                Keys.onDeletePressed: deleteEntry(modelData.id)
            }

            // Keyboard navigation
            Keys.onUpPressed: {
                if (currentIndex > 0) {
                    currentIndex--;
                } else {
                    searchField.forceActiveFocus();
                }
            }
            Keys.onDownPressed: {
                if (currentIndex < count - 1) {
                    currentIndex++;
                }
            }
            Keys.onEscapePressed: historyDrawer.close()
        }

        // Entry count footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: Theme.backgroundSecondary
            visible: historyEntries.length > 0

            Text {
                anchors.centerIn: parent
                text: filteredEntries.length + " of " + historyEntries.length + " entries"
                color: Theme.textSecondary
                font.pixelSize: 11
            }
        }
    }

    // Confirm clear dialog
    Dialog {
        id: confirmClearDialog
        title: "Clear History"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        background: Rectangle {
            color: Theme.background
            border.color: Theme.border
            radius: 8
        }

        header: Rectangle {
            color: Theme.backgroundSecondary
            height: 48
            radius: 8

            Text {
                anchors.centerIn: parent
                text: "Clear History"
                font.pixelSize: 16
                font.bold: true
                color: Theme.textPrimary
            }
        }

        contentItem: Text {
            text: "Are you sure you want to clear all history?\nThis action cannot be undone."
            color: Theme.textPrimary
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            padding: 20
        }

        onAccepted: clearAllHistory()
    }
}
