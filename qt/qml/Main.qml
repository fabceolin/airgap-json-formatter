import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 600
    title: "Airgap JSON Formatter"

    color: Theme.background

    // Track WASM initialization state
    property bool wasmInitialized: false

    // Track when JsonBridge becomes ready
    Connections {
        target: JsonBridge
        function onReadyChanged() {
            wasmInitialized = JsonBridge.ready;
        }
    }

    // Initialize WASM on component completion
    Component.onCompleted: {
        // Check if bridge is ready via C++ context property
        wasmInitialized = JsonBridge.ready;
    }

    // Timer for copy feedback reset
    Timer {
        id: copyFeedbackTimer
        interval: 1500
        onTriggered: toolbar.copyButtonText = "Copy Output"
    }

    // Debounce timer for validation
    Timer {
        id: validationTimer
        interval: 300
        onTriggered: validateInput()
    }

    function validateInput() {
        if (!inputPane.text || !inputPane.text.trim()) {
            statusBar.isValid = true;
            statusBar.errorMessage = "";
            statusBar.objectCount = 0;
            statusBar.arrayCount = 0;
            statusBar.totalKeys = 0;
            statusBar.maxDepth = 0;
            inputPane.errorLine = -1;
            inputPane.errorMessage = "";
            return;
        }

        const result = JsonBridge.validateJson(inputPane.text);

        statusBar.isValid = result.isValid;

        if (result.isValid) {
            statusBar.errorMessage = "";
            statusBar.errorLine = 0;
            statusBar.errorColumn = 0;
            statusBar.objectCount = result.stats.object_count || 0;
            statusBar.arrayCount = result.stats.array_count || 0;
            statusBar.stringCount = result.stats.string_count || 0;
            statusBar.numberCount = result.stats.number_count || 0;
            statusBar.booleanCount = result.stats.boolean_count || 0;
            statusBar.nullCount = result.stats.null_count || 0;
            statusBar.totalKeys = result.stats.total_keys || 0;
            statusBar.maxDepth = result.stats.max_depth || 0;
            inputPane.errorLine = -1;
            inputPane.errorMessage = "";
        } else if (result.error) {
            statusBar.errorMessage = result.error.message || "Unknown error";
            statusBar.errorLine = result.error.line || 1;
            statusBar.errorColumn = result.error.column || 1;
            inputPane.errorLine = result.error.line || 1;
            inputPane.errorMessage = result.error.message || "Unknown error";
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Header {
            id: header
            Layout.fillWidth: true
            offlineReady: window.wasmInitialized
        }

        Toolbar {
            id: toolbar
            Layout.fillWidth: true

            onFormatRequested: (indentType) => {
                if (!inputPane.text.trim()) {
                    return;
                }
                const result = JsonBridge.formatJson(inputPane.text, indentType);
                if (result.success) {
                    outputPane.text = result.result;
                    // Re-validate after formatting
                    validateInput();
                } else {
                    outputPane.text = "Error: " + result.error;
                }
            }

            onMinifyRequested: {
                if (!inputPane.text.trim()) {
                    return;
                }
                const result = JsonBridge.minifyJson(inputPane.text);
                if (result.success) {
                    outputPane.text = result.result;
                    // Re-validate after minifying
                    validateInput();
                } else {
                    outputPane.text = "Error: " + result.error;
                }
            }

            onCopyRequested: {
                if (outputPane.text) {
                    JsonBridge.copyToClipboard(outputPane.text);
                    toolbar.copyButtonText = "Copied!";
                    copyFeedbackTimer.restart();
                }
            }

            onClearRequested: {
                inputPane.text = "";
                outputPane.text = "";
                // Reset validation state
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                statusBar.objectCount = 0;
                statusBar.arrayCount = 0;
                statusBar.totalKeys = 0;
                statusBar.maxDepth = 0;
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
            }
        }

        SplitView {
            id: splitView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: window.width >= 1200 ? Qt.Horizontal : Qt.Vertical

            handle: Rectangle {
                implicitWidth: 6
                implicitHeight: 6
                color: SplitHandle.hovered ? Theme.accent : Theme.splitHandle
            }

            InputPane {
                id: inputPane
                SplitView.preferredWidth: parent.width / 2
                SplitView.preferredHeight: parent.height / 2
                SplitView.minimumWidth: 300
                SplitView.minimumHeight: 200

                onTextChanged: {
                    validationTimer.restart();
                }
            }

            OutputPane {
                id: outputPane
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumWidth: 300
                SplitView.minimumHeight: 200
            }
        }

        StatusBar {
            id: statusBar
            Layout.fillWidth: true
        }
    }

    // Keyboard shortcuts
    Shortcut {
        sequence: "Ctrl+Shift+F"
        onActivated: toolbar.formatRequested(toolbar.selectedIndent)
    }

    Shortcut {
        sequence: "Ctrl+Shift+M"
        onActivated: toolbar.minifyRequested()
    }

    Shortcut {
        sequence: "Ctrl+Shift+C"
        onActivated: toolbar.copyRequested()
    }

    Shortcut {
        sequence: "Ctrl+Shift+Ins"
        onActivated: {
            const text = JsonBridge.readFromClipboard();
            if (text) {
                inputPane.text = text;
            }
        }
    }

    // Ctrl+V paste (Qt WASM doesn't handle browser paste events well)
    Shortcut {
        sequences: [StandardKey.Paste]
        onActivated: {
            const text = JsonBridge.readFromClipboard();
            if (text) {
                inputPane.text = text;
            }
        }
    }
}
