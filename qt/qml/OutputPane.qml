import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: outputPane

    property string text: ""  // Plain text for copy operations
    property string format: "json"  // "json", "xml", or "markdown" - used for highlighting

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
            // Responsive font size: minimum 14px on mobile (AC: 7)
            font.pixelSize: Math.max(Theme.monoFontSize, Theme.mobileFontSize)
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
        updateHighlighting();
    }

    onFormatChanged: {
        updateHighlighting();
    }

    // Update syntax highlighting based on format (AC: 6 - Code view shows raw HTML)
    function updateHighlighting() {
        if (!text) {
            textArea.text = "";
            return;
        }

        // Story 10.5: For markdown format in Code view, show raw HTML with syntax highlighting
        if (format === "markdown") {
            // The text contains rendered HTML - highlight it as HTML/XML
            textArea.text = JsonBridge.highlightXml(text);
        } else if (format === "xml") {
            textArea.text = JsonBridge.highlightXml(text);
        } else {
            // Default: JSON highlighting
            textArea.text = JsonBridge.highlightJson(text);
        }
    }

    // For copy operations, return plain text
    function getPlainText() {
        return text;
    }
}
