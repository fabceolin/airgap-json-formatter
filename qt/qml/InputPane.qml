import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: inputPane

    property alias text: textArea.text
    property int errorLine: -1  // -1 = no error
    property string errorMessage: ""

    color: Theme.backgroundSecondary
    border.color: Theme.border
    border.width: 1

    // Calculate line height based on font metrics
    readonly property real lineHeight: textArea.font.pixelSize * 1.4

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 1

        TextArea {
            id: textArea
            color: Theme.textPrimary
            font.family: Theme.monoFont
            font.pixelSize: Theme.monoFontSize
            placeholderText: "Paste or type JSON here..."
            placeholderTextColor: Theme.textSecondary
            background: Rectangle {
                color: "transparent"

                // Error line highlight
                Rectangle {
                    id: errorHighlight
                    visible: inputPane.errorLine > 0
                    x: 0
                    y: (inputPane.errorLine - 1) * inputPane.lineHeight + textArea.topPadding
                    width: parent.width
                    height: inputPane.lineHeight
                    color: "#4a2d2d"
                    opacity: 0.6
                }
            }
            wrapMode: TextEdit.Wrap
            selectByMouse: true

            // Tooltip for error message
            ToolTip {
                id: errorToolTip
                visible: errorMouseArea.containsMouse && inputPane.errorLine > 0 && inputPane.errorMessage
                text: inputPane.errorMessage
                delay: 500

                contentItem: Text {
                    text: errorToolTip.text
                    color: Theme.textPrimary
                    font.pixelSize: 12
                }

                background: Rectangle {
                    color: Theme.backgroundTertiary
                    border.color: Theme.textError
                    border.width: 1
                    radius: 4
                }
            }
        }

        // Mouse area for error line tooltip detection
        MouseArea {
            id: errorMouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            propagateComposedEvents: true

            property bool containsMouse: false

            onPositionChanged: (mouse) => {
                if (inputPane.errorLine > 0) {
                    const errorY = (inputPane.errorLine - 1) * inputPane.lineHeight + textArea.topPadding;
                    containsMouse = mouse.y >= errorY && mouse.y <= errorY + inputPane.lineHeight;
                } else {
                    containsMouse = false;
                }
            }

            onExited: containsMouse = false
        }
    }

    // Scroll to error line
    function scrollToError() {
        if (errorLine > 0) {
            const targetY = (errorLine - 1) * lineHeight;
            const viewportHeight = scrollView.height;
            // Center the error line in the viewport
            const scrollTarget = Math.max(0, targetY - viewportHeight / 2);
            scrollView.contentItem.contentY = scrollTarget;
        }
    }

    onErrorLineChanged: {
        if (errorLine > 0) {
            scrollToError();
        }
    }
}
