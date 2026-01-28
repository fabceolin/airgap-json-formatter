import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Item {
    id: inputOutputToggle

    // Active pane: "input" or "output"
    property string activePane: "input"

    // Content components to be set by parent
    property Component inputContent: null
    property Component outputContent: null

    // Expose loaders for testing (UNIT-011)
    property alias inputLoader: inputLoader
    property alias outputLoader: outputLoader

    // Signal for tab changes (UNIT-012)
    signal tabChanged(var payload)

    // Tab bar height
    readonly property int tabBarHeight: Theme.mobileButtonHeight

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
        Rectangle {
            id: tabBar
            Layout.fillWidth: true
            height: inputOutputToggle.tabBarHeight
            color: Theme.backgroundTertiary

            // Bottom border
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.border
            }

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // Input Tab
                Rectangle {
                    id: inputTab
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: inputOutputToggle.activePane === "input" ? Theme.backgroundSecondary : "transparent"

                    // Active indicator
                    Rectangle {
                        visible: inputOutputToggle.activePane === "input"
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 3
                        color: Theme.accent
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "Input"
                        color: inputOutputToggle.activePane === "input" ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: Theme.mobileFontSize
                        font.weight: inputOutputToggle.activePane === "input" ? Font.Medium : Font.Normal
                    }

                    // Touch/Mouse handling with fallback for Qt WASM touch issues
                    MouseArea {
                        anchors.fill: parent
                        // Accept both touch and mouse for fallback (Task 8)
                        onClicked: inputTabAction()
                        onPressed: {
                            // Timer-based fallback: if touch doesn't complete, mouse will
                            inputTabFallbackTimer.restart();
                        }
                        onReleased: inputTabFallbackTimer.stop()
                    }

                    Timer {
                        id: inputTabFallbackTimer
                        interval: 100
                        onTriggered: inputTabAction()
                    }

                    function inputTabAction() {
                        if (inputOutputToggle.activePane !== "input") {
                            var previousPane = inputOutputToggle.activePane;
                            inputOutputToggle.activePane = "input";
                            inputOutputToggle.tabChanged({previous: previousPane, current: "input"});
                        }
                    }
                }

                // Separator
                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: Theme.border
                }

                // Output Tab
                Rectangle {
                    id: outputTab
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: inputOutputToggle.activePane === "output" ? Theme.backgroundSecondary : "transparent"

                    // Active indicator
                    Rectangle {
                        visible: inputOutputToggle.activePane === "output"
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 3
                        color: Theme.accent
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "Output"
                        color: inputOutputToggle.activePane === "output" ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: Theme.mobileFontSize
                        font.weight: inputOutputToggle.activePane === "output" ? Font.Medium : Font.Normal
                    }

                    // Touch/Mouse handling with fallback for Qt WASM touch issues
                    MouseArea {
                        anchors.fill: parent
                        // Accept both touch and mouse for fallback (Task 8)
                        onClicked: outputTabAction()
                        onPressed: {
                            // Timer-based fallback: if touch doesn't complete, mouse will
                            outputTabFallbackTimer.restart();
                        }
                        onReleased: outputTabFallbackTimer.stop()
                    }

                    Timer {
                        id: outputTabFallbackTimer
                        interval: 100
                        onTriggered: outputTabAction()
                    }

                    function outputTabAction() {
                        if (inputOutputToggle.activePane !== "output") {
                            var previousPane = inputOutputToggle.activePane;
                            inputOutputToggle.activePane = "output";
                            inputOutputToggle.tabChanged({previous: previousPane, current: "output"});
                        }
                    }
                }
            }
        }

        // Content area - StackLayout keeps both panes alive
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: inputOutputToggle.activePane === "input" ? 0 : 1

            // Input pane - always active to preserve state (UNIT-011)
            Loader {
                id: inputLoader
                active: true  // Never destroy to preserve cursor/scroll/undo
                sourceComponent: inputOutputToggle.inputContent
            }

            // Output pane - always active to preserve scroll position (UNIT-011)
            Loader {
                id: outputLoader
                active: true  // Never destroy to preserve scroll position
                sourceComponent: inputOutputToggle.outputContent
            }
        }
    }

    // Initialize activePane on component completion (UNIT-009)
    Component.onCompleted: {
        activePane = "input";
    }
}
