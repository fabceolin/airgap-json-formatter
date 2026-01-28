import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 800
    minimumWidth: 320  // Support mobile screens (AC: 1)
    minimumHeight: 480  // Support landscape phones
    title: "Airgap JSON Formatter"

    color: Theme.background

    // Responsive breakpoints (reference Theme.qml for values)
    readonly property bool isDesktop: width >= Theme.breakpointDesktop
    readonly property bool isTablet: width >= Theme.breakpointMobile && width < Theme.breakpointDesktop
    readonly property bool isMobile: width < Theme.breakpointMobile

    // Derived states for layout decisions
    readonly property bool showFullToolbar: isDesktop
    readonly property bool showCompactToolbar: isTablet
    readonly property bool showMobileToolbar: isMobile
    readonly property bool useSplitView: !isMobile
    readonly property bool useTabView: isMobile

    // Track WASM initialization state
    property bool wasmInitialized: false

    // Format auto-detection (Story 8.4)
    property string detectedFormat: "json"

    // View mode: "tree" or "text"
    property string viewMode: "tree"

    // Store current formatted JSON for both views
    property string currentFormattedJson: ""

    // Track when JsonBridge becomes ready and handle async operation results
    Connections {
        target: JsonBridge
        function onReadyChanged() {
            wasmInitialized = JsonBridge.ready;
            // Story 9.3: Check for share URL params after WASM is ready
            if (JsonBridge.ready) {
                processShareUrl();
            }
        }

        function onFormatCompleted(result) {
            if (result.success) {
                currentFormattedJson = result.result;
                outputPane.text = result.result;
                // Load tree model for tree view (synchronous)
                JsonBridge.loadTreeModel(result.result);
                // Update status bar from format result (avoid extra async validateJson call)
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Stop pending validation - formatting already validated
                validationTimer.stop();
                // Save to history via AsyncSerialiser queue
                JsonBridge.saveToHistory(result.result);
                // Auto-expand tree view after model loads
                autoExpandTimer.restart();
            } else {
                currentFormattedJson = "";
                outputPane.text = "Error: " + result.error;
            }
        }

        function onMinifyCompleted(result) {
            if (result.success) {
                currentFormattedJson = result.result;
                outputPane.text = result.result;
                // Load tree model for tree view (synchronous)
                JsonBridge.loadTreeModel(result.result);
                // Update status bar (avoid extra async validateJson call)
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Stop pending validation - minifying already validated
                validationTimer.stop();
                // Save to history via AsyncSerialiser queue
                JsonBridge.saveToHistory(result.result);
                // Switch to text mode for minified output
                window.viewMode = "text";
            } else {
                currentFormattedJson = "";
                outputPane.text = "Error: " + result.error;
            }
        }

        // Story 8.4: XML format completion handler
        function onFormatXmlCompleted(result) {
            if (result.success) {
                currentFormattedJson = result.result;
                outputPane.text = result.result;
                // Story 8.6: Load XML tree model for tree view
                JsonBridge.loadXmlTreeModel(result.result);
                // Update status bar
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Stop pending validation
                validationTimer.stop();
                // Save to history
                JsonBridge.saveToHistory(result.result);
                // Auto-expand tree view after model loads
                autoExpandTimer.restart();
            } else {
                currentFormattedJson = "";
                outputPane.text = "Error: " + result.error;
            }
        }

        // Story 8.4: XML minify completion handler
        function onMinifyXmlCompleted(result) {
            if (result.success) {
                currentFormattedJson = result.result;
                outputPane.text = result.result;
                // Story 8.6: Load XML tree model (minified still works as tree)
                JsonBridge.loadXmlTreeModel(result.result);
                // Update status bar
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Stop pending validation
                validationTimer.stop();
                // Save to history
                JsonBridge.saveToHistory(result.result);
                // Switch to text mode for minified output
                window.viewMode = "text";
            } else {
                currentFormattedJson = "";
                outputPane.text = "Error: " + result.error;
            }
        }

        function onValidateCompleted(result) {
            statusBar.isValid = result.isValid;
            // Story 9.3: Update canShare based on validation and non-empty input
            toolbar.canShare = result.isValid && inputPane.text.trim() !== "";

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

        function onHistoryLoaded(entries) {
            // History panel will receive this via its own connection
        }

        function onHistorySaved(success, id) {
            // History saved callback - can be used for feedback if needed
        }

        function onClipboardRead(content) {
            if (content && content.length > 0) {
                if (window.pasteMode === "simple") {
                    // Simple paste - just put text in input
                    inputPane.text = content;
                } else {
                    // Auto-format paste
                    handlePastedContent(content);
                }
            }
            window.pasteMode = "auto"; // Reset to default
        }

        function onCopyCompleted(success) {
            if (success) {
                toolbar.copyButtonText = "Copied!";
                copyFeedbackTimer.restart();
            }
        }

        function onBusyChanged(busy) {
            // Can be used for UI loading states
        }

        // Story 8.4: Format auto-detection
        function onFormatDetected(format) {
            window.detectedFormat = format;
        }

        // Story 10.4: Markdown with Mermaid rendering completion handler
        function onMarkdownWithMermaidRendered(html, warnings) {
            currentFormattedJson = html;
            outputPane.text = html;
            // Update status bar
            statusBar.isValid = true;
            statusBar.errorMessage = "";
            inputPane.errorLine = -1;
            inputPane.errorMessage = "";
            // Stop pending validation
            validationTimer.stop();
            // Switch to text view for rendered HTML output
            window.viewMode = "text";
            // Show warnings if any
            if (warnings && warnings.length > 0) {
                console.log("Markdown render warnings:", warnings);
            }
        }

        // Story 10.4: Markdown with Mermaid rendering error handler
        function onMarkdownWithMermaidError(error) {
            currentFormattedJson = "";
            outputPane.text = "Markdown Render Error: " + error;
            statusBar.isValid = false;
            statusBar.errorMessage = error;
        }
    }

    // Handle pasted content with auto-format
    function handlePastedContent(text) {
        // Check if we should auto-format: input is empty or fully selected
        const shouldAutoFormat = inputPane.text.trim() === "" ||
                                 inputPane.isFullySelected();

        if (shouldAutoFormat) {
            // Try to format the pasted content
            inputPane.text = text;  // Put original in input
            JsonBridge.formatJson(text, toolbar.selectedIndent);
            // Result comes via onFormatCompleted signal
        } else {
            // Has partial content - paste normally without auto-format
            inputPane.text = text;
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
        onTriggered: toolbar.copyButtonText = "Copy"
    }

    // Debounce timer for validation
    Timer {
        id: validationTimer
        interval: 300
        onTriggered: validateInput()
    }

    // Timer for auto-expand after model load
    Timer {
        id: autoExpandTimer
        interval: 50
        onTriggered: jsonTreeView.expandAll()
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
            // Story 9.3: Disable share when empty
            toolbar.canShare = false;
            return;
        }

        // Async call - result comes via onValidateCompleted signal
        JsonBridge.validateJson(inputPane.text);
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
            viewMode: window.viewMode
            detectedFormat: window.detectedFormat  // Story 8.5: Bind detected format

            // Story 8.5: Handle format selection
            onFormatSelected: function(format) {
                console.log("Format selected:", format);
            }

            onViewModeToggled: {
                window.viewMode = (window.viewMode === "tree") ? "text" : "tree"
            }

            onExpandAllRequested: {
                if (window.viewMode === "tree") {
                    jsonTreeView.expandAll()
                }
            }

            onCollapseAllRequested: {
                if (window.viewMode === "tree") {
                    jsonTreeView.collapseAll()
                }
            }

            onFormatRequested: (indentType) => {
                if (!inputPane.text.trim()) {
                    return;
                }
                // Story 8.5 + 10.4: Use effectiveFormat from toolbar (manual override or auto-detected)
                var format = toolbar.effectiveFormat;
                if (format === "xml") {
                    JsonBridge.formatXml(inputPane.text, indentType);
                } else if (format === "markdown") {
                    // Story 10.4: Route markdown to Mermaid-enabled renderer
                    JsonBridge.requestRenderMarkdownWithMermaid(inputPane.text, Theme.darkMode ? "dark" : "default");
                } else {
                    // Default to JSON for "json" or "unknown"
                    JsonBridge.formatJson(inputPane.text, indentType);
                }
            }

            onMinifyRequested: {
                if (!inputPane.text.trim()) {
                    return;
                }
                // Story 8.5: Use effectiveFormat from toolbar (manual override or auto-detected)
                var format = toolbar.effectiveFormat;
                if (format === "xml") {
                    JsonBridge.minifyXml(inputPane.text);
                } else {
                    // Default to JSON for "json" or "unknown"
                    JsonBridge.minifyJson(inputPane.text);
                }
            }

            onCopyRequested: {
                if (currentFormattedJson) {
                    // Async call - result comes via onCopyCompleted signal
                    JsonBridge.copyToClipboard(currentFormattedJson);
                }
            }

            onClearRequested: {
                inputPane.text = "";
                outputPane.text = "";
                currentFormattedJson = "";
                // Clear both tree models
                JsonBridge.treeModel.clear();
                JsonBridge.xmlTreeModel.clear();
                // Reset validation state
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                statusBar.objectCount = 0;
                statusBar.arrayCount = 0;
                statusBar.totalKeys = 0;
                statusBar.maxDepth = 0;
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Story 9.3: Disable share when cleared
                toolbar.canShare = false;
                // Story 8.5: Reset format selector to Auto (AC: 5)
                toolbar.resetFormatSelector();
            }

            onLoadHistoryRequested: {
                historyPanel.open();
            }

            // Story 9.3: Open share dialog
            onShareRequested: {
                shareDialog.open();
            }
        }

        // Desktop/Tablet: SplitView layout
        SplitView {
            id: splitView
            visible: window.useSplitView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: window.width >= 1200 ? Qt.Horizontal : Qt.Vertical

            handle: Rectangle {
                implicitWidth: window.isTablet ? 24 : 6  // Touch-friendly on tablets
                implicitHeight: window.isTablet ? 24 : 6
                color: SplitHandle.hovered ? Theme.accent : Theme.splitHandle
            }

            InputPane {
                id: inputPane
                SplitView.preferredWidth: parent.width / 2
                SplitView.preferredHeight: parent.height / 2
                SplitView.minimumWidth: window.isTablet ? 200 : 300  // Reduced for tablets (Task 5)
                SplitView.minimumHeight: window.isTablet ? 150 : 200

                onTextChanged: {
                    validationTimer.restart();
                    // Story 8.4: Auto-detect format on input change
                    if (text.length > 0) {
                        JsonBridge.detectFormat(text);
                    } else {
                        window.detectedFormat = "json";  // Default when empty
                    }
                }
            }

            // Output area - switches between TreeView and TextArea
            Item {
                id: outputArea
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumWidth: window.isTablet ? 200 : 300  // Reduced for tablets (Task 5)
                SplitView.minimumHeight: window.isTablet ? 150 : 200

                OutputPane {
                    id: outputPane
                    anchors.fill: parent
                    visible: viewMode === "text"
                }

                JsonTreeView {
                    id: jsonTreeView
                    anchors.fill: parent
                    visible: viewMode === "tree"
                    model: window.detectedFormat === "xml" ? JsonBridge.xmlTreeModel : JsonBridge.treeModel
                }
            }
        }

        // Mobile: Tab-based input/output toggle
        InputOutputToggle {
            id: inputOutputToggle
            visible: window.useTabView
            Layout.fillWidth: true
            Layout.fillHeight: true

            inputContent: Component {
                InputPane {
                    id: mobileInputPane

                    // Sync with desktop inputPane
                    Component.onCompleted: {
                        // Bind text bidirectionally
                        text = Qt.binding(function() { return inputPane.text; });
                    }

                    onTextChanged: {
                        if (text !== inputPane.text) {
                            inputPane.text = text;
                        }
                        validationTimer.restart();
                        // Story 8.4: Auto-detect format on input change
                        if (text.length > 0) {
                            JsonBridge.detectFormat(text);
                        } else {
                            window.detectedFormat = "json";  // Default when empty
                        }
                    }

                    // Mirror error state from desktop inputPane
                    errorLine: inputPane.errorLine
                    errorMessage: inputPane.errorMessage
                }
            }

            outputContent: Component {
                Item {
                    // Output area - switches between TreeView and TextArea (mirrors desktop)
                    OutputPane {
                        id: mobileOutputPane
                        anchors.fill: parent
                        visible: viewMode === "text"
                        text: outputPane.text
                    }

                    JsonTreeView {
                        id: mobileJsonTreeView
                        anchors.fill: parent
                        visible: viewMode === "tree"
                        model: window.detectedFormat === "xml" ? JsonBridge.xmlTreeModel : JsonBridge.treeModel
                    }
                }
            }
        }

        StatusBar {
            id: statusBar
            Layout.fillWidth: true
            // Story 8.4: Bind detected format
            detectedFormat: window.detectedFormat
        }
    }

    // History panel (drawer from right)
    HistoryPanel {
        id: historyPanel
        parent: Overlay.overlay

        onEntrySelected: (content) => {
            // Stop any pending validation - format will re-validate
            validationTimer.stop();
            inputPane.text = content;
            // Direct call - AsyncSerialiser handles operation serialization
            JsonBridge.formatJson(content, toolbar.selectedIndent);
        }
    }

    // Story 9.3: Share dialog
    ShareDialog {
        id: shareDialog
        parent: Overlay.overlay

        onShareConfirmed: (passphrase) => {
            shareDialog.isBusy = true;
            shareDialog.errorMessage = "";

            // Get JSON to share (prefer formatted output, fallback to input)
            const jsonToShare = currentFormattedJson || inputPane.text;
            if (!jsonToShare || !jsonToShare.trim()) {
                shareDialog.errorMessage = "No JSON to share";
                shareDialog.isBusy = false;
                return;
            }

            // Call bridge to create share URL
            const resultStr = JsonBridge.createShareUrl(jsonToShare, passphrase);
            try {
                const result = JSON.parse(resultStr);
                if (result.success) {
                    // Copy URL to clipboard and close dialog
                    JsonBridge.copyToClipboard(result.url);
                    shareDialog.close();

                    // Show appropriate status message
                    if (result.mode === "quick") {
                        statusBar.showMessage("Share link copied! Expires in 5 min", "success");
                    } else {
                        statusBar.showMessage("Protected share link copied! Recipient needs passphrase", "success");
                    }
                } else {
                    // Show error in dialog
                    shareDialog.errorMessage = result.error || "Failed to create share link";
                    shareDialog.isBusy = false;
                }
            } catch (e) {
                shareDialog.errorMessage = "Error creating share link";
                shareDialog.isBusy = false;
            }
        }

        onCancelled: {
            // Dialog cancelled, no action needed
        }
    }

    // Story 9.3: Passphrase prompt for protected share links
    PassphrasePrompt {
        id: passphrasePrompt
        parent: Overlay.overlay

        // Store pending share data for retry
        property string pendingData: ""

        onPassphraseEntered: (passphrase) => {
            if (!pendingData) {
                passphrasePrompt.showError("No share data to decrypt");
                return;
            }

            // Try to decode with passphrase
            const resultStr = JsonBridge.decodeShare(pendingData, passphrase, true);
            try {
                const result = JSON.parse(resultStr);
                if (result.success) {
                    // Success - load JSON
                    passphrasePrompt.close();
                    pendingData = "";
                    JsonBridge.clearShareParams();
                    inputPane.text = result.json;
                    JsonBridge.formatJson(result.json, toolbar.selectedIndent);
                    statusBar.showMessage("Loaded shared JSON", "success");
                } else {
                    // Check error type for appropriate message
                    if (result.errorCode === "wrong_passphrase" || result.errorCode === "decryption_failed") {
                        passphrasePrompt.showError("Incorrect passphrase");
                    } else if (result.errorCode === "expired") {
                        passphrasePrompt.close();
                        pendingData = "";
                        JsonBridge.clearShareParams();
                        statusBar.showMessage("This share link has expired (links valid for 5 minutes)", "error");
                    } else {
                        passphrasePrompt.showError(result.error || "Failed to decrypt");
                    }
                }
            } catch (e) {
                passphrasePrompt.showError("Error decrypting share link");
            }
        }

        onCancelled: {
            pendingData = "";
            JsonBridge.clearShareParams();
        }
    }

    // Story 9.3: Process share URL on startup (after WASM ready)
    function processShareUrl() {
        const paramsStr = JsonBridge.getShareParams();
        try {
            const params = JSON.parse(paramsStr);
            if (!params.hasShareData) {
                return; // No share data in URL
            }

            if (params.isProtected) {
                // Protected share: show passphrase prompt
                passphrasePrompt.pendingData = params.data;
                passphrasePrompt.open();
            } else {
                // Quick share: decode immediately with key from fragment
                if (!params.key) {
                    JsonBridge.clearShareParams();
                    statusBar.showMessage("Invalid or corrupted share link", "error");
                    return;
                }

                const resultStr = JsonBridge.decodeShare(params.data, params.key, false);
                const result = JSON.parse(resultStr);

                JsonBridge.clearShareParams();

                if (result.success) {
                    inputPane.text = result.json;
                    JsonBridge.formatJson(result.json, toolbar.selectedIndent);
                    statusBar.showMessage("Loaded shared JSON", "success");
                } else if (result.errorCode === "expired") {
                    statusBar.showMessage("This share link has expired (links valid for 5 minutes)", "error");
                } else {
                    statusBar.showMessage("Invalid or corrupted share link", "error");
                }
            }
        } catch (e) {
            console.error("Error processing share URL:", e);
            JsonBridge.clearShareParams();
        }
    }

    // Mobile overflow menu (full window overlay)
    OverflowMenu {
        id: overflowMenu
        anchors.fill: parent
        visible: window.isMobile
        currentIndent: toolbar.selectedIndent
        viewMode: window.viewMode

        onOnLoad: historyPanel.open()
        onOnClear: toolbar.clearRequested()
        onOnMinify: toolbar.minifyRequested()
        onOnCopy: toolbar.copyRequested()
        onOnIndentChange: (indent) => {
            toolbar.selectedIndent = indent;
        }
        onOnViewToggle: toolbar.viewModeToggled()
        onOnThemeToggle: Theme.toggleTheme()
        onOnExpandAll: toolbar.expandAllRequested()
        onOnCollapseAll: toolbar.collapseAllRequested()
    }

    // Wire up toolbar overflow menu signal
    Connections {
        target: toolbar
        function onOverflowMenuRequested() {
            overflowMenu.open();
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
            // Async call - result comes via onClipboardRead signal
            // Set a flag so we know this is a simple paste (no auto-format)
            window.pasteMode = "simple";
            JsonBridge.readFromClipboard();
        }
    }

    // Track paste mode for clipboard read handler
    property string pasteMode: "auto"

    // Ctrl+V paste with auto-format (Qt WASM doesn't handle browser paste events well)
    Shortcut {
        sequences: [StandardKey.Paste]
        onActivated: {
            // Async call - result comes via onClipboardRead signal
            window.pasteMode = "auto";
            JsonBridge.readFromClipboard();
        }
    }

    // TreeView expand/collapse shortcuts
    Shortcut {
        sequence: "Ctrl+E"
        enabled: viewMode === "tree"
        onActivated: jsonTreeView.expandAll()
    }

    Shortcut {
        sequence: "Ctrl+Shift+E"
        enabled: viewMode === "tree"
        onActivated: jsonTreeView.collapseAll()
    }

    // Toggle view mode
    Shortcut {
        sequence: "Ctrl+T"
        onActivated: viewMode = (viewMode === "tree") ? "text" : "tree"
    }

    // Open history panel
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: historyPanel.open()
    }

    // Story 9.3: Open share dialog
    Shortcut {
        sequence: "Ctrl+Shift+S"
        enabled: toolbar.canShare
        onActivated: shareDialog.open()
    }
}
