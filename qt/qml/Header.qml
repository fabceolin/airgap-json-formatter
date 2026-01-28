import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Rectangle {
    id: header
    height: window.isMobile ? 48 : 40  // Slightly taller on mobile
    color: Theme.background

    property bool offlineReady: false

    // Responsive mode (reference window breakpoints)
    readonly property bool mobileMode: typeof window !== "undefined" && window.isMobile

    // Subtle bottom border
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: header.mobileMode ? 12 : 16
        anchors.rightMargin: header.mobileMode ? 12 : 16
        spacing: header.mobileMode ? 8 : 16

        // App Title - shortened on mobile
        Text {
            text: header.mobileMode ? "Airgap" : "Airgap JSON Formatter"
            color: Theme.textPrimary
            font.pixelSize: header.mobileMode ? 16 : 18
            font.weight: Font.Medium
        }

        // Version badge - hidden on mobile
        Text {
            visible: !header.mobileMode
            text: "v" + Theme.appVersion
            color: Theme.textSecondary
            font.pixelSize: 11
            Layout.alignment: Qt.AlignVCenter
        }

        Item { Layout.fillWidth: true }  // Spacer

        // Airgap Protected Badge - Always visible (security trust indicator)
        // Compact version on mobile (icon only), full version on desktop
        Rectangle {
            id: airgapBadge
            width: header.mobileMode ? 36 : airgapRow.width + 16
            height: header.mobileMode ? 36 : 26
            radius: 4
            color: Theme.badgeSuccessBg
            border.color: Theme.badgeSuccessBorder
            border.width: 1

            // Mobile: Icon only (centered)
            Text {
                id: airgapIconOnly
                visible: header.mobileMode
                anchors.centerIn: parent
                text: "🔒"
                font.pixelSize: 16
            }

            // Desktop: Icon + text
            Row {
                id: airgapRow
                visible: !header.mobileMode
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
                id: airgapTooltip
                visible: airgapBadgeMouseArea.containsMouse
                text: "All data processing happens locally in your browser.\nNo data is ever sent to external servers."
                delay: 500

                contentItem: Text {
                    text: airgapTooltip.text
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

        // Offline Ready Indicator - hidden on mobile
        Rectangle {
            id: offlineBadge
            visible: !header.mobileMode && header.offlineReady
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
                id: offlineTooltip
                visible: offlineBadgeMouseArea.containsMouse
                text: "Application is fully loaded and can work without internet connection."
                delay: 500

                contentItem: Text {
                    text: offlineTooltip.text
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

        // Theme Toggle Button - hidden on mobile (moved to overflow menu)
        Rectangle {
            id: themeToggle
            visible: !header.mobileMode
            width: 32
            height: 26
            radius: 4
            color: themeToggleMouseArea.containsMouse ? Theme.backgroundTertiary : "transparent"
            border.color: Theme.border
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "◐"
                color: Theme.textPrimary
                font.pixelSize: 16
            }

            MouseArea {
                id: themeToggleMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Theme.toggleTheme()
            }

            ToolTip {
                id: themeTooltip
                visible: themeToggleMouseArea.containsMouse
                text: Theme.darkMode ? "Switch to Light Mode" : "Switch to Dark Mode"
                delay: 500

                contentItem: Text {
                    text: themeTooltip.text
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
        }
    }
}
