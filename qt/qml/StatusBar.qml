import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Rectangle {
    id: statusBar
    height: 30
    color: Theme.backgroundTertiary

    property bool isValid: true
    property string errorMessage: ""
    property int errorLine: 0
    property int errorColumn: 0

    // Statistics
    property int objectCount: 0
    property int arrayCount: 0
    property int stringCount: 0
    property int numberCount: 0
    property int booleanCount: 0
    property int nullCount: 0
    property int totalKeys: 0
    property int maxDepth: 0

    // Story 8.4: Detected format ("json", "xml", "unknown")
    property string detectedFormat: "json"

    // Story 9.3: Temporary status message (for share feedback, etc.)
    property string statusMessage: ""
    property string statusType: ""  // "success", "error", or ""

    // Timer to auto-clear status message
    Timer {
        id: statusMessageTimer
        interval: 5000
        onTriggered: {
            statusBar.statusMessage = "";
            statusBar.statusType = "";
        }
    }

    // Function to show a temporary status message
    function showMessage(message, type) {
        statusMessage = message;
        statusType = type || "";
        statusMessageTimer.restart();
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 16

        // Story 8.4: Format indicator badge
        Rectangle {
            width: formatBadgeRow.width + 12
            height: 22
            radius: 4
            color: Theme.backgroundSecondary

            Row {
                id: formatBadgeRow
                anchors.centerIn: parent
                spacing: 4

                Text {
                    text: statusBar.detectedFormat === "json" ? "JSON" :
                          statusBar.detectedFormat === "xml" ? "XML" : "?"
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }
            }
        }

        // Validation Status Badge
        Rectangle {
            width: validationRow.width + 16
            height: 22
            radius: 4
            color: statusBar.isValid ? Theme.badgeSuccessBg : Theme.badgeErrorBg

            Row {
                id: validationRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: statusBar.isValid ? "✓" : "✗"
                    color: statusBar.isValid ? Theme.textSuccess : Theme.textError
                    font.pixelSize: 12
                }

                Text {
                    // Story 8.4: Update validation text based on detected format
                    text: statusBar.isValid
                        ? (statusBar.detectedFormat === "xml" ? "Valid XML" : "Valid JSON")
                        : (statusBar.detectedFormat === "xml" ? "Invalid XML" : "Invalid JSON")
                    color: statusBar.isValid ? Theme.textSuccess : Theme.textError
                    font.pixelSize: 12
                }
            }
        }

        // Error Message (shown when invalid)
        Text {
            visible: !statusBar.isValid && statusBar.errorMessage
            text: "Line " + statusBar.errorLine + ", Col " + statusBar.errorColumn + ": " + statusBar.errorMessage
            color: Theme.textError
            font.pixelSize: 12
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        // Statistics (shown when valid and no status message)
        Text {
            visible: statusBar.isValid && statusBar.statusMessage === ""
            text: "Objects: " + statusBar.objectCount +
                  " | Arrays: " + statusBar.arrayCount +
                  " | Keys: " + statusBar.totalKeys +
                  " | Depth: " + statusBar.maxDepth
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        // Story 9.3: Temporary status message (for share, load shared, etc.)
        Text {
            visible: statusBar.statusMessage !== ""
            text: statusBar.statusMessage
            color: statusBar.statusType === "success" ? Theme.textSuccess :
                   statusBar.statusType === "error" ? Theme.textError : Theme.textSecondary
            font.pixelSize: 12
            font.weight: Font.Medium
        }

        Item { Layout.fillWidth: true }  // Spacer

        // Privacy Footer Text
        Text {
            text: "All processing happens locally. Your data never leaves this device."
            color: Theme.textSecondary
            font.pixelSize: 11
            opacity: 0.8
        }
    }
}
