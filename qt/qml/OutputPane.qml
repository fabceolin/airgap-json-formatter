import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: outputPane

    property string text: ""  // Plain text for copy operations

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

    onTextChanged: {
        if (text) {
            // Get highlighted HTML from Rust via bridge
            textArea.text = JsonBridge.highlightJson(text);
        } else {
            textArea.text = "";
        }
    }

    // For copy operations, return plain text
    function getPlainText() {
        return text;
    }
}
