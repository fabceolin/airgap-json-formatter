import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: outputPane

    property string rawText: ""  // Plain text for copy operations
    property alias text: rawText

    color: Theme.backgroundSecondary
    border.color: Theme.border
    border.width: 1

    ScrollView {
        anchors.fill: parent
        anchors.margins: 1

        TextArea {
            id: textArea
            textFormat: TextEdit.RichText  // Enable HTML rendering
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

    onRawTextChanged: {
        if (rawText) {
            // Get highlighted HTML from Rust via bridge
            textArea.text = JsonBridge.highlightJson(rawText);
        } else {
            textArea.text = "";
        }
    }

    // For copy operations, return plain text
    function getPlainText() {
        return rawText;
    }
}
