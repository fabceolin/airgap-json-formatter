import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Rectangle {
    id: header
    height: 40
    color: Theme.background

    property bool offlineReady: false

    // Subtle bottom border
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        // App Title
        Text {
            text: "Airgap JSON Formatter"
            color: Theme.textPrimary
            font.pixelSize: 18
            font.weight: Font.Medium
        }

        Item { Layout.fillWidth: true }  // Spacer

        // Airgap Protected Badge
        Rectangle {
            id: airgapBadge
            width: airgapRow.width + 16
            height: 26
            radius: 4
            color: "#1a3a1a"
            border.color: "#2d5a2d"
            border.width: 1

            Row {
                id: airgapRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: "🔒"
                    font.pixelSize: 14
                }

                Text {
                    text: "Airgap Protected"
                    color: Theme.textSuccess
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }

            ToolTip {
                visible: airgapBadgeMouseArea.containsMouse
                text: "All data processing happens locally in your browser.\nNo data is ever sent to external servers."
                delay: 500

                contentItem: Text {
                    text: parent.text
                    color: Theme.textPrimary
                    font.pixelSize: 12
                }

                background: Rectangle {
                    color: Theme.backgroundTertiary
                    border.color: Theme.border
                    border.width: 1
                    radius: 4
                }
            }

            MouseArea {
                id: airgapBadgeMouseArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }

        // Offline Ready Indicator
        Rectangle {
            id: offlineBadge
            visible: header.offlineReady
            width: offlineRow.width + 12
            height: 26
            radius: 4
            color: "transparent"
            border.color: Theme.border
            border.width: 1

            Row {
                id: offlineRow
                anchors.centerIn: parent
                spacing: 4

                Text {
                    text: "✓"
                    color: Theme.textSuccess
                    font.pixelSize: 12
                }

                Text {
                    text: "Offline Ready"
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }
            }

            ToolTip {
                visible: offlineBadgeMouseArea.containsMouse
                text: "Application is fully loaded and can work without internet connection."
                delay: 500

                contentItem: Text {
                    text: parent.text
                    color: Theme.textPrimary
                    font.pixelSize: 12
                }

                background: Rectangle {
                    color: Theme.backgroundTertiary
                    border.color: Theme.border
                    border.width: 1
                    radius: 4
                }
            }

            MouseArea {
                id: offlineBadgeMouseArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }
}
