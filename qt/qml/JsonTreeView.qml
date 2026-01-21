import QtQuick
import QtQuick.Controls
import AirgapFormatter

Rectangle {
    id: root

    property alias model: treeView.model
    property int animationDuration: 150

    // Auto-expand configuration
    property bool autoExpandOnLoad: true
    property int maxAutoExpandNodes: 1000
    property int defaultExpandDepth: 3

    color: Theme.backgroundSecondary
    border.color: Theme.border
    border.width: 1

    function expandAll() {
        for (let row = 0; row < treeView.rows; row++) {
            treeView.expandRecursively(row, -1)
        }
    }

    function collapseAll() {
        treeView.collapseRecursively()
    }

    function expandToLevel(maxDepth) {
        // Use TreeView's built-in expandRecursively with depth limit
        for (let row = 0; row < treeView.rows; row++) {
            treeView.expandRecursively(row, maxDepth)
        }
    }

    function performAutoExpand() {
        if (!autoExpandOnLoad) return
        if (!treeView.model) return

        const nodeCount = treeView.model.totalNodeCount()
        if (nodeCount <= 0) return

        Qt.callLater(function() {
            if (nodeCount <= maxAutoExpandNodes) {
                expandAll()
            } else {
                expandToLevel(defaultExpandDepth)
            }
        })
    }

    // Listen for model data reset to trigger auto-expand
    Connections {
        target: treeView.model
        function onModelReset() {
            if (root.autoExpandOnLoad) {
                root.performAutoExpand()
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 1

        TreeView {
            id: treeView
            anchors.fill: parent
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            reuseItems: true

            model: JsonBridge.treeModel

            selectionModel: ItemSelectionModel {
                model: treeView.model
            }

            delegate: Item {
                id: delegateRoot
                implicitWidth: treeView.width
                implicitHeight: Math.max(contentRow.height, 24) + (showClosingBracket ? 22 : 0)

                required property TreeView treeView
                required property bool isTreeNode
                required property bool expanded
                required property int hasChildren
                required property int depth
                required property int row
                required property int column
                required property bool current
                required property bool selected

                // Model data
                required property string key
                required property var value
                required property string valueType
                required property string jsonPath
                required property int childCount
                required property bool isExpandable
                required property bool isLastChild
                required property string parentValueType

                property bool hovered: mouseArea.containsMouse
                property int indent: depth * 20
                // Show closing bracket when this is the last child of an expanded object/array
                property bool showClosingBracket: delegateRoot.isLastChild &&
                    (delegateRoot.parentValueType === "object" || delegateRoot.parentValueType === "array")

                Rectangle {
                    anchors.fill: parent
                    color: (delegateRoot.hovered || copyIcons.buttonsHovered) ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.targetRow = delegateRoot.row
                            contextMenu.targetJsonPath = delegateRoot.jsonPath
                            contextMenu.targetValue = delegateRoot.value
                            contextMenu.targetValueType = delegateRoot.valueType
                            contextMenu.popup()
                        }
                    }

                    onDoubleClicked: {
                        if (delegateRoot.isExpandable) {
                            treeView.toggleExpanded(delegateRoot.row)
                        }
                    }
                }

                Row {
                    id: contentRow
                    x: delegateRoot.indent + 4
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    height: 22

                    // Expand/Collapse indicator
                    Item {
                        width: 16
                        height: parent.height
                        visible: delegateRoot.isExpandable

                        Text {
                            anchors.centerIn: parent
                            text: delegateRoot.expanded ? "\u25BC" : "\u25B6"
                            color: Theme.syntaxPunctuation
                            font.pixelSize: 8
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: treeView.toggleExpanded(delegateRoot.row)
                        }
                    }

                    // Spacer when not expandable
                    Item {
                        width: 16
                        height: parent.height
                        visible: !delegateRoot.isExpandable
                    }

                    // Key display
                    Text {
                        id: keyText
                        visible: delegateRoot.key !== ""
                        text: {
                            // Array indices don't need quotes
                            if (delegateRoot.treeView.model && delegateRoot.depth > 0) {
                                const parentIndex = treeView.index(delegateRoot.row, 0)
                                const parent = treeView.model.parent(parentIndex)
                                // Check if parent is array by looking at the key pattern
                                if (/^\d+$/.test(delegateRoot.key)) {
                                    return "[" + delegateRoot.key + "]"
                                }
                            }
                            return "\"" + delegateRoot.key + "\""
                        }
                        color: /^\d+$/.test(delegateRoot.key) ? Theme.syntaxNumber : Theme.syntaxKey
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.monoFontSize
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // Colon separator
                    Text {
                        visible: delegateRoot.key !== ""
                        text: ": "
                        color: Theme.syntaxPunctuation
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.monoFontSize
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // Value display
                    Loader {
                        id: valueLoader
                        anchors.verticalCenter: parent.verticalCenter
                        sourceComponent: {
                            switch (delegateRoot.valueType) {
                                case "object": return objectComponent
                                case "array": return arrayComponent
                                case "string": return stringComponent
                                case "number": return numberComponent
                                case "boolean": return booleanComponent
                                case "null": return nullComponent
                                default: return nullComponent
                            }
                        }
                    }

                    // Child count badge (when collapsed)
                    Text {
                        id: countBadge
                        visible: !delegateRoot.expanded && delegateRoot.isExpandable
                        text: {
                            if (delegateRoot.valueType === "object") {
                                return "(" + delegateRoot.childCount + (delegateRoot.childCount === 1 ? " key)" : " keys)")
                            } else if (delegateRoot.valueType === "array") {
                                return "(" + delegateRoot.childCount + (delegateRoot.childCount === 1 ? " item)" : " items)")
                            }
                            return ""
                        }
                        color: Theme.syntaxBadge
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.monoFontSize - 2
                        font.italic: true
                        anchors.verticalCenter: parent.verticalCenter
                    }

                }

                // Closing bracket for parent container (rendered after last child)
                Text {
                    id: closingBracket
                    visible: delegateRoot.showClosingBracket
                    anchors.top: contentRow.bottom
                    anchors.topMargin: 2
                    // Parent's indentation: (depth - 1) * 20 + 4 (margin) + 16 (expand icon space)
                    x: (delegateRoot.depth - 1) * 20 + 20
                    text: delegateRoot.parentValueType === "object" ? "}" : "]"
                    color: Theme.syntaxPunctuation
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                }

                // Copy icons on hover
                Row {
                    id: copyIcons
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    // Keep visible if row is hovered OR if any button inside is hovered
                    property bool buttonsHovered: copyValueHover.hovered || copyPathHover.hovered
                    opacity: (delegateRoot.hovered || buttonsHovered) ? 1.0 : 0.0
                    visible: opacity > 0

                    Behavior on opacity {
                        NumberAnimation { duration: 150 }
                    }

                    // Copy Value button
                    Rectangle {
                        width: 22
                        height: 22
                        radius: 3
                        color: copyValueHover.hovered ? Theme.backgroundTertiary : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "[V]"
                            font.pixelSize: 9
                            color: Theme.textSecondary
                        }

                        HoverHandler {
                            id: copyValueHover
                            cursorShape: Qt.PointingHandCursor
                        }

                        TapHandler {
                            onTapped: root.copyValue(delegateRoot.row)
                        }

                        ToolTip {
                            visible: copyValueHover.hovered
                            text: "Copy Value"
                            delay: 500
                        }
                    }

                    // Copy Path button
                    Rectangle {
                        width: 22
                        height: 22
                        radius: 3
                        color: copyPathHover.hovered ? Theme.backgroundTertiary : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "[P]"
                            font.pixelSize: 9
                            color: Theme.textSecondary
                        }

                        HoverHandler {
                            id: copyPathHover
                            cursorShape: Qt.PointingHandCursor
                        }

                        TapHandler {
                            onTapped: root.copyPath(delegateRoot.jsonPath)
                        }

                        ToolTip {
                            visible: copyPathHover.hovered
                            text: "Copy Path"
                            delay: 500
                        }
                    }
                }
            }

            // Value type components
            Component {
                id: objectComponent
                Text {
                    // Show {} for empty objects, { for non-empty
                    property int delegateChildCount: parent ? parent.parent.parent.childCount : 0
                    text: delegateChildCount === 0 ? "{}" : "{"
                    color: Theme.syntaxPunctuation
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                }
            }

            Component {
                id: arrayComponent
                Text {
                    // Show [] for empty arrays, [ for non-empty
                    property int delegateChildCount: parent ? parent.parent.parent.childCount : 0
                    text: delegateChildCount === 0 ? "[]" : "["
                    color: Theme.syntaxPunctuation
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                }
            }

            Component {
                id: stringComponent
                Text {
                    property var delegateValue: parent ? parent.parent.parent.value : ""
                    text: "\"" + String(delegateValue || "").substring(0, 100) + (String(delegateValue || "").length > 100 ? "..." : "") + "\""
                    color: Theme.syntaxString
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            Component {
                id: numberComponent
                Text {
                    property var delegateValue: parent ? parent.parent.parent.value : 0
                    text: String(delegateValue)
                    color: Theme.syntaxNumber
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                }
            }

            Component {
                id: booleanComponent
                Text {
                    property var delegateValue: parent ? parent.parent.parent.value : false
                    text: delegateValue ? "true" : "false"
                    color: Theme.syntaxBoolean
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                }
            }

            Component {
                id: nullComponent
                Text {
                    text: "null"
                    color: Theme.syntaxNull
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoFontSize
                }
            }
        }
    }

    // Context menu
    Menu {
        id: contextMenu
        property int targetRow: -1
        property string targetJsonPath: ""
        property var targetValue
        property string targetValueType: ""

        MenuItem {
            text: "Copy Value"
            onTriggered: root.copyValue(contextMenu.targetRow)
        }

        MenuItem {
            text: "Copy Path"
            onTriggered: root.copyPath(contextMenu.targetJsonPath)
        }

        MenuSeparator {}

        MenuItem {
            text: "Expand All"
            onTriggered: root.expandAll()
        }

        MenuItem {
            text: "Collapse All"
            onTriggered: root.collapseAll()
        }
    }

    // Copy feedback toast
    Rectangle {
        id: copyFeedback
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        width: copyFeedbackText.width + 24
        height: 32
        radius: 4
        color: Theme.accent
        opacity: 0
        visible: opacity > 0

        Text {
            id: copyFeedbackText
            anchors.centerIn: parent
            text: "Copied!"
            color: "white"
            font.pixelSize: 13
        }

        Timer {
            id: feedbackTimer
            interval: 1500
            onTriggered: copyFeedback.opacity = 0
        }

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }

    function copyValue(row) {
        const index = treeView.index(row, 0)
        if (index.valid) {
            const serialized = JsonBridge.treeModel.serializeNode(index)
            JsonBridge.copyToClipboard(serialized)
            showCopyFeedback()
        }
    }

    function copyPath(path) {
        JsonBridge.copyToClipboard(path)
        showCopyFeedback()
    }

    function showCopyFeedback() {
        copyFeedback.opacity = 1
        feedbackTimer.restart()
    }
}
