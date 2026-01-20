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

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 16

        // Validation Status Badge
        Rectangle {
            width: validationRow.width + 16
            height: 22
            radius: 4
            color: statusBar.isValid ? "#2d4a2d" : "#4a2d2d"

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
                    text: statusBar.isValid ? "Valid JSON" : "Invalid JSON"
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

        // Statistics (shown when valid)
        Text {
            visible: statusBar.isValid
            text: "Objects: " + statusBar.objectCount +
                  " | Arrays: " + statusBar.arrayCount +
                  " | Keys: " + statusBar.totalKeys +
                  " | Depth: " + statusBar.maxDepth
            color: Theme.textSecondary
            font.pixelSize: 12
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
