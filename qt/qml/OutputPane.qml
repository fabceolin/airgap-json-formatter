import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: outputPane

    property alias text: textArea.text

    color: Theme.backgroundSecondary
    border.color: Theme.border
    border.width: 1

    ScrollView {
        anchors.fill: parent
        anchors.margins: 1

        TextArea {
            id: textArea
            color: Theme.textPrimary
            font.family: Theme.monoFont
            font.pixelSize: Theme.monoFontSize
            placeholderText: "Formatted output will appear here"
            placeholderTextColor: Theme.textSecondary
            readOnly: true
            background: Rectangle {
                color: "transparent"
            }
            wrapMode: TextEdit.Wrap
            selectByMouse: true
        }
    }
}
